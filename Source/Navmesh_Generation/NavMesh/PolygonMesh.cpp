// Fill out your copyright notice in the Description page of Project Settings.


#include "PolygonMesh.h"
#include "OpenHeightfield.h"
#include "../Utility/UtilityDebug.h"
#include "Engine/TextRenderActor.h"
#include "../Utility/UtilityGeneral.h"
#include "Components/TextRenderComponent.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
APolygonMesh::APolygonMesh()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MaxVertexPerPoly = FMath::Max(3, MaxVertexPerPoly);
}

// Called when the game starts or when spawned
void APolygonMesh::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APolygonMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APolygonMesh::Init(const AOpenHeightfield* OpenHF)
{
	BoundMin = OpenHF->BoundMin;
	BoundMax = OpenHF->BoundMax;
	CellSize = OpenHF->CellSize;
	CellHeight = OpenHF->CellHeight;
}

int APolygonMesh::SplitContourDataByRegion(const AContour* Contour)
{
	int LoopIndex = 1;
	int MaxNumberOfVertices = 0;

	//Stay in the loop as long as you have iterated through all the regions
	while (LoopIndex < Contour->RegionCount)
	{
		FContourData NewContourData;
		NewContourData.RegionID = LoopIndex;

		//Iterate through all the vertices
		for (auto Vertex : Contour->SimplifiedVertices)
		{	
			//If the current region ID is equal to the loopindex, increase the number of vertices
			if (Vertex.InternalRegionID == LoopIndex)
			{
				NewContourData.Vertices.Add(Vertex.Coordinate);
			}
		}

		//Update the maximum number of vertices and update the needed values
		MaxNumberOfVertices = FMath::Max(MaxNumberOfVertices, NewContourData.Vertices.Num());
		ContoursData.Add(NewContourData);
		LoopIndex++;
	}

	return MaxNumberOfVertices;
}

