// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Contour.generated.h"

class AOpenHeightfield;
class UOpenSpan;

USTRUCT()
struct FContourVertexData
{
	GENERATED_USTRUCT_BODY()

	//Coordinates of the vertex considered
	FVector Coordinate = FVector(0.f, 0.f, 0.f);

	//External region ID to which the vertex is adjacent by/connected to (the one of the neighbor span considered in the processing)
	int ExternalRegionID = 0;

	//Internal region ID to which the vertex is adjacent by/connected to (the one of the current span considered in the processing)
	//Stored for debug purposes
	int InternalRegionID = 0;
};

UCLASS()
class NAVMESH_GENERATION_API AContour : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AContour();

	void Tick(float DeltaTime) override;

	//Initialize the base data of the countour using the one retrieved from the Height and Open fields
	void Init(const AOpenHeightfield* OpenHeightfield);

	//Generate the countour of the region
	void GenerateContour(AOpenHeightfield* OpenHeightfield);

	//TODO: Double check the way in which the region spans are checked, there's code you can reuse
	//Check if the axis neighbor of a specific span belong or not to the same region the span considered is in - the span in the NULL_REGIOn are skipped
	void FindNeighborRegionConnection(const AOpenHeightfield* OpenHeightfield, int& NumberOfContourDiscarded);

	//Build the raw countour of a region, by iterating through all the border spans of it
	void BuildRawContours(UOpenSpan* Span, const int StartDir, TArray<FContourVertexData>& Vertices);

	//Get the highest grid cell value of the corner spans
	int GetCornerHeightIndex(UOpenSpan* Span, const int NeighborDir);

	//Draw the raw contour of the region passed in
	void DrawRegionRawContour(TArray<FContourVertexData>& Vertices, int CurrentRegionID);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	//Min coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMin;

	//Max coordinates of the heightfield derived from the bounds of the navmesh area
	FVector BoundMax;
	
	//Size of the single cells (voxels) in which the heightfiels is subdivided, the cells are squared
	float CellSize;
	
	//Height of the single cells (voxels) in which the heightfiels is subdivided
	float CellHeight;

	//Total number of regions
	int RegionCount;

	//Region ID to which the countour refer to
	int RegionID;

	//Total amount of the detailed vertices
	int RawVerticesCount;

	//Total amount of the simplified vertices 
	int SimplifiedVerticesCount;

	//Vertices representing the detailed contour
	TArray<FContourVertexData> RawVertices;

	//Vertices representing the simplified contour
	TArray<FContourVertexData> SimplifiedVertices;
};
