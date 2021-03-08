// Fill out your copyright notice in the Description page of Project Settings.

#include "NavMeshGenerator.h"
#include "CustomNavigationData.h"

ACustomNavigationData::ACustomNavigationData()
{
    /*FindPathImplementation = FindPath;*/
}

FPathFindingResult ACustomNavigationData::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
    /*GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Path requested"));*/
    return FPathFindingResult();
}

void ACustomNavigationData::ConditionalConstructGenerator()
{
    if (!NavDataGenerator.IsValid())
    {
        //Initialize the generator and the navmesh on the first opening of the editor
        TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NewGen(new FNavMeshGenerator());
        NewGen.Get()->SetNavmesh(this);
        NavDataGenerator = MoveTemp(NewGen);

        NewGen.Get()->GatherValidOverlappingGeometries();
        NewGen.Get()->GenerateNavmesh();
    }
}

UPrimitiveComponent* ACustomNavigationData::ConstructRenderingComponent()
{
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Update Display Debug"));
    return nullptr;
}