void APolygonMesh::GeneratePolygonMesh(const AContour* Contour)
{
	int TotalContourVertices = Contour->SimplifiedVertices.Num();

	if (!Contour || TotalContourVertices == 0 || (TotalContourVertices - 1) >  UINT_MAX)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Invalid contour detected, impossible to generate the polygon mesh"));
		return;
	}

	/*
	Contours processing variables
	*/
	int SourceVertexCount = TotalContourVertices;
	int MaxPossiblePolygons = TotalContourVertices - (Contour->RegionCount * 2);
	int MaxVertexPerContour = SplitContourDataByRegion(Contour);
	int TotalVertexCount = 0;
	TArray<int> ContourToGlobalIndices;
	ContourToGlobalIndices.SetNum(MaxVertexPerContour);

	/*
	Global variables to keep track of the processed contour data, avoiding unecessary duplicates
	*/
	int TotalPolyCount = 0;
	int GlobalPolyIndex = 0;
	TArray<FVector> GlobalVertices;
	TArray<int> GlobalPolys;
	TArray<int> GlobalIndices;
	TArray<int> GlobalRegions;
	GlobalRegions.SetNum(MaxPossiblePolygons);

	/*
	Temporary variables used during the processing and merging of the polygons
	*/
	TArray<FTriangleData> TempIndices;
	TArray<int> TempTriangles;

	//The external array contains the polygons, the internal one the indices relative to that specific polygon
	TArray<TArray<int>> TempPolysIndices;

	FPolygonMergeData PolyMergeInfo;
	TArray<int>MergedPoly;
	MergedPoly.SetNum(MaxVertexPerPoly);

	//Iterate through the contours with valid vertices size
	for (int ContourIndex = 0; ContourIndex < ContoursData.Num(); ContourIndex++)
	{
		if (ContoursData[ContourIndex].Vertices.Num() < 3)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Polygon generation failure: Contour has too few vertices."));
			return;
		}

		//Reset all the temp arrays
		TempPolysIndices.Empty();
		TempTriangles.Empty();
		TempIndices.Empty();

		//Initialize all the indices element to have an array size equal to the vertices one
		for (int Index = 0; Index < ContoursData[ContourIndex].Vertices.Num(); Index++)
		{
			FTriangleData NewData;
			NewData.ContourIndex = Index;
			TempIndices.Add(NewData);
		}

		//Determine the number of triangles generated for the contour, skipping the rest of the process if the generation fails
		int PolyCount = Triangulate(ContoursData[ContourIndex].Vertices, TempIndices, TempTriangles);
		if (PolyCount <= 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Polygon generation failure: Could not triangulate."));
			continue;
		}

		//Utility functions to display the contours and the triangles created
		/*UUtilityDebug::DrawPolygon(GetWorld(), ContoursData[ContourIndex].Vertices, FColor::Red, 20.0f, 1.0f);
		DrawDebugPolyMeshTriangles(ContoursData[ContourIndex].Vertices, TempTriangles, TriangleCount);*/

		//Iterate through all the contour vertices
		for (int ContourVertIndex = 0; ContourVertIndex < ContoursData[ContourIndex].Vertices.Num(); ContourVertIndex++)
		{
			int GlobalVertIndex = -1;

			//Conversion of the contour vertices and indices to global 
			//Check if a vertex has been already added to the array, if not add it to the array and update the needed variables
			if (!GlobalVertices.Find(ContoursData[ContourIndex].Vertices[ContourVertIndex], GlobalVertIndex))
			{
				GlobalVertIndex = TotalVertexCount;
				TotalVertexCount++;

				SetArrayElement(GlobalVertices, ContoursData[ContourIndex].Vertices[ContourVertIndex], GlobalVertIndex);
			}

			//If it's already been added, convert the contour indices to global indices
			ContourToGlobalIndices[ContourVertIndex] = GlobalVertIndex;
		}


		//Initialize the elements of the TempPolysIndices container and add the triangle indices to its sub arrays
		TempPolysIndices.SetNum(PolyCount);
		for (int Index = 0; Index < PolyCount; Index++)
		{
			TempPolysIndices[Index].Add(ContourToGlobalIndices[TempTriangles[Index * 3]]);
			TempPolysIndices[Index].Add(ContourToGlobalIndices[TempTriangles[Index * 3 + 1]]);
			TempPolysIndices[Index].Add(ContourToGlobalIndices[TempTriangles[Index * 3 + 2]]);
		}

		//Execute this code if an additional merging of the polygons is required
		if (MaxVertexPerPoly > 3)
		{
			while (true)
			{
				float LongestMergeEdge = -1.f;
				int BestPolyA = -1;
				int BestPolyB = -1;
				int PolyAVertIndex = -1;
				int PolyBVertIndex = -1;

				//Iterate through all the indices of consecutive polygons, checking if they are mergeable
				for (int PolyIndexA = 0; PolyIndexA < TempPolysIndices.Num() - 1; PolyIndexA++)
				{
					for (int PolyIndexB = PolyIndexA + 1; PolyIndexB < TempPolysIndices.Num(); PolyIndexB++)
					{
						GetPolyMergeInfo(TempPolysIndices[PolyIndexA], TempPolysIndices[PolyIndexB], GlobalVertices, PolyMergeInfo);
						
						//If a mergeable pair is found, store its index and merging data
						//At the end the polygons with the longest merging edge will be saved 
						if (PolyMergeInfo.EdgeLenghtSqrt > LongestMergeEdge)
						{
							LongestMergeEdge = PolyMergeInfo.EdgeLenghtSqrt;
							BestPolyA = PolyIndexA;
							PolyAVertIndex = PolyMergeInfo.StartingSharedIndexA;
							BestPolyB = PolyIndexB;
							PolyBVertIndex = PolyMergeInfo.StartingSharedIndexB;
						}
					}
				}

				//If no merging edge is found, exit the loop
				if (LongestMergeEdge <= 0)
				{
					break;
				}

				//Add to the first valid merging polygon found, the missing indices from the second one
				for (int Index = 0; Index < TempPolysIndices[BestPolyB].Num(); Index++)
				{
					if (!TempPolysIndices[BestPolyA].Contains(TempPolysIndices[BestPolyB][Index]))
					{
						TempPolysIndices[BestPolyA].Add(TempPolysIndices[BestPolyB][Index]);
					}
				}

				//Sort the indices of the poly to make sure they are sequential
				//TODO: Currently there's an issue when a region connected to other 2 is processed after the connected 
				//as the vertices disposition in the contour is not sequential and this create incorrect polygons 
				TempPolysIndices[BestPolyA].Sort();

				//Remove the second polygon indices as they are not needed anymore
				TempPolysIndices.RemoveAt(BestPolyB);

				//Decrease the numbers of polygon
				PolyCount--;
			}
		}

		//Add to the GlobalPolys container the single polygon indices, split by a NULL_INDEX value
		//TODO: To evaluate if this step is needed
		for (int Poly = 0; Poly < TempPolysIndices.Num(); Poly++)
		{
			for (int PolyIndex = 0; PolyIndex < TempPolysIndices[Poly].Num(); PolyIndex++)
			{
				GlobalPolys.Add(TempPolysIndices[Poly][PolyIndex]);
			}

			GlobalPolys.Add(NULL_INDEX);
		}

		TotalPolyCount += PolyCount;
	}

	DrawDebugPolyMeshPolys(GlobalVertices, GlobalPolys);
}

