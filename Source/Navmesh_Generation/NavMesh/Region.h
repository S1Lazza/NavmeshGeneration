// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Region.generated.h"


UCLASS()
class NAVMESH_GENERATION_API URegion : public UObject
{
	GENERATED_BODY()

public:
	URegion() {};

	//Reset the region ID and empty all the related data
	void ResetID(const int NewRegionID);
	
	//Check if the current region can be merged with another one
	bool CanRegionBeMergedWith(URegion* OtherRegion);

	//Merge the current region into the target one
	bool PerformRegionMergingIn(URegion* TargetRegion);

	//Remove not needed adjacent connections in the Connections array
	void RemoveAdjacentDuplicateConnections();

	//Replace the old connections of a region with new ones
	//This operation is performed during the region merging to remove connection reference to the not-existent region (the one merged)
	void ReplaceNeighborRegionID(int OldID, int NewID);
	
	//ID of the region considered
	int ID = 0;

	//Number of spans belonging to this region
	int SpanCount = 0;

	bool IDRemapNeeded = false;

	//Represents an ordered list of connections between this and other regions
	TArray<int> Connections;

	//Represents an ordered list of non-null regions that overlap this region.
	//An overlap is considered to have occurred if a span in this region is below a span belonging to another region
	TArray<int> OverlappingRegions;
};
