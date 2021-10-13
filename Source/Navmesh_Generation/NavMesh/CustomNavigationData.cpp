// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomNavigationData.h"
#include "NavMeshGenerator.h"
#include "NavmeshController.h"
#include "../Pathfinding/Pathfinding.h"

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
			TArray<FVector> PathPoints;
			PathPoints = UPathfinding::BuildPath(Query.StartLocation, Query.EndLocation, CustomNavmesh);

			for (auto& Point : PathPoints)
			{
				NavPath->GetPathPoints().Add(FNavPathPoint(Point));
			}

			NavPath->MarkReady();
			Result.Result = ENavigationQueryResult::Success;
		}
	}

	return Result;
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
        NavMeshController = GetWorld()->SpawnActor<ANavMeshController>(FVector(0.f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f), SpawnInfo);
    }
}

void ACustomNavigationData::SetResultingPoly(TArray<FPolygonData> NavPoly)
{
    ResultingPoly = NavPoly;
}