int APolygonMesh::Triangulate(TArray<FVector>& Vertices, TArray<FTriangleData>& Indices, TArray<int>& Triangles)
{
	//The number of triangles is localized to the contour considered, empty it before triangulating a new one
	Triangles.Empty();

	//Iterate through all the indices to check where are the valid partition
	for (int Index = 0; Index < Indices.Num(); Index++)
	{
		int IndexPlus1 = GetNextArrayIndex(Index, Indices.Num());
		int IndexPlus2 = GetNextArrayIndex(IndexPlus1, Indices.Num());

		//When a valid partition is found, flag the middle vertex  
		if (IsValidPartition(Index, IndexPlus2, Vertices, Indices))
		{
			Indices[IndexPlus1].IsTriangleCenterIndex = true;
		}
	}

	//Loop through the vertices creating triangles as long as there are more than 3 indices
	while (Indices.Num() > 3)
	{
		int MinLengthSq = -1;
		int MinLenghtSqVert = -1;

		//Loop through all the indices and find where the shortest partition can be created to form a new triangle
		for (int Index = 0; Index < Indices.Num(); Index++)
		{
			int IndexPlus1 = GetNextArrayIndex(Index, Indices.Num());
			if (Indices[IndexPlus1].IsTriangleCenterIndex)
			{
				int IndexPlus2 = GetNextArrayIndex(IndexPlus1, Indices.Num());

				float DeltaX = Vertices[IndexPlus2].X - Vertices[Index].X;
				float DeltaY = Vertices[IndexPlus2].Y - Vertices[Index].Y;
				float LengthSq = DeltaX * DeltaX + DeltaY * DeltaY;

				if (MinLengthSq < 0 || LengthSq < MinLengthSq)
				{
					MinLengthSq = LengthSq;
					MinLenghtSqVert = Index;
				}
			}
		}
		
		//If no new partition has been found and there are still indices available, the triangulation fails
		//Note: this is highly dependent from region formation, make sure to play around with the EdgeMaxDeviation and MaxEdgeLenght if this happens (Contour)
		//decreasing their value and therefore increasing their precision
		if (MinLenghtSqVert == -1)
		{
			return -(Triangles.Num() / 3);
		}

		//At the index where the shortest partition is found, save the indices of the new triangle
		int Index = MinLenghtSqVert;
		int IndexPlus1 = GetNextArrayIndex(Index, Indices.Num());
		int IndexPlus2 = GetNextArrayIndex(IndexPlus1, Indices.Num());

		Triangles.Add(Indices[Index].ContourIndex);
		Triangles.Add(Indices[IndexPlus1].ContourIndex);
		Triangles.Add(Indices[IndexPlus2].ContourIndex);

		//And remove its center vertex from the Indices array, in this way the part of the polygon forming the partition will be removed
		//avoiding issues and duplicated
		Indices.RemoveAt(IndexPlus1);

		//Because of the removal, makes sure there are no issue with the array index by wrapping their values 
		if (IndexPlus1 == 0 || IndexPlus1 >= Indices.Num())
		{
			Index = Indices.Num() - 1;
			IndexPlus1 = 0;
		}

		//Check if the new indices arragment allows for creating a partition at the Index value and floag it if that's the case
		if (IsValidPartition(GetPreviousArrayIndex(Index, Indices.Num()), IndexPlus1, Vertices, Indices))
		{
			Indices[Index].IsTriangleCenterIndex = true;
		}
		else
		{
			Indices[Index].IsTriangleCenterIndex = false;
		}

		//Same for IndexPlus1
		if (IsValidPartition(Index, GetNextArrayIndex(IndexPlus1, Indices.Num()), Vertices, Indices))
		{
			Indices[IndexPlus1].IsTriangleCenterIndex = true;
		}
		else
		{
			Indices[IndexPlus1].IsTriangleCenterIndex = false;
		}
	}

	//At the end of the loop there will be 3 indices left, add the final triangle to the array and return the total number of triangles found
	Triangles.Add(Indices[0].ContourIndex);
	Triangles.Add(Indices[1].ContourIndex);
	Triangles.Add(Indices[2].ContourIndex);

	return Triangles.Num() / 3;

}

