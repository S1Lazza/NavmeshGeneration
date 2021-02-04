// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseHeightfield.generated.h"


UCLASS(Abstract)
class NAVMESH_GENERATION_API ABaseHeightfield : public AActor
{
	GENERATED_BODY()

public:

	ABaseHeightfield();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Calculated width, depth, height of the heightfield based on the cellsize and cellheight passed in
	void CalculateWidthDepthHeight();

	//Retrieve the grid index of a specific voxel based on its width and depth
	int GetGridIndex(const int WidthIndex, const int DepthIndex);

	//Retrieve the adjacent grid location to a cell width 
	int GetDirOffSetWidth(const int Direction);

	//Retrieve the adjacent grid location to a cell depth 
	int GetDirOffSetDepth(const int Direction);

	//Width of the solid heightfield in voxels
	int Width;

	//Depth of the solid heightfield in voxels
	int Depth;

	//Height of the solid heightfield in voxels
	int Height;

	//Size of the single cells (voxels) in which the heightfiels is subdivided, the cells are squared
	float CellSize = 40.f;

	//Height of the single cells (voxels) in which the heightfiels is subdivided
	float CellHeight = 40.f;

	//Min coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMin;

	//Max coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMax;

	//Represent the maximum slope angle (in degree) that is considered traversable
	//Cells that pass the value specified are flagged as UNWALKABLE
	float MaxTraversableAngle = 45.f;

	//Represent the minimum height distance between 2 spans (min of the upper and max of the lower one)
	//that allow for the min one to be still considered walkable
	float MinTraversableHeight = 100.f;

	//Represents the maximum ledge height that is considered to be still traversable
	float MaxTraversableStep = 50.f;

	//The amount of smoothing to be performed when generating the distance field
	int SmoothingThreshold = 2;

	//Closest distance any part of a mesh can get to an obstruction in the source geometry
	int TraversableAreaBorderSize = 1;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
