// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshGenerator.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "Contour.h"
#include "PolygonMesh.h"
#include "NavMeshController.h"
#include "CustomNavigationData.h"
#include "../Utility/UtilityGeneral.h"
#include "../Utility/UtilityDebug.h"
#include "Kismet/KismetSystemLibrary.h"


//// Sets default values
FNavMeshGenerator::FNavMeshGenerator(ACustomNavigationData* InNavmesh)
{
	NavigationMesh = InNavmesh;
	NavBounds = NavigationMesh->GetNavigableBounds()[0];
}

bool FNavMeshGenerator::RebuildAll()
{
	if (NavigationMesh->GetGenerator())
	{
		NavigationMesh->CreateNavmeshController();
		NavigationMesh->UpdateControllerPosition();
		GatherValidOverlappingGeometries();
		GenerateNavmesh();
		return true;
	}

	return false;
}

void FNavMeshGenerator::RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas)
{
	//Naive implemetation as all the navmesh is rebuilt when a geometry is moved in the level
	//TODO: Move the enabling in a different place, ideally inside a controller with all the other parameters
	if (NavigationMesh->GetGenerator() && NavigationMesh->NavMeshController->EnableDirtyAreasRebuild)
	{
		NavigationMesh->CreateNavmeshController();
		NavigationMesh->UpdateControllerPosition();
		GatherValidOverlappingGeometries();
		GenerateNavmesh();
	}
}

void FNavMeshGenerator::GatherValidOverlappingGeometries()
{
	ClearDebugLines(NavigationMesh->GetWorld());
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
		return;
	}

	InitializeNavmeshObjects();

	FVector NavCenter = NavBounds.GetCenter();
	FVector BoxBoundCoord = NavBounds.GetExtent();
	float MaxCoord = FMath::Max(BoxBoundCoord.X, BoxBoundCoord.Y);
	FVector MaxBoxBoundsCoord(MaxCoord, MaxCoord, BoxBoundCoord.Z);

	SolidHF->InitializeParameters(NavigationMesh->NavMeshController);
	SolidHF->DefineFieldsBounds(NavCenter, MaxBoxBoundsCoord);

	for (UStaticMeshComponent* Mesh : Geometries)
	{
		CreateSolidHeightfield(SolidHF, Mesh);
	}

	CreateOpenHeightfield(OpenHF, SolidHF, NavigationMesh->NavMeshController);
	CreateContour(Contour, OpenHF);
	CreatePolygonMesh(PolygonMesh, Contour, OpenHF);
}

void FNavMeshGenerator::InitializeNavmeshObjects()
{
	SolidHF = NewObject<USolidHeightfield>(USolidHeightfield::StaticClass());
	SolidHF->CurrentWorld = NavigationMesh->GetWorld();

	OpenHF = NewObject<UOpenHeightfield>(UOpenHeightfield::StaticClass());

	Contour = NewObject<UContour>(UContour::StaticClass());
	PolygonMesh = NewObject<UPolygonMesh>(UPolygonMesh::StaticClass());
}

void FNavMeshGenerator::CreateSolidHeightfield(USolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh)
{
	TArray<FVector> Vertices;
	TArray<int> Indices;

	UUtilityGeneral::GetAllMeshVertices(Mesh, Vertices);
	UUtilityGeneral::GetMeshIndices(Mesh, Indices);

	if (Vertices.Num() == 0 || Indices.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("No data to generate the solid heightfield"));
		return;
	}

	SolidHeightfield->VoxelizeTriangles(Vertices, Indices);
	SolidHeightfield->MarkLowHeightSpan();

	//TODO Currently the ledge spans are incorrectly filtered, need to check the method below
	/*SolidHeightfield->MarkLedgeSpan();*/

	/*SolidHeightfield->DrawDebugSpanData();*/
}

void FNavMeshGenerator::CreateOpenHeightfield(UOpenHeightfield* OpenHeightfield, USolidHeightfield* SolidHeightfield, const ANavMeshController* NavController)
{
	OpenHeightfield->InitializeParameters(SolidHeightfield, NavController);
	OpenHeightfield->FindOpenSpanData(SolidHeightfield);
	/*OpenHeightfield->DrawDebugSpanData();*/

	if (OpenHeightfield->GetPerformFullGeneration())
	{
		OpenHeightfield->GenerateNeightborLinks();
		OpenHeightfield->GenerateDistanceField();
		/*OpenHeightfield->DrawDistanceFieldDebugData(false, true);*/
		OpenHeightfield->GenerateRegions();

		//TODO: There's an infinite loop when the MinUnconnectedRegion parameter is set to 0 or 1, check why
		/*OpenHeightfield->HandleSmallRegions();*/
		/*OpenHeightfield->DrawDebugRegions(false, true);*/
	}
}

void FNavMeshGenerator::CreateContour(UContour* MeshContour, UOpenHeightfield* OpenHeightfield)
{
	MeshContour->InitializeParameters(OpenHeightfield, NavigationMesh->NavMeshController);
	MeshContour->GenerateContour(OpenHeightfield);
	/*MeshContour->DrawRegionRawContour();*/
}

void FNavMeshGenerator::CreatePolygonMesh(UPolygonMesh* PolyMesh, const UContour* MeshContour, const UOpenHeightfield* OpenHeightfield)
{
	PolyMesh->InitializeParameters(NavigationMesh->NavMeshController);
	PolyMesh->GeneratePolygonMesh(MeshContour, true, 1);
	/*PolyMesh->DrawDebugPolyMeshPolys();*/
	PolyMesh->SendDataToNavmesh(NavigationMesh->ResultingPoly);
}

void FNavMeshGenerator::ClearDebugLines(UWorld* CurrentWorld)
{
	FlushPersistentDebugLines(CurrentWorld);
}

void FNavMeshGenerator::SetNavmesh(ACustomNavigationData* NavMesh)
{
	NavigationMesh = NavMesh;
}

void FNavMeshGenerator::SetNavBounds(ACustomNavigationData* NavMesh)
{
	//Currently only the first bound is assigned to work with the custom navmesh
	NavBounds = NavMesh->GetNavigableBounds()[0];
}

