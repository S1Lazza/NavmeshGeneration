// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshGenerator.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "Contour.h"
#include "PolygonMesh.h"
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
	if (NavigationMesh->GetGenerator() && EnableDirtyAreasRebuild)
	{
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

	UWorld* World = NavigationMesh->GetWorld();
	FVector SpawnLocation = NavBounds.GetCenter();

	ClearDebugLines(World);

	//Initialize spawn parameters, needed to create a new instance of the actors (the always spawn flag is used for safety purposes)
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASolidHeightfield* SolidHF = World->SpawnActor<ASolidHeightfield>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);
	AOpenHeightfield* OpenHF = World->SpawnActor<AOpenHeightfield>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);
	AContour* Contour = World->SpawnActor<AContour>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);
	APolygonMesh* PolygonMesh = World->SpawnActor<APolygonMesh>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);

	FVector BoxBoundCoord = NavBounds.GetExtent();
	float MaxCoord = FMath::Max(BoxBoundCoord.X, BoxBoundCoord.Y);
	FVector MaxBoxBoundsCoord(MaxCoord, MaxCoord, BoxBoundCoord.Z);

	SolidHF->DefineFieldsBounds(SpawnLocation, MaxBoxBoundsCoord);

	for (UStaticMeshComponent* Mesh : Geometries)
	{
		CreateSolidHeightfield(SolidHF, Mesh);
	}

	CreateOpenHeightfield(OpenHF, SolidHF, true);
	CreateContour(Contour, OpenHF);
	CreatePolygonMesh(PolygonMesh, Contour, OpenHF);
}

void FNavMeshGenerator::CreateSolidHeightfield(ASolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh)
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

void FNavMeshGenerator::CreateOpenHeightfield(AOpenHeightfield* OpenHeightfield, ASolidHeightfield* SolidHeightfield, bool PerformFullGeneration)
{
	OpenHeightfield->Init(SolidHeightfield);
	OpenHeightfield->FindOpenSpanData(SolidHeightfield);
	/*OpenHeightfield->DrawDebugSpanData();*/

	if (PerformFullGeneration)
	{
		OpenHeightfield->GenerateNeightborLinks();
		OpenHeightfield->GenerateDistanceField();
		/*OpenHeightfield->DrawDistanceFieldDebugData(false, true);*/
		OpenHeightfield->GenerateRegions();

		//TODO: There's an infinite loop when the MinUnconnectedRegion parameter is set to 0 or 1, check why
		/*OpenHeightfield->HandleSmallRegions();*/
		/*OpenHeightfield->DrawDebugRegions(false, true);*/
		SolidHeightfield->FreeSpanData();
	}
}

void FNavMeshGenerator::CreateContour(AContour* Contour, AOpenHeightfield* OpenHeightfield)
{
	Contour->Init(OpenHeightfield);
	Contour->GenerateContour(OpenHeightfield);
	OpenHeightfield->FreeSpanData();
}

void FNavMeshGenerator::CreatePolygonMesh(APolygonMesh* PolyMesh, const AContour* Contour, const AOpenHeightfield* OpenHeightfield)
{
	PolyMesh->Init(OpenHeightfield);
	PolyMesh->GeneratePolygonMesh(Contour);
	PolyMesh->SendDataToNavmesh(NavigationMesh->ResultingPoly);
}

void FNavMeshGenerator::ClearDebugLines(UWorld* CurrentWorld)
{
	FlushPersistentDebugLines(CurrentWorld);
}

void FNavMeshGenerator::SetNavmesh(ACustomNavigationData* NavMesh)
{
	NavigationMesh = NavMesh;
	NavBounds = NavigationMesh->GetNavigableBounds()[0];
}

