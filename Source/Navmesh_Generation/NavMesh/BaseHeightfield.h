// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseHeightfield.generated.h"


UCLASS(Abstract)
class NAVMESH_GENERATION_API UBaseHeightfield : public UObject
{
	GENERATED_BODY()

public:
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

	//Min coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMin;

	//Max coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMax;

	float CellSize;

	float CellHeight;

	float MinTraversableHeight;

	float MaxTraversableStep;

	int SmoothingThreshold;

	//A pointer to the world has been added for debug purposes to use all the intermediate debug functions present inside the classes
	//UObjects by default don't have access to the world
	UWorld* CurrentWorld;
};
