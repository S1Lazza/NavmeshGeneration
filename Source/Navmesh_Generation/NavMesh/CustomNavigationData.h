// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavigationData.h"
#include "PolygonMesh.h"
#include "NavMeshGenerator.h"
#include "CustomNavigationData.generated.h"

UCLASS()
class NAVMESH_GENERATION_API ACustomNavigationData : public ANavigationData
{
	GENERATED_BODY()

public:
	ACustomNavigationData();

	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	virtual void ConditionalConstructGenerator() override;

	virtual UPrimitiveComponent* ConstructRenderingComponent() override;

	TArray<FPolygonData> ResultingPoly;
};
