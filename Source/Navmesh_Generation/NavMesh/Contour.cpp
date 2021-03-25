// Fill out your copyright notice in the Description page of Project Settings.


#include "Contour.h"
#include "OpenHeightfield.h"
#include "OpenSpan.h"
#include "NavMeshController.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "../Utility/UtilityDebug.h"

void UContour::InitializeParameters(const UOpenHeightfield* OpenHeightfield, const ANavMeshController* NavController)
{
	CurrentWorld = OpenHeightfield->CurrentWorld;
	BoundMin = OpenHeightfield->BoundMin;
	BoundMax = OpenHeightfield->BoundMax;
	CellSize = OpenHeightfield->CellSize;
	CellHeight = OpenHeightfield->CellHeight;
	RegionCount = OpenHeightfield->RegionCount;

	EdgeMaxDeviation = NavController->EdgeMaxDeviation;
	MaxEdgeLenght = NavController->MaxEdgeLenght;
}

void UContour::GenerateContour(const UOpenHeightfield* OpenHeightfield)
{
	int DiscardedCountour = 0;

	FindNeighborRegionConnection(OpenHeightfield, DiscardedCountour);

	for (auto Span : OpenHeightfield->Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			//If span belongs to the null region or has no neighbor connected to another one, skip it 
			if (CurrentSpan->RegionID == NULL_REGION || !CurrentSpan->CheckNeighborRegionFlag())
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			TArray<FContourVertexData> TempRawVertices;
			TArray<FContourVertexData> TempSimplifiedVertices;

			int NeighborDir = 0;
			bool OnlyNullRegionConnected = true;

			//Make sure the neighbor considered reside in another region, otherwise switch direction until it is
			while (!CurrentSpan->GetNeighborRegionFlag(NeighborDir))
			{
				NeighborDir++;
			}

			BuildRawContours(CurrentSpan, NeighborDir, OnlyNullRegionConnected, TempRawVertices);
			BuildSimplifiedCountour(CurrentSpan->RegionID, OnlyNullRegionConnected, TempRawVertices, TempSimplifiedVertices);

			//Add the vertices from the temporary container to the general one
			for (FContourVertexData Vertex : TempSimplifiedVertices)
			{
				SimplifiedVertices.Add(Vertex);
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void UContour::FindNeighborRegionConnection(const UOpenHeightfield* OpenHeightfield, int& NumberOfContourDiscarded)
{
	for (auto Span : OpenHeightfield->Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do 
		{
			//If the span considered is part of the null region, skip it
			if (CurrentSpan->RegionID == NULL_REGION)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			int DiffNeighborID = 0;

			//Iterate through the neighbor span
			for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
			{
				int NeighborRegionID = NULL_REGION;
				UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);

				if (NeighborSpan)
				{
					NeighborRegionID = NeighborSpan->RegionID;
				}

				//If the current span region ID is different from the neighbor one
				//It means the neighbor is not in the same region and therefore is processable for the contour generation
				if (CurrentSpan->RegionID != NeighborRegionID)
				{
					CurrentSpan->NeighborInDiffRegion[NeighborDir] = true;
					DiffNeighborID++;
				}
			}

			//If all the neighbors are part of a different region, the span considered is an island span
			//No need to process it for the contour generation
			if (DiffNeighborID == 4)
			{
				CurrentSpan->ResetNeighborRegionFlag();
				NumberOfContourDiscarded++;
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void UContour::BuildRawContours(UOpenSpan* Span, const int StartDir, bool& OnlyNullRegionConnection, TArray<FContourVertexData>& VerticesRaw)
{
	int IndexRaw = 0;

	UOpenSpan* CurrentSpan = Span;
	int Direction = StartDir;
	int StartWidth = CurrentSpan->Width;
	int StartDepth = CurrentSpan->Depth;

	//Similar logic to the one applied in the FindRegionConnection method
	//Check the OpenHeightfield class for more details
	int LoopCount = 0;

	while (LoopCount < UINT16_MAX)
	{
		//If the neighbor span does not belong to the same region
		if (CurrentSpan->GetNeighborRegionFlag(Direction))
		{	
			//Based on the span and field data retrieved the X, Y, Z position of the vertex
			float PosX = BoundMin.X + CellSize * StartWidth;
			float PosY = BoundMin.Y + (CellSize * StartDepth + CellSize);
			float PosZ = BoundMin.Z + CellSize * GetCornerHeightIndex(CurrentSpan, Direction);

			//Update the X and Y position based on the current direction
			switch (Direction)
			{
			case 0: PosY -= CellSize; break;
			case 1: PosX += CellSize; PosY -= CellSize; break;
			case 2: PosX += CellSize; break;
			}

			//If the neighbor exist, store its current region ID, otherwise default to 0
			int RegionIDDirection = NULL_REGION;
			UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(Direction);
			if (NeighborSpan)
			{
				RegionIDDirection = NeighborSpan->RegionID;
			}

			//Check if the current contour is bordering a non-null region, if not then is an island contour
			if (RegionIDDirection != NULL_REGION && OnlyNullRegionConnection)
			{
				OnlyNullRegionConnection = false;
			}

			//Initialize a new instance and add the new vertex data to the array
			FContourVertexData Vertex;
			Vertex.Coordinate = FVector(PosX, PosY, PosZ);
			Vertex.ExternalRegionID = RegionIDDirection;
			Vertex.InternalRegionID = CurrentSpan->RegionID;

			Vertex.RawIndex = IndexRaw;
			VerticesRaw.Add(Vertex);

			//Reset the flage of the neighbor processed and increase the direction in a clockwise direction
			CurrentSpan->NeighborInDiffRegion[Direction] = false;
			Direction = UOpenSpan::IncreaseNeighborDirection(Direction, 1);
			IndexRaw++;
		}
		else
		{
			//Set the span to the neighbor one considered
			CurrentSpan = CurrentSpan->GetAxisNeighbor(Direction);

			//Move the wisth and depth to match the new span coordinates
			switch (Direction)
			{
			case 0: StartWidth--; break;
			case 1: StartDepth--; break;
			case 2: StartWidth++; break;
			case 3: StartDepth++; break;
			}

			//Rotate counterclockwise
			Direction = UOpenSpan::IncreaseNeighborDirection(Direction, 3);
		}

		//If the loop is complete and it goes back to the original span, exit
		if (CurrentSpan == Span && Direction == StartDir)
		{
			break;
		}

		LoopCount++;
	}
	
}

void UContour::BuildSimplifiedCountour(const int CurrentRegionID, const bool OnlyNullRegionConnection, TArray<FContourVertexData>& VerticesRaw, TArray<FContourVertexData>& VerticesSimplified)
{
	//If the region considered is an island region, store the bottom-left and top-right vertices
	if (OnlyNullRegionConnection)
	{
		FContourVertexData BottomLeft;
		BottomLeft = VerticesRaw[0];

		FContourVertexData TopRight;
		TopRight = VerticesRaw[0];

		//Iterated through all the vertices to find the bottom left and top right ones
		for (auto Vertex : VerticesRaw)
		{
			FVector Pos = Vertex.Coordinate;
			
			if (Pos.X < BottomLeft.Coordinate.X || (Pos.X == BottomLeft.Coordinate.X && Pos.Y < BottomLeft.Coordinate.Y))
			{
				BottomLeft = Vertex;
			}

			if (Pos.X >= TopRight.Coordinate.X || (Pos.X == TopRight.Coordinate.X && Pos.Y > TopRight.Coordinate.Y))
			{
				TopRight = Vertex;
			}
		}

		//Add them to the array of the simplified contour
		VerticesSimplified.Add(BottomLeft);
		VerticesSimplified.Add(TopRight);
	}

	//Otherwise store the portal vertices, the ones that are located where a region switch happen 
	else
	{
		for (int Index = 0; Index < VerticesRaw.Num(); Index++)
		{
			int Region1 = VerticesRaw[Index].ExternalRegionID;
			
			int NextIndex = (Index + 1) % VerticesRaw.Num();

			int Region2 = VerticesRaw[NextIndex].ExternalRegionID;

			//Check two consecutive vertices, if their region bordering differs, they are portal vertices and must be added to the array
			if (Region1 != Region2)
			{
				VerticesSimplified.Add(VerticesRaw[Index]);
			}
		}
	}

	//No point in executing additional calculations if there are no vertices to read the data from or to compare to
	int RawVerticesCount = VerticesRaw.Num();
	int SimplifiedVerticesCount = VerticesSimplified.Num();

	if (RawVerticesCount == 0 || SimplifiedVerticesCount == 0)
	{
		return;
	}

	ReinsertNullRegionVertices(VerticesRaw, VerticesSimplified);
	CheckNullRegionMaxEdge(VerticesRaw, VerticesSimplified);
	RemoveDuplicatesVertices(VerticesSimplified);
}

void UContour::ReinsertNullRegionVertices(TArray<FContourVertexData>& VerticesRaw, TArray<FContourVertexData>& VerticesSimplified)
{
	int RawVerticesCount = VerticesRaw.Num();
	int SimplifiedVerticesCount = VerticesSimplified.Num();

	//Iterate through all the vertices, checking one edge at time
	for (int MandatoryIndex1 = 0; MandatoryIndex1 < SimplifiedVerticesCount; MandatoryIndex1++)
	{
		//The mandatory indexes represent the vertex locations forming the edge to check against inside the simplified array data
		//The raw indexes represent the vertex locations in between the edge created by the simplyfed array data
		int MandatoryIndex2 = (MandatoryIndex1 + 1) % SimplifiedVerticesCount;
		int RawIndex1 = VerticesSimplified[MandatoryIndex1].RawIndex;
		int RawIndex2 = VerticesSimplified[MandatoryIndex2].RawIndex;

		//Check to make sure the last point is correctly skipped, preventing the code to reach the while loop and possibly create an infinite loop
		int IndexToCheck = 0;
		if (RawIndex2 == 0)
		{
			IndexToCheck = RawVerticesCount - 1;
		}
		else
		{
			IndexToCheck = RawIndex2;
		}

		//Condition to check the vertices to skip, the ones that
		// 1 - have the same region ID of the final point of the edge considered
		// 2- the region ID is different from the null region
		//For vertices that represents portals between non null region, there is no calculation to apply and all the raw vertices are discarded
		if (VerticesRaw[(RawIndex1 + 1) % RawVerticesCount].ExternalRegionID == VerticesRaw[IndexToCheck].ExternalRegionID &&
			VerticesRaw[(RawIndex1 + 1) % RawVerticesCount].ExternalRegionID != NULL_REGION)
		{
			continue;
		}

		int IndexTestVertex = (RawIndex1 + 1) % RawVerticesCount;

		//As long as the current test vertex index is not equal to the second raw index, continue looping
		while (IndexTestVertex != RawIndex2)
		{
			//Find the distance between the edge and the vertex considered
			/*float Deviation = FMath::PointDistToSegment(VerticesRaw[IndexTestVertex].Coordinate, VerticesSimplified[MandatoryIndex1].Coordinate, VerticesSimplified[MandatoryIndex2].Coordinate);*/

			//According to the algorithm using the distance between the raw point considered and the segment should do the trick
			//However I noticed that, in some cases this does not work (when you have a straight line of point equidistant from the raw one)
			//If one is not valid all of them are not valid and therefore the line is removed
			//For this reason the solution below is preferred
			FVector EdgeMidpoint = (VerticesSimplified[MandatoryIndex1].Coordinate + VerticesSimplified[MandatoryIndex2].Coordinate) * 0.5f;
			float Deviation = FVector::DistSquared(VerticesRaw[IndexTestVertex].Coordinate, EdgeMidpoint);

			//If greater than the value established
			if (Deviation >= (EdgeMaxDeviation * EdgeMaxDeviation))
			{
				//Insert the new vertex in between, making sure to update all the needed value 
				//(the total vertices in the array, the vertices forming the edge to check against and the current index to test)
				VerticesSimplified.Insert(VerticesRaw[IndexTestVertex], MandatoryIndex1 + 1);
				SimplifiedVerticesCount++;
				MandatoryIndex1++;
				MandatoryIndex2 = (MandatoryIndex2 + 1) % SimplifiedVerticesCount;
				IndexTestVertex = (IndexTestVertex + 1) % RawVerticesCount;
			}
			else
			{
				//Otherwise simply increase the index of the test one
				IndexTestVertex = (IndexTestVertex + 1) % RawVerticesCount;
			}
		}

	}
}

void UContour::CheckNullRegionMaxEdge(TArray<FContourVertexData>& VerticesRaw, TArray<FContourVertexData>& VerticesSimplified)
{
	//If the value to compare against is 0 or less there is no point in running the code
	if (MaxEdgeLenght <= 0)
	{
		return;
	}

	int RawVerticesCount = VerticesRaw.Num();
	int SimplifiedVerticesCount = VerticesSimplified.Num();

	//Similar logic to the ReinsertNullRegionVertices method, taking into consideration an edge of two consecutive vertices bordering the null region
	for (int MandatoryVertex1 = 0; MandatoryVertex1 < SimplifiedVerticesCount; MandatoryVertex1++)
	{
		int MandatoryVertex2 = (MandatoryVertex1 + 1) % SimplifiedVerticesCount;

		//Execute the logic only if the final vertices of the edge belongs to the null region
		if (VerticesSimplified[MandatoryVertex2].ExternalRegionID == NULL_REGION)
		{
			float DistanceSqr = FVector::DistSquared(VerticesSimplified[MandatoryVertex1].Coordinate, VerticesSimplified[MandatoryVertex2].Coordinate);

			//Check the length of the edge and, if it's greater than the value established, insert a vertex at its middle point
			while (DistanceSqr > MaxEdgeLenght * MaxEdgeLenght)
			{
				FVector Point1 = VerticesSimplified[MandatoryVertex1].Coordinate;
				FVector Point2 = VerticesSimplified[MandatoryVertex2].Coordinate;

				FVector MiddlePoint = (Point2 - Point1) / 2 + Point1;

				//The new vertex inserted is initialized with the data of the initial vewrtex of the edge
				FContourVertexData NewPoint;
				NewPoint.Coordinate = MiddlePoint;
				NewPoint.ExternalRegionID = VerticesSimplified[MandatoryVertex1].ExternalRegionID;
				NewPoint.InternalRegionID = VerticesSimplified[MandatoryVertex1].InternalRegionID;

				VerticesSimplified.Insert(NewPoint, MandatoryVertex1 + 1);

				//After the insertion, the total number of vertices is updated (needed for the loop) as well as the final vertex of the edge
				SimplifiedVerticesCount++;
				MandatoryVertex2 = (MandatoryVertex1 + 1) % SimplifiedVerticesCount;

				//The loop continues because the new edge created could still be longer than the value used as conditions
				//So the distance is ricalculated before repeating the loop
				DistanceSqr = FVector::DistSquared(VerticesSimplified[MandatoryVertex1].Coordinate, VerticesSimplified[MandatoryVertex2].Coordinate);
			}
		}
	}
}

void UContour::RemoveDuplicatesVertices(TArray<FContourVertexData>& VerticesSimplified)
{
	int SimplifiedVerticesCount = VerticesSimplified.Num();

	for (int Index = 0; Index < SimplifiedVerticesCount; Index++)
	{
		int NextIndex = (Index + 1) % SimplifiedVerticesCount;

		//Check if the locations of two consecutive vertices are equal
		if (VerticesSimplified[Index].Coordinate == VerticesSimplified[NextIndex].Coordinate)
		{
			//If they are one of the vertices can be removed as it's a non needed duplicate
			VerticesSimplified.RemoveAt(NextIndex);
			SimplifiedVerticesCount--;
		}
	}
}

int UContour::GetCornerHeightIndex(UOpenSpan* Span, const int NeighborDir)
{
	//Set the max floor equal to the current span floor
	int MaxFloor = Span->Min;

	UOpenSpan* DiagonalSpan = nullptr;

	//Increment the direction of the neghbor checking in clockwise order
	int DirectionOffset = UOpenSpan::IncreaseNeighborDirection(NeighborDir, 1);

	//Get neighbor, if valid recheck the max floor value with the one of the neighbor span
	UOpenSpan* AxisSpan = Span->GetAxisNeighbor(NeighborDir);
	if (AxisSpan)
	{
		MaxFloor = FMath::Max(MaxFloor, AxisSpan->Min);
		DiagonalSpan = AxisSpan->GetAxisNeighbor(DirectionOffset);
	}

	//Get the following neighbor and, if valid, recheck again the max floor value
	AxisSpan = Span->GetAxisNeighbor(DirectionOffset);
	if (AxisSpan)
	{
		MaxFloor = FMath::Max(MaxFloor, AxisSpan->Min);
		if (!DiagonalSpan)
		{
			DiagonalSpan = AxisSpan->GetAxisNeighbor(NeighborDir);
		}
	}

	//Finally compared the max floor with the diagonal span one
	if (DiagonalSpan)
	{
		MaxFloor = FMath::Max(MaxFloor, DiagonalSpan->Min);
	}

	return MaxFloor;
}

void UContour::DrawRegionContour()
{
	TArray<FVector> TempContainer;
	int LoopIndex = 1;

	while (LoopIndex < RegionCount)
	{
		for (auto Vertex : SimplifiedVertices)
		{
			if (Vertex.InternalRegionID == LoopIndex)
			{
				TempContainer.Add(Vertex.Coordinate);
			}
		}

		//for (int Index = 0; Index < TempContainer.Num(); Index++)
		//{
		//	/*FActorSpawnParameters SpawnInfo;
		//	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		//	ATextRenderActor* Text = CurrentWorld->SpawnActor<ATextRenderActor>(TempContainer[Index], FRotator(0.f, 180.f, 0.f), SpawnInfo);
		//	Text->GetTextRender()->SetText(FString::FromInt(Index));
		//	Text->GetTextRender()->SetTextRenderColor(FColor::Red);*/

		//	DrawDebugSphere(CurrentWorld, TempContainer[Index], 4.f, 2.f, FColor::Red, false, 20.f, 0.f, 2.f);
		//}

		UUtilityDebug::DrawPolygon(CurrentWorld, TempContainer, FColor::Blue, 20.0f, 2.0f);
		TempContainer.Empty();
		LoopIndex++;
	}
}

