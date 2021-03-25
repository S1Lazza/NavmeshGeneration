// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Contour.h"
#include "PolygonMesh.generated.h"

#define NULL_INDEX -1

class UOpenHeightfield;
class ANavMeshController;

USTRUCT()
struct FContourData
{
	GENERATED_USTRUCT_BODY()

	//Coordinates of the vertices belonging to the contour
	TArray<FVector> Vertices;

	//Internal region ID to indicate the region the vertex belongs to
	int RegionID = 0;
};

USTRUCT()
struct FTriangleData
{
	GENERATED_USTRUCT_BODY()

	//The value corresponding to the vertex index inside the contour data
	int ContourIndex = 0;

	//If true ndicates that the index took into consideration is the middle point of a triangle valid as partition 
	bool IsTriangleCenterIndex = false;
};

USTRUCT()
struct FPolygonMergeData
{
	GENERATED_USTRUCT_BODY()

	//Squared lenght of the edge shared between the polygons
	float EdgeLenghtSqrt = 0.f;

	//The index of the start of the shared edge in polygon A
	int StartingSharedIndexA = 0;

	//The index of the start of the shared edge in polygon B
	int StartingSharedIndexB = 0;
};

USTRUCT()
struct FPolygonData
{
	GENERATED_USTRUCT_BODY()

	//Polygon vertices location
	TArray<FVector> Vertices;

	//Polygon centroid location
	FVector Centroid = FVector(0.f, 0.f, 0.f);

	//List of the polygons that shares an edge with this one
	TArray<FPolygonData> AdjacentPolygonList;

	//Utility data for iterating and searching through the adjacent polygons
	int Index;

	//Pathfinding Info
	//TODO: Move these info outside of this struct as they are part of a different logic and system
	float F = 0.f;

	float G = 0.f;

	float H = 0.f;
};



UCLASS()
class NAVMESH_GENERATION_API UPolygonMesh : public UObject
{
	GENERATED_BODY()
	
public:	
	//Initialize the base data of the polygon mesh using the one retrieved from the Height and Open fields
	void InitializeParameters(const ANavMeshController* NavController);

	//Split the contour vertices based on the region they belong to and return the size of the biggest one
	int SplitContourDataByRegion(const UContour* Contour);

	//Generate the polygon mesh from the contour data provided
	void GeneratePolygonMesh(const UContour* Contour, const bool PerformRecursiveMerging = false, const int NumberOfRecursion = 0);

	//Attempt to triangluate a polygon based on the vertex and indices data provided
	//Return the total number of triangles generate, negative number if the generation fails
	int Triangulate(TArray<FVector>& Vertices, TArray<FTriangleData>& Indices, TArray<int>& Triangles);

	//Merge the valid triangles into larger polygons, only executed if MaxVertexPOerPoly > 3
	void PerformPolygonMerging(TArray<TArray<int>>& PolysIndices, TArray<FVector>& Vertices, int& PolyTotalCount);

	//During the merging can happen that some indices of a polygon refer to vertices that are collinear and not really needed for keeping the polygon shape
	//This indices can be safely removed and an additional merging can be done afterwards to additionally reduce the number of polygons
	//The enabling of this option and the number of times to perform the operation is controlled by the PerformRecursiveMerging and NumberOfRecursion variables
	//Inside the GeneratePolygonMesh method
	void RemoveCollinearIndices(TArray<TArray<int>>& PolysIndices, TArray<FVector>& Vertices);

	//Store the informtation relative to the polygon that can be merged and their shared edge
	void GetPolyMergeInfo(TArray<int>& PolyIndicesA, TArray<int>& PolyIndicesB, TArray<FVector>& Vertices, FPolygonMergeData& MergingInfo);

	//Split the vertices and indices array into data for easier access
	void SplitPolygonData(TArray<FVector>& Vertices, TArray<int>& PolyIndices);

	//Create connections between the adjacent shared edges
	void BuildEdgeAdjacencyData();

	//Find the centroid of the polygons created
	FVector FindPolygonCentroid(TArray<FVector>& Vertices);

	//Sort the vertex order of the merged polygons : https://math.stackexchange.com/questions/978642/how-to-sort-vertices-of-a-polygon-in-counter-clockwise-order
	//This is required because the vertices possess a global indicizationa and the shared vertices between regions
	//Can cause issues during the merging of the polygons because of that -> the new polygon can present a wrong order of the vertices which can led to
	//An incorrect formation of the polygon edges
	void SortPolygonVertexOrder(TArray<FVector>& Vertices, TArray<int>& PolyIndices);

	int GetPolyVertCount(int PolyStartingIndex, TArray<int>& PolygonIndices);

	//Determine if vertex B of a polygon at vertex A lies within the internal angle
	//If not it means that the possible partition to subdvide the polygon is external to the polygon 
	bool LocatedInInternalAngle(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices);

	//Check if the possible partition considered is intersecting one of the pre-existing edge of the polygon
	bool InvalidEdgeIntersection(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices);

	//Use the sum of the condition values retrieved from the LocatedInInternalAngle and InvalidEdgeIntersection methods to determine if a partition 
	//can be considered or not for triangulation
	bool IsValidPartition(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices);

	//Return the double of the signed area (the area of a triangle, if the vertices are listed counterclockwise
	//Or the negative of that area, if the vertices are listed clockwise) of the triangle
	float GetDoubleSignedArea(const FVector A, const FVector B, const FVector C);

	//True if the vertex to test is located to the left of the line
	bool IsVertexLeft(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd);

	//True if the vertex to test is located to the left of the line or collinear with it
	bool IsVertexLeftOrCollinear(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd);

	//True if the vertex to test is located to the right of the line
	bool IsVertexRight(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd);

	//True if the vertex to test is located to the right of the line or collinear with it
	bool IsVertexRightOrCollinear(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd);

	//Get the previous index of an array, making sure is wapped to the end if the index goes negative
	int GetPreviousArrayIndex(const int Index, const int ArraySize);

	//Get the next index of an array, making sure is wapped to the end if the index goes outside range
	int GetNextArrayIndex(const int Index, const int ArraySize);

	//Draw the triangles forming the polymesh
	void DrawDebugPolyMeshTriangles(const TArray<FVector>& Vertices, const TArray<int>& Triangles, const int NumberOfTriangles);

	//Draw the merged polygons of the single contours
	void DrawDebugPolyMeshPolys();

	//Draw the centroid of the polygons froming the navmesh
	void DrawPolygonCentroid();

	TArray<FPolygonData> GetResultingPoly() const { return ResultingPoly; };

private:
	int MaxVertexPerPoly;

	TArray<int> GlobalPolys;

	TArray<FVector> GlobalVertices;

	TArray<FContourData> ContoursData;

	TArray<FPolygonData> ResultingPoly;

	//A pointer to the world has been added for debug purposes to use all the intermediate debug functions present inside the single methods
	//UObjects by default don't have access to the world
	UWorld* CurrentWorld;
};