void APolygonMesh::GetPolyMergeInfo(TArray<int>& PolyIndicesA, TArray<int>& PolyIndicesB, TArray<FVector>& Vertices, FPolygonMergeData& MergingInfo)
{
	MergingInfo.EdgeLenghtSqrt = -1.f;
	MergingInfo.StartingSharedIndexA = -1;
	MergingInfo.StartingSharedIndexB = -1;

	int PolyVertACount = PolyIndicesA.Num();
	int PolyVertBCount = PolyIndicesB.Num();

	if (PolyVertACount + PolyVertBCount - 2 > MaxVertexPerPoly)
	{
		return;
	}

	//Iterate through all the polygons indices, checking if a shared edge exist
	for (int PolyVertA = 0; PolyVertA < PolyVertACount; PolyVertA++)
	{
		//Indices of polygon A
		int IndexVertA = PolyIndicesA[PolyVertA];
		int IndexVertAPlus = PolyIndicesA[GetNextArrayIndex(PolyVertA, PolyVertACount)];

		for (int PolyVertB = 0; PolyVertB < PolyVertBCount; PolyVertB++)
		{
			//Indices of polygon B
			int IndexVertB = PolyIndicesB[PolyVertB];
			int IndexVertBPlus = PolyIndicesB[GetNextArrayIndex(PolyVertB, PolyVertBCount)];

			//If a share edge is found, store the indices info
			if (IndexVertA == IndexVertBPlus && IndexVertAPlus == IndexVertB)
			{
				MergingInfo.StartingSharedIndexA = PolyVertA;
				MergingInfo.StartingSharedIndexB = PolyVertB;
			}
		}
	}

	//If no merging info are found, return
	if (MergingInfo.StartingSharedIndexA == -1)
	{
		return;
	}

	//Check if the merged polygon would be convex
	int SharedVertex;
	int SharedVertexPlus;
	int SharedVertexMinus;

	SharedVertexMinus = PolyIndicesA[GetPreviousArrayIndex(MergingInfo.StartingSharedIndexA, PolyVertACount)];
	SharedVertex = PolyIndicesA[MergingInfo.StartingSharedIndexA];
	SharedVertexPlus = PolyIndicesB[((MergingInfo.StartingSharedIndexB + 2) % (PolyVertBCount - 1))];

	//The shared vertex is is not to the left of segment vertMinus->vertPlus, for a clockwise wrapped polygon this indicates a concave section
	if (!IsVertexLeftOrCollinear(Vertices[SharedVertex], Vertices[SharedVertexMinus], Vertices[SharedVertexPlus]))
	{
		return;
	}

	SharedVertexMinus = PolyIndicesB[GetPreviousArrayIndex(MergingInfo.StartingSharedIndexB, PolyVertBCount)];
	SharedVertex = PolyIndicesB[MergingInfo.StartingSharedIndexB];
	SharedVertexPlus = PolyIndicesA[((MergingInfo.StartingSharedIndexA + 2) % (PolyVertACount - 1))];

	//The shared vertex is is not to the left of segment vertMinus->vertPlus, for a clockwise wrapped polygon this indicates a concave section
	if (!IsVertexLeftOrCollinear(Vertices[SharedVertex], Vertices[SharedVertexMinus], Vertices[SharedVertexPlus]))
	{
		return;
	}

	SharedVertexMinus = PolyIndicesA[MergingInfo.StartingSharedIndexA];
	SharedVertex = PolyIndicesA[GetNextArrayIndex(MergingInfo.StartingSharedIndexA, PolyVertACount)];

	//Store the squared length of the shared edge
	float SharedEdgeL = FVector::DistSquared(Vertices[SharedVertexMinus], Vertices[SharedVertex]);
	MergingInfo.EdgeLenghtSqrt = SharedEdgeL;

}

