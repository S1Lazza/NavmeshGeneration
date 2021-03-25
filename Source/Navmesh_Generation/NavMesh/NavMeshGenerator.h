// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "AI/NavDataGenerator.h"
#include "Math/Box.h"
#include "AI/Navigation/NavigationTypes.h"

class USolidHeightfield;
class UOpenHeightfield;
class UContour;
class UPolygonMesh;
class ANavMeshController;
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

	void InitializeNavmeshObjects();

	////Create the solid heightfield and return the voxel data
	void CreateSolidHeightfield(USolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh);

	////Create an open heightfield based on the data retrieved from the solid one and return it
	void CreateOpenHeightfield(UOpenHeightfield* OpenHeightfield, USolidHeightfield* SolidHeightfield, const ANavMeshController* NavController);

	//Create the contours that define the traversable area of the geometries
	void CreateContour(UContour* MeshContour, UOpenHeightfield* OpenHeightfield);

	//Create the polygons forming the navmesh using the contours data
	void CreatePolygonMesh(UPolygonMesh* PolyMesh, const UContour* MeshContour, const UOpenHeightfield* OpenHeightfield);

	void ClearDebugLines(UWorld* CurrentWorld);

	void SetNavmesh(ACustomNavigationData* NavMesh);

	void SetNavBounds(ACustomNavigationData* NavMesh);

	USolidHeightfield* GetSolidHeightfield() { return SolidHF; }
	UOpenHeightfield* GetOpenHeightfield() { return OpenHF; }
	UContour* GetContour() { return Contour; }
	UPolygonMesh* GetPolygonMesh() { return PolygonMesh; }

private:
	FBox NavBounds;
	TArray<UStaticMeshComponent*> Geometries;
	ACustomNavigationData* NavigationMesh;
	USolidHeightfield* SolidHF;
	UOpenHeightfield* OpenHF;
	UContour* Contour;
	UPolygonMesh* PolygonMesh;
};
