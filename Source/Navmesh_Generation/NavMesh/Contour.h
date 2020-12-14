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

	FVector Coordinate = FVector(0.f, 0.f, 0.f);
	int RegionID = 0;
	int SpanID = 0;
};

UCLASS()
class NAVMESH_GENERATION_API AContour : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AContour();

	void Tick(float DeltaTime) override;

	void Init(const AOpenHeightfield* OpenHeightfield);

	void GenerateContour(AOpenHeightfield* OpenHeightfield);

	//TODO: Double check the way in which the region spans are checked, there's code you can reuse
	void FindNeighborRegionConnection(const AOpenHeightfield* OpenHeightfield, int& NumberOfContourDiscarded);

	void BuildRawContours(UOpenSpan* Span, const int StartDir, TArray<FContourVertexData>& Vertices);

	//Get the highest grid cell value of the corner spans
	int GetCornerHeightIndex(UOpenSpan* Span, const int NeighborDir);

	void DrawRegionRawContour(TArray<FContourVertexData>& Vertices, int CurrentRegionID);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FVector BoundMin;

	FVector BoundMax;
	
	float CellSize;
	
	float CellHeight;

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
