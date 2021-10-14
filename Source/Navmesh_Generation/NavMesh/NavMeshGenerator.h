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
class UDetailedMesh;
class ANavMeshController;
class ACustomNavigationData;

class NAVMESH_GENERATION_API FNavMeshGenerator : public FNavDataGenerator
{
public:	
	//Sets default values for this actor's properties
	FNavMeshGenerator() {};

	virtual bool RebuildAll() override;

	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas);

	//Gather all the valid geometry in the level, meaning the overlapping ones, world static, that can affect navigation
	void GatherValidOverlappingGeometries();

	//Generate the navmesh
	void GenerateNavmesh();

	//Initialize all the UObject needed for creating the navmesh
	void InitializeNavmeshObjects();

	//Create the solid heightfield and return the voxel data
	void CreateSolidHeightfield(const UStaticMeshComponent* Mesh);

	//Create an open heightfield based on the data retrieved from the solid one and return it
	void CreateOpenHeightfield();

	//Create the contours that define the traversable area of the geometries
	void CreateContour();

	//Create the polygons forming the navmesh using the contours data
	void CreatePolygonMesh();

	//Create a polygon mesh with detailed height information
	void CreateDetailedMesh();

	//Pass the polygon data from the generator to the navmesh
	void SendDataToNavmesh();

	void SetNavmesh(ACustomNavigationData* NavMesh);
	void SetNavBounds();

	const FBox GetNavBounds() const { return NavBounds; }
	USolidHeightfield* GetSolidHeightfield() const { return SolidHF; }
	UOpenHeightfield* GetOpenHeightfield() const { return OpenHF; }
	UContour* GetContour() const { return Contour; }
	UPolygonMesh* GetPolygonMesh() const { return PolygonMesh; }
	UDetailedMesh* GetDetailedMesh() const { return DetailedMesh; }

private:
	FBox NavBounds;
	TArray<UStaticMeshComponent*> Geometries;
	ACustomNavigationData* NavigationMesh;
	
	//The pointer to the objects are saved to access the debug functions located in the controller
	USolidHeightfield* SolidHF;
	UOpenHeightfield* OpenHF;
	UContour* Contour;
	UPolygonMesh* PolygonMesh;
	UDetailedMesh* DetailedMesh;
};
