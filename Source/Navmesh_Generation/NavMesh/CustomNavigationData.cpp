// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomNavigationData.h"
#include "NavMeshGenerator.h"
#include "NavmeshController.h"

FPathFindingResult ACustomNavigationData::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    /*GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Path requested"));*/
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
