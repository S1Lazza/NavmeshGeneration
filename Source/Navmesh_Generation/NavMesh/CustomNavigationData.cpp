// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomNavigationData.h"
#include "NavMeshGenerator.h"
#include "NavmeshController.h"

ACustomNavigationData::ACustomNavigationData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		FindPathImplementation = FindPath;
		FindHierarchicalPathImplementation = FindPath;
	}
}

void ACustomNavigationData::BeginPlay()
{
	Super::BeginPlay();
}

FPathFindingResult ACustomNavigationData::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    /*GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Path requested"));*/

	const ANavigationData* Self = Query.NavData.Get();
	const ACustomNavigationData* CustomNavmesh = (const ACustomNavigationData*)Self;
	check(CustomNavmesh != nullptr);

	if (CustomNavmesh == nullptr)
	{
		return ENavigationQueryResult::Error;
	}

	FPathFindingResult Result(ENavigationQueryResult::Error);
	Result.Path = Query.PathInstanceToFill.IsValid() ? Query.PathInstanceToFill : Self->CreatePathInstance<FNavigationPath>(Query);

	FNavigationPath* NavPath = Result.Path.Get();

	if (NavPath != nullptr)
	{
		if ((Query.StartLocation - Query.EndLocation).IsNearlyZero())
		{
			Result.Path->GetPathPoints().Reset();
			Result.Path->GetPathPoints().Add(FNavPathPoint(Query.EndLocation));
			Result.Result = ENavigationQueryResult::Success;
		}
		else if (Query.QueryFilter.IsValid())
		{
			for (auto& Point : CustomNavmesh->GetResultingPoly())
			{
				NavPath->GetPathPoints().Add(FNavPathPoint(Point.Centroid));
			}

			NavPath->MarkReady();
			Result.Result = ENavigationQueryResult::Success;
		}
		return Result;
	}

    return FPathFindingResult();
}

void ACustomNavigationData::ConditionalConstructGenerator()
{
    if (!NavDataGenerator.IsValid())
    {
        //Initialize the generator
        TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NewGen(new FNavMeshGenerator());
        NewGen.Get()->SetNavmesh(this);
        NewGen.Get()->SetNavBounds();

        //Initialize the controller
        CreateNavmeshController();
        NavMeshController->SetNavGenerator(NewGen);
        NavMeshController->UpdateEditorPosition();

        //Construct the navmesh
        NewGen.Get()->GatherValidOverlappingGeometries();
        NewGen.Get()->GenerateNavmesh();

        NavDataGenerator = MoveTemp(NewGen);
    }
}

UPrimitiveComponent* ACustomNavigationData::ConstructRenderingComponent()
{
    return nullptr;
}

void ACustomNavigationData::CreateNavmeshController()
{
    //Make sure the controller is created only if it doesn't already exist
    if (!NavMeshController)
    {
        FActorSpawnParameters SpawnInfo;
        SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        FVector SpawnLocation = GetNavigableBounds()[0].GetCenter();

        NavMeshController = GetWorld()->SpawnActor<ANavMeshController>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);
    }
}

void ACustomNavigationData::SetResultingPoly(TArray<FPolygonData> NavPoly)
{
    ResultingPoly = NavPoly;
}
