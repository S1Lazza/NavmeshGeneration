// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshGenerator.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "Contour.h"
#include "PolygonMesh.h"
#include "DetailedMesh.h"
#include "NavMeshController.h"
#include "CustomNavigationData.h"
#include "../Utility/UtilityGeneral.h"
#include "../Utility/UtilityDebug.h"
#include "Kismet/KismetSystemLibrary.h"

bool FNavMeshGenerator::RebuildAll()
{
	if (NavigationMesh->GetGenerator())
	{
		//This need to be called in case the controller is accidentally deleted
		NavigationMesh->CreateNavmeshController();
		NavigationMesh->GetNavmeshController()->UpdateEditorPosition();

		GatherValidOverlappingGeometries();
		GenerateNavmesh();

		NavigationMesh->GetNavmeshController()->DisplayDebugElements();

		return true;
	}

	return false;
}

void FNavMeshGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	//Naive implemetation as all the navmesh is rebuilt when a geometry is moved in the level
	if (NavigationMesh->GetGenerator() && NavigationMesh->GetNavmeshController()->EnableDirtyAreasRebuild)
	{
		NavigationMesh->CreateNavmeshController();
		NavigationMesh->GetNavmeshController()->UpdateEditorPosition();

		GatherValidOverlappingGeometries();
		GenerateNavmesh();

		NavigationMesh->GetNavmeshController()->DisplayDebugElements();
	}
}

void FNavMeshGenerator::GatherValidOverlappingGeometries()
{
	Geometries.Empty();

	TArray<AActor*> ValidGeometries;

	//Query parameters for filtering the overlapping actors, ObjectTypeQuery1 = WorldStatic
	TArray< TEnumAsByte<EObjectTypeQuery> > ObjectTypes;
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);
	
	TArray<AActor*> ActorsToIgnore;

	//Check which actors are overlapping with the box based on the parameters specified
	UKismetSystemLibrary::BoxOverlapActors(NavigationMesh->GetWorld(), NavBounds.GetCenter(), NavBounds.GetExtent(), ObjectTypes, nullptr, ActorsToIgnore, ValidGeometries);

	for (AActor*& actor : ValidGeometries)
	{
		UActorComponent* ActorCmp = actor->GetComponentByClass(UStaticMeshComponent::StaticClass());

		//Check if the overlapping actors have a static mesh component
		if (ActorCmp)
		{
			//Check if the actor is flagged as been able to affect the navigation
			if (ActorCmp->CanEverAffectNavigation())
			{
				//Check if the actor collision has been set to block pawns
				UStaticMeshComponent* Mesh = CastChecked<UStaticMeshComponent>(ActorCmp);

				if (Mesh->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block)
				{
					//Add the mesh to the array
					Geometries.Add(Mesh);
				}
			}
		}
	}
}

void FNavMeshGenerator::GenerateNavmesh()
{
	if (Geometries.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No valid geometries detected inside the nav bound, navmesh data generation aborted"));
		return;
	}

	InitializeNavmeshObjects();

	SolidHF->InitializeParameters(NavigationMesh->GetNavmeshController());
	
	//Calculate the extension of the nav bound and pass the data found to the solidheightfield to determine the area covered by it
	FVector NavCenter = NavBounds.GetCenter();
	FVector BoxBoundCoord = NavBounds.GetExtent();
	float MaxCoord = FMath::Max(BoxBoundCoord.X, BoxBoundCoord.Y);
	FVector MaxBoxBoundsCoord(MaxCoord, MaxCoord, BoxBoundCoord.Z);
	
	SolidHF->DefineFieldsBounds(NavCenter, MaxBoxBoundsCoord);

	for (UStaticMeshComponent* Mesh : Geometries)
	{
		CreateSolidHeightfield(Mesh);
	}

	CreateOpenHeightfield();
	CreateContour();
	CreatePolygonMesh();

	SendDataToNavmesh();
}

//TODO: Need to clean them before creating the new ones
void FNavMeshGenerator::InitializeNavmeshObjects()
{
	//The objects are reinitialized every single time the navmesh is updated in editor for an easier cleanup of the intermediate data
	SolidHF = NewObject<USolidHeightfield>(USolidHeightfield::StaticClass());
	OpenHF = NewObject<UOpenHeightfield>(UOpenHeightfield::StaticClass());
	Contour = NewObject<UContour>(UContour::StaticClass());
	PolygonMesh = NewObject<UPolygonMesh>(UPolygonMesh::StaticClass());
	DetailedMesh = NewObject<UDetailedMesh>(UDetailedMesh::StaticClass());
}

void FNavMeshGenerator::CreateSolidHeightfield(const UStaticMeshComponent* Mesh)
{
	TArray<FVector> Vertices;
	TArray<int> Indices;

	UUtilityGeneral::GetAllMeshVertices(Mesh, Vertices);
	UUtilityGeneral::GetMeshIndices(Mesh, Indices);
	
	FString TextToDisplay = Mesh->GetOwner()->GetName();
	FString AdditionalText = " has no geometry data to generate the solid heightfield";
	TextToDisplay += AdditionalText;

	if (Vertices.Num() == 0 || Indices.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *TextToDisplay);
		return;
	}

	SolidHF->VoxelizeTriangles(Vertices, Indices);
	SolidHF->MarkLowHeightSpan();
	SolidHF->MarkLedgeSpan();
}

void FNavMeshGenerator::CreateOpenHeightfield()
{
	OpenHF->InitializeParameters(SolidHF, NavigationMesh->GetNavmeshController());
	OpenHF->FindOpenSpanData(SolidHF);

	if (OpenHF->GetPerformFullGeneration())
	{
		OpenHF->GenerateNeightborLinks();
		OpenHF->GenerateDistanceField();
		OpenHF->GenerateRegions();
		OpenHF->HandleSmallRegions();
		OpenHF->ReassignBorderSpan();
		/*OpenHF->CleanRegionBorders();*/
	}
}

void FNavMeshGenerator::CreateContour()
{
	Contour->InitializeParameters(OpenHF, NavigationMesh->GetNavmeshController());
	Contour->GenerateContour(OpenHF);
}

void FNavMeshGenerator::CreatePolygonMesh()
{
	PolygonMesh->InitializeParameters(NavigationMesh->GetNavmeshController());
	PolygonMesh->GeneratePolygonMesh(Contour, true, 0);
}

void FNavMeshGenerator::CreateDetailedMesh()
{
}

void FNavMeshGenerator::SendDataToNavmesh()
{
	NavigationMesh->SetResultingPoly(PolygonMesh->GetResultingPoly());
}

void FNavMeshGenerator::SetNavmesh(ACustomNavigationData* NavMesh)
{
	NavigationMesh = NavMesh;
}

void FNavMeshGenerator::SetNavBounds()
{
	//Currently only the first bound is assigned to work with the custom navmesh
	NavBounds = NavigationMesh->GetNavigableBounds()[0];
}

