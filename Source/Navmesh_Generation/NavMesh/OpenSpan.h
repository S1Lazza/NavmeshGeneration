// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OpenSpan.generated.h"

#define NULL_REGION 0

UCLASS()
class NAVMESH_GENERATION_API UOpenSpan : public UObject
{
	GENERATED_BODY()

public:
	UOpenSpan() {};

	UOpenSpan(const int MinHeight, const int MaxHeight);

	//Set the axis neghbor pointers of the span passed in
	void SetAxisNeighbor(const int Direction, UOpenSpan* NeighborSpan);

	//Get the specific axis neighbor of a span based on the direction passed in (the neighbor information are stored clockwise) 
	UOpenSpan* GetAxisNeighbor(const int Direction);

	//Get the specific diagonal neighbor of a span based on the direction passed in (the neighbor information are stored clockwise) 
	UOpenSpan* GetDiagonalNeighbor(const int Direction);

	void GetNeighborRegionIDs(TArray<int>& NeighborIDs);

	//Return the direction of the first neighbor that is contained in a different region
	int GetRegionEdgeDirection();
	
	//Return the direction of the first neighbor in the non null region
	int GetNonNullEdgeDirection();

	//Return the direction of the first neighbor in the null region
	int GetNullEdgeDirection();

	//Width of the span
	int Width = 0;

	//Dpeth of the span
	int Depth = 0;

	//Min height of the span
	int Min = 0;

	//Max height of the span
	int Max = 0;

	//The distance of the current span from a border - distance is retrieved by looking at the neightbor cell, it is not real distance
	int DistanceToBorder = 0;

	//The distance of the current span from the center of the region it belongs to
	int DistanceToRegionCore = 0;

	//Flag to indicate the region the span belongs to, 0 is default value meaning no region is assigned
	int RegionID = NULL_REGION;

	bool ProcessedForRegionFixing = false;

	//Next span in the column (check heightspan to know why the uproperty is used)
	UPROPERTY()
	UOpenSpan* nextSpan;	

	//Neightbor spans
	UPROPERTY()
	UOpenSpan* NeighborConnection0;
	UPROPERTY()
	UOpenSpan* NeighborConnection1;
	UPROPERTY()
	UOpenSpan* NeighborConnection2;
	UPROPERTY()
	UOpenSpan* NeighborConnection3;
};

static int IncreaseNeighborDirection(int Direction, int Increment)
{
	int IncrementDiff;

	Direction += Increment;

	if (Direction > 3)
	{
		IncrementDiff = Direction % 4;
		Direction = IncrementDiff;
	}

	return Direction;
}

//It is assumed that a value greater than 4 would not be passed as decrement
static int DecreaseNeighborDirection(int Direction, int Decrement)
{
	Direction -= Decrement;
	if (Direction < 0)
	{
		Direction += 4;
	}

	return Direction;
}