int APolygonMesh::GetPolyVertCount(int PolyStartingIndex, TArray<int>& PolygonIndices)
{
	for (int Index = 0; Index < MaxVertexPerPoly; Index++)
	{
		if (PolygonIndices[PolyStartingIndex + Index] == NULL_INDEX)
		{
			return Index;
		}
	}
	return MaxVertexPerPoly;
}

bool APolygonMesh::LocatedInInternalAngle(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices)
{
	//Get the index values of the vertices of interest 
	int VertAIndex = Indices[IndexA].ContourIndex;
	int VertBIndex = Indices[IndexB].ContourIndex;

	int VertAMinus = Indices[GetPreviousArrayIndex(IndexA, Indices.Num())].ContourIndex;
	int VertAPlus = Indices[GetNextArrayIndex(IndexA, Indices.Num())].ContourIndex;

	//The angle internal to the polygon is <= 180 degrees
	if (IsVertexLeftOrCollinear(Vertices[VertAIndex], Vertices[VertAMinus], Vertices[VertAPlus]))
	{
		//Check if B lies within this angle
		return IsVertexLeft(Vertices[VertBIndex], Vertices[VertAIndex], Vertices[VertAMinus]) &&
			   IsVertexRight(Vertices[VertBIndex], Vertices[VertAIndex], Vertices[VertAPlus]);
	}

	//The angle internal to the polygon is > 180 degrees
	//Test to see if B lies within the external angle and flip the result. 
	//If B lies within the external angle, it can't lie within the internal angle
	return !((IsVertexLeftOrCollinear(Vertices[VertBIndex], Vertices[VertAIndex], Vertices[VertAPlus])) &&
		    (IsVertexRightOrCollinear(Vertices[VertBIndex], Vertices[VertAIndex], Vertices[VertAMinus])));
}

bool APolygonMesh::InvalidEdgeIntersection(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices)
{
	int VertAIndex = Indices[IndexA].ContourIndex;
	int VertBIndex = Indices[IndexB].ContourIndex;

	//Loop through all the edges
	for (int PolyEdgeBegin = 0; PolyEdgeBegin < Indices.Num(); PolyEdgeBegin++)
	{
		int PolyEdgeEnd = GetNextArrayIndex(PolyEdgeBegin, Indices.Num());

		//If the edge considered if different from the indices passed in
		if (!(PolyEdgeBegin == IndexA || PolyEdgeBegin == IndexB || PolyEdgeEnd == IndexA || PolyEdgeEnd == IndexB))
		{
			//Retrieved the indices values of the index
			int EdgeVertBegin = Indices[PolyEdgeBegin].ContourIndex;
			int EdgeVertEnd = Indices[PolyEdgeEnd].ContourIndex;
			
			//One of the test vertices is located on the xy coordinate with one of the endpoints of this edge, skip the edge
			if ((Vertices[EdgeVertBegin].X == Vertices[VertAIndex].X && Vertices[EdgeVertBegin].Y == Vertices[VertAIndex].Y) ||
				(Vertices[EdgeVertBegin].X == Vertices[VertBIndex].X && Vertices[EdgeVertBegin].Y == Vertices[VertBIndex].Y) ||
				(Vertices[EdgeVertEnd].X == Vertices[VertAIndex].X && Vertices[EdgeVertEnd].Y == Vertices[VertAIndex].Y) ||
				(Vertices[EdgeVertEnd].X == Vertices[VertBIndex].X && Vertices[EdgeVertEnd].Y == Vertices[VertBIndex].Y))
			{
				continue;
			}

			//The edge considered is not connected to either of the test vertices
			//If the line segment AB intersects with this edge, then the intersection is illegal
			FVector IntersectionPoint(0.f, 0.f, 0.f);
			if (FMath::SegmentIntersection2D(Vertices[VertAIndex], Vertices[VertBIndex], Vertices[EdgeVertBegin], Vertices[EdgeVertEnd], IntersectionPoint))
			{
				return true;
			}
		}
	}

	return false;
}

