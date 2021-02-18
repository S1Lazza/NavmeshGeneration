// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "AI/NavDataGenerator.h"
#include "Math/Box.h"
#include "AI/Navigation/NavigationTypes.h"

class UBillboardComponent;
class ASolidHeightfield;
class AOpenHeightfield;
class AContour;
class APolygonMesh;
class ACustomNavigationData;


class NAVMESH_GENERATION_API FNavMeshGenerator : public FNavDataGenerator
{
public:	
	//Sets default values for this actor's properties
	FNavMeshGenerator() {};
	FNavMeshGenerator(ACustomNavigationData* InNavmesh);

	virtual bool RebuildAll() override;

	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas);

	//Gather all the valid geometry in the level, meaning the overlapping ones, world static, that can affect navigation
	void GatherValidOverlappingGeometries();

	////Generate the navmesh
	void GenerateNavmesh();

	////Create the solid heightfield and return the voxel data
	void CreateSolidHeightfield(ASolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh);

	////Create an open heightfield based on the data retrieved from the solid one and return it
	void CreateOpenHeightfield(AOpenHeightfield* OpenHeightfield, ASolidHeightfield* SolidHeightfield, bool PerformFullGeneration);

	void CreateContour(AContour* Contour, AOpenHeightfield* OpenHeightfield);

	void CreatePolygonMesh(APolygonMesh* PolyMesh, const AContour* Contour, const AOpenHeightfield* OpenHeightfield);

	void ClearDebugLines(UWorld* CurrentWorld);

	void SetNavmesh(ACustomNavigationData* NavMesh);

private:
	FBox NavBounds;
	TArray<UStaticMeshComponent*> Geometries;
	ACustomNavigationData* NavigationMesh;
	bool EnableDirtyAreasRebuild = false;
};
