// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HeightSpan.h"
#include "BaseHeightfield.h"
#include "SolidHeightfield.generated.h"

class ANavMeshController;

//Enum to specify the side of the box used in the calculation of the clipping algorithm
UENUM()
enum class BoxSide : uint8
{
	LEFT_FACE = 0	UMETA(DisplayName = "LEFT_FACE"),
	RIGHT_FACE		UMETA(DisplayName = "RIGHT_FACE"),
	TOP_FACE		UMETA(DisplayName = "TOP_FACE"),
	BOTTOM_FACE		UMETA(DisplayName = "BOTTOM_FACE"),
	FRONT_FACE		UMETA(DisplayName = "FRONT_FACE"),
	BACK_FACE		UMETA(DisplayName = "BACK_FACE")
};

UCLASS(NotBlueprintable, NotPlaceable)
class NAVMESH_GENERATION_API USolidHeightfield : public UBaseHeightfield
{
	GENERATED_BODY()
	
public:	
	void InitializeParameters(const ANavMeshController* NavController);

	//Calculate the min and max bounds of the field based on the geometry vertices
	void DefineFieldsBounds(const FVector AreaCenter, const FVector AreaExtent);

	//Define the voxel grid based on the geometry data taken by the mesh
	void VoxelizeTriangles(const TArray<FVector>& Vertices, const TArray<int> Indices);

	void FindGeometryHeight(const TArray<FVector>& Vertices, float& MinHeight, float& MaxHeight);

	/*Application of the Sutherland-Hodgman clipping algorithm - https://parksdigital.com/sutherland-hodgeman-polygon-clipping-algorithm.html
	  The iteration through all the sides can be found inside the ClipPolygon method
	  Rotation is not taken into account as the voxels are represented by AABB*/
	void ClipVersusPlane(TArray<FVector>& Vertices, const FVector MinBound, const FVector MaxBound, const BoxSide Side);

	//Clip the polygon vertices passed in against the AABB box of size BoundMin, BoundMax
	//By iterating through all the sides of the box
	void ClipPolygon(TArray<FVector>& Vertices, const FVector MinBound, const FVector MaxBound);

	//Establish if the current vertex is inside the clipping plane specified by the "Side" parameter
	int InsideClippingPlane(const FVector Vertex, const FVector MinBound, const FVector MaxBound, const BoxSide Side);

	//Find the intersection between the line and the plane specified by the "Side" parameter
	FVector FindPlaneIntersection(const FVector V0, const FVector V1, const FVector MinBound, const FVector MaxBound, const BoxSide Side);

	//Filter the walkable polygon from the unwalkable ones by checking against the maximum slope allowed
	PolygonType FilterWalkablePolygon(const TArray<FVector>& Vertices);

	/*Calculation of the up normal, based on the Dihedral angle formula - https://mathworld.wolfram.com/DihedralAngle.html
	  It assumes that one of the plane is perpendicular to the up axis and takes the MaxTraversableAngle as reference angle*/
	void CalculateUpNormal();

	/*Add span data to the heightfield, the new span is either merged into existing spans or a new span is created
	  Return true if the data is successfully added, otherwise false*/
	bool AddSpanData(int WidthIndex, int DepthIndex, int HeightIndexMin, int HeightIndexMax, PolygonType Type);

	//Draw the debug data relative to the spans composing the geometry
	void DrawDebugSpanData();

	//Remove the traversable flag from spans that have another span too close above them specified by MinTraversableHeight
	void MarkLowHeightSpan();

	//Remove the traversable flag from spans that are close to a drop greater than the specified MaxTraversableStep
	void MarkLedgeSpan();

	const FVector GetBoundMin() const { return BoundMin; };
	const FVector GetBoundMax() const { return BoundMax; };
	const TMap<int, UHeightSpan*> GetSpans() const { return Spans; };

private:
	//Represent the plane normal calculated based on the MaxTraversableAngle
	//Every polygon with a up normal less than this value is considered UNWALKABLE
	float UpNormal;

	float MaxTraversableAngle;

	//Container of all the spans contained in the heightfield
	UPROPERTY()
	TMap<int, UHeightSpan*> Spans;
};
