// Fill out your copyright notice in the Description page of Project Settings.

#include "NavMeshGenerator.h"
#include "NavmeshController.h"
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
        //Initialize the generator and the navmesh logic on the first opening of the editor
        TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NewGen(new FNavMeshGenerator());
        NewGen.Get()->SetNavmesh(this);
        NewGen.Get()->SetNavBounds(this);
        NavDataGenerator = MoveTemp(NewGen);

        CreateNavmeshController();

        NewGen.Get()->GatherValidOverlappingGeometries();
        NewGen.Get()->GenerateNavmesh();
    }
}

UPrimitiveComponent* ACustomNavigationData::ConstructRenderingComponent()
{
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Update Display Debug"));
    return nullptr;
}

void ACustomNavigationData::CreateNavmeshController()
{
    //Make sure the controller is created only if it doesn't exist
    if (!NavMeshController)
    {
        FActorSpawnParameters SpawnInfo;
        SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        FVector SpawnLocation = GetNavigableBounds()[0].GetCenter();

        NavMeshController = GetWorld()->SpawnActor<ANavMeshController>(SpawnLocation, FRotator(0.f, 0.f, 0.f), SpawnInfo);
    }
}

void ACustomNavigationData::UpdateControllerPosition()
{
    NavMeshController->SetActorLocation(GetNavigableBounds()[0].GetCenter());
}
