// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavigationData.h"
#include "PolygonMesh.h"
#include "CustomNavigationData.generated.h"

class FNavMeshGenerator;
class ANavMeshController;

UCLASS()
class NAVMESH_GENERATION_API ACustomNavigationData : public ANavigationData
{
	GENERATED_BODY()

public:
	ACustomNavigationData() {};

	static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	virtual void ConditionalConstructGenerator() override;

	virtual UPrimitiveComponent* ConstructRenderingComponent() override;

	void CreateNavmeshController();

	void SetResultingPoly(TArray<FPolygonData> NavPoly);

	ANavMeshController* GetNavmeshController() const { return NavMeshController; };
	TArray<FPolygonData> GetResultingPoly() const { return ResultingPoly; };

private:
	UPROPERTY()
	ANavMeshController* NavMeshController;

	TArray<FPolygonData> ResultingPoly;
};
