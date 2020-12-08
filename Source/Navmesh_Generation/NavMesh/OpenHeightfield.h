// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseHeightfield.h"
#include "Openspan.h"
#include "OpenHeightfield.generated.h"

#define REGION_MAX_BORDER 10000

class ASolidHeightfield;
class URegion;

UCLASS()
class NAVMESH_GENERATION_API AOpenHeightfield : public ABaseHeightfield
{
	GENERATED_BODY()
	
public:

	AOpenHeightfield();

	// Called every frame
	void Tick(float DeltaTime) override;

	//Initialize the default value for the openfield based on the ones retrieved from the solid
	void Init(const ASolidHeightfield* SolidHeightfield);

	//Detect the open areas in the heighfield and add them to the openspan data container
	void FindOpenSpanData(const ASolidHeightfield* SolidHeightfield);

	//Find and assign the neightbor spans of every span
	//We only need to check and set the axis neighbor as the diagonal ones can be found by checking the axis neighbor of a neighbor span
	//Check the GenerateDistanceField() method to see how this is achieved
	void GenerateNeightborLinks();

	//First part of the distance field generation algorithm use to filter the border span from the others
	void FindBorderSpan();

	//Second part of the distance field generation algorithm use to assign a specific distance value to every span proportioned to its distance from the border
	void GenerateDistanceField();

	//Calculate the minimum and maximum border distance while computing the distance field
	void CalculateBorderDistanceMinMax(const int DistanceToBorder);

	//Apply the watershed algorithm - https://ch.mathworks.com/help/images/marker-controlled-watershed-segmentation.html
	//to identify the regions based on the DistanceToBorderValue
	void GenerateRegions();

	//Find the most appropriate regions to attach spans to after the region has been defined
	void ExpandRegions(TArray<UOpenSpan*>& FloodedSpans, const int MaxIterations);

	//Try creating a new region surrounding a span
	void FloodNewRegion(UOpenSpan* RootSpan, const int FillToDistance, int& RegionID);

	//Make sure small regions are removed or merged into the bigger ones based on the MinUnconnectedRegionSize and MinMergeRegionSize parameters
	void HandleSmallRegions();

	//Initialize all the data fields of the regions based on the data retrieved from the spans
	void GatherRegionsData(TArray<URegion*>& Regions);

	//Find unconnected (island) regions that are below the allowed minimum size and convert them to null regions
	void RemoveSmallUnconnectedRegions(TArray<URegion*>& Regions);

	//Search for small regions to merge with other regions
	void MergeRegions(TArray<URegion*>& Regions);

	//Remap the region and spans ID to make sure they are sequential 
	//Because after the merging it can happen that the region IDs can be not longer sequential
	void RemapRegionAndSpansID(TArray<URegion*>& Regions);

	//Constrain the MinUnconnectedRegionSize and MinMergeRegionSize parameters to be positive value
	void SetMinRegionParameters();

	//Traverse the edge of a region and add all the neighbor connection found to the region connection array
	void FindRegionConnections(UOpenSpan* Span, int NeighborDirection, TArray<int>& RegionConnection);

	void CleanNullRegionBorders();

	void PartialFloodRegion(UOpenSpan* Span, int BorderDirection, int NewRegionID);

	void ResetSpanFlags();

	bool ProcessNullRegion(UOpenSpan* StartSpan, int StartDirection);

	bool ProcessOuterCorner(UOpenSpan* Span, int BorderDirection);

	int SelectedRegionID(UOpenSpan* Span, int BorderDirection, int CornerDirection);

	//Draw the open span data
	void DrawDebugSpanData();

	//Draw neighbor information of a span both axis and diagonal
	//A debug number will appear on top of the neghbor spans if the debug is visible
	//The neighbor spans order is process in clockwise direction
	void DrawSpanNeighbor(UOpenSpan* Span, const bool DebugNumbersVisible);
	
	//Draw the distance field data
	void DrawDistanceFieldDebugData(const bool DebugNumbersVisible, const bool DebugPlanesVisible);

	//Draw the region data
	void DrawDebugRegions(const bool DebugNumbersVisible, const bool DebugPlanesVisible);

protected:
	// Called when the game starts or when spawned
	void BeginPlay() override;

private:
	//Minimum distance from the border based on the data retrieved by looking at the DistanceToBorder value of the single spans
	int MinBorderDistance = 0;

	//Maximum distance from the border based on the data retrieved by looking at the DistanceToBorder value of the single spans
	int MaxBorderDistance = 0;

	//Applies extra algorithms to regions to help prevent poorly formed regions from forming
	bool UseConservativeExpansion = true;

	bool UseOnlyNullSpans = true;

	//Total number of regions
	int RegionCount = 0;

	int MinUnconnectedRegionSize = 10;

	int MinMergeRegionSize = 20;

	//Container of all the openspans contained in the heightfield
	UPROPERTY()
	TMap<int, UOpenSpan*> Spans;
};