bool APolygonMesh::IsValidPartition(int IndexA, int IndexB, TArray<FVector>& Vertices, TArray<FTriangleData>& Indices)
{
	bool Internal = LocatedInInternalAngle(IndexA, IndexB, Vertices, Indices);
	bool Invalid = !InvalidEdgeIntersection(IndexA, IndexB, Vertices, Indices);
	return Internal && Invalid;
}

float APolygonMesh::GetDoubleSignedArea(const FVector A, const FVector B, const FVector C)
{
	//The value returned is negative because the arrangment order of the vertices is clockwise
	return -((B.X - A.X) * (C.Y - A.Y) - (C.X - A.X) * (B.Y - A.Y));
}

bool APolygonMesh::IsVertexLeft(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd)
{
	return GetDoubleSignedArea(LineStart, VertexToTest, LineEnd) < 0;
}

bool APolygonMesh::IsVertexLeftOrCollinear(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd)
{
	return GetDoubleSignedArea(LineStart, VertexToTest, LineEnd) <= 0;
}

bool APolygonMesh::IsVertexRight(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd)
{
	return GetDoubleSignedArea(LineStart, VertexToTest, LineEnd) > 0;
}

bool APolygonMesh::IsVertexRightOrCollinear(const FVector VertexToTest, const FVector LineStart, const FVector LineEnd)
{
	return GetDoubleSignedArea(LineStart, VertexToTest, LineEnd) >= 0;
}

int APolygonMesh::GetPreviousArrayIndex(const int Index, const int ArraySize)
{
	if ((Index - 1) >= 0)
	{
		return Index - 1;
	}
	else
	{
		return ArraySize - 1;
	}
}

int APolygonMesh::GetNextArrayIndex(const int Index, const int ArraySize)
{
	if ((Index + 1) < ArraySize)
	{
		return Index + 1;
	}
	else
	{
		return 0;
	}
}

void APolygonMesh::DrawDebugPolyMeshTriangles(const TArray<FVector>& Vertices, const TArray<int>& Triangles, const int NumberOfTriangles)
{
	int LoopIndex = 0;
	TArray<FVector> TempArray;

	while (LoopIndex < NumberOfTriangles)
	{
		//Add every triangle to a temp array using the indices provided in the Triangles array
		//Draw the triangle, empty the array, increase the loop count and repeat the process
		for (int Index = LoopIndex * 3; Index < (3 + LoopIndex * 3); Index++)
		{
			int TriangleIndex = Triangles[Index];
			TempArray.Add(Vertices[TriangleIndex]);
		}

		UUtilityDebug::DrawPolygon(GetWorld(), TempArray, FColor::Red, 20.0f, 1.0f);
		TempArray.Empty();
		LoopIndex++;
	}
}

void APolygonMesh::DrawDebugPolyMeshPolys(const TArray<FVector>& Vertices, const TArray<int>& Polys)
{
	TArray<FVector> TempArray;
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int Index = 0; Index < Polys.Num(); Index++)
	{
		if (Polys[Index] != NULL_INDEX)
		{
			int VertexIndex = Polys[Index];

			/*ATextRenderActor* Text = GetWorld()->SpawnActor<ATextRenderActor>(Vertices[VertexIndex], FRotator(0.f, 180.f, 0.f), SpawnInfo);
			Text->GetTextRender()->SetText(FString::FromInt(VertexIndex));
			Text->GetTextRender()->SetTextRenderColor(FColor::Red);*/

			TempArray.Add(Vertices[VertexIndex]);
		}
		else
		{
			if (TempArray.Num() > 0)
			{
				UUtilityDebug::DrawPolygon(GetWorld(), TempArray, FColor::Red, 1000.f, 1.f);
				TempArray.Empty();
			}
		}
	}
}

