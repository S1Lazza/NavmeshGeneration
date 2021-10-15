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
	BoundMin = OpenHeightfield->GetBoundMin();
	RegionCount = OpenHeightfield->GetRegionCount();

	CurrentWorld = NavController->GetWorld();
	CellSize = NavController->CellSize;
	CellHeight = NavController->CellHeight;
	EdgeMaxDeviation = NavController->EdgeMaxDeviation;
	MaxEdgeLenght = NavController->MaxEdgeLenght;
}

void UContour::GenerateContour(const UOpenHeightfield* OpenHeightfield)
{
	int DiscardedCountour = 0;

	FindNeighborRegionConnection(OpenHeightfield, DiscardedCountour);

	for (auto Span : OpenHeightfield->GetSpans())
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
	for (auto Span : OpenHeightfield->GetSpans())
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
			float PosZ = BoundMin.Z + CellHeight * GetCornerHeightIndex(CurrentSpan, Direction);

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

	int VertA = 0;
	while (VertA < SimplifiedVerticesCount)
	{
		//Get the raw index of two consecutive simplified vertices
		int VertB = (VertA + 1) % SimplifiedVerticesCount;
		int RawIndex1 = VerticesSimplified[VertA].RawIndex;
		int RawIndex2 = VerticesSimplified[VertB].RawIndex;
		int VertToTest = (RawIndex1 + 1) % RawVerticesCount;

		float MaxDeviation = 0.f;
		int VertexToInsert = -1;

		//Iterate through all the raw vertices between the range established 
		//Excluding the vertices bordering the null region
		if (VerticesRaw[(VertToTest) % RawVerticesCount].ExternalRegionID == NULL_REGION)
		{
			while (VertToTest != RawIndex2)
			{
				//If the deviation between the current vertex considered and the segment AB is greater than the current value set
				float Deviation = FMath::PointDistToSegment(VerticesRaw[VertToTest].Coordinate, VerticesSimplified[VertA].Coordinate, VerticesSimplified[VertB].Coordinate);

				if (Deviation > MaxDeviation)
				{
					//Assign the vertex as the one to insert into the contour and set the MaxDeviation value
					//The vertex with the max deviation is the one needed
					MaxDeviation = Deviation;
					VertexToInsert = VertToTest;
				}

				VertToTest = (VertToTest + 1) % RawVerticesCount;
			}
		}

		//If the deviation is greater than the EdgeMaxDeviation specified add the vertex to the array
		if (VertexToInsert != -1 && MaxDeviation > EdgeMaxDeviation)
		{
			VerticesSimplified.Insert(VerticesRaw[VertexToInsert], VertA + 1);
			SimplifiedVerticesCount++;
		}
		//Otherwise increase the loop index
		else
		{
			VertA++;
		}
	}
}

void UContour::CheckNullRegionMaxEdge(TArray<FContourVertexData>& VerticesRaw, TArray<FContourVertexData>& VerticesSimplified)
{
	//If the value to compare against is lower than the current cell size return as it will cause an infinite loop
	if (MaxEdgeLenght < CellSize)
	{
		return;
	}

	int RawVerticesCount = VerticesRaw.Num();
	int SimplifiedVerticesCount = VerticesSimplified.Num();

	int VertA = 0;

	//Similar logic to the one used in the ReinsertNullRegionVertices method
	while (VertA < SimplifiedVerticesCount)
	{
		//Get the raw index of two consecutive simplified vertices
		int VertB = (VertA + 1) % SimplifiedVerticesCount;
		int RawIndex1 = VerticesSimplified[VertA].RawIndex;
		int RawIndex2 = VerticesSimplified[VertB].RawIndex;

		int NewVert = -1;
		int VertToTest = (RawIndex1 + 1) % SimplifiedVerticesCount;

		//Check if the vertex to test belongs to the null region
		if (VerticesRaw[(VertToTest) % RawVerticesCount].ExternalRegionID == NULL_REGION)
		{
			//Check if the distance between the limit vertices considered is greater than the the value set
			float DistX = VerticesRaw[RawIndex2].Coordinate.X - VerticesRaw[RawIndex1].Coordinate.X;
			float DistY = VerticesRaw[RawIndex2].Coordinate.Y - VerticesRaw[RawIndex1].Coordinate.Y;

			//If it is set the NewVert value equal to the vertex that is halfway between the 2 considered
			if (DistX * DistX + DistY * DistY > MaxEdgeLenght * MaxEdgeLenght)
			{
				int IndexDistance = (RawIndex2 < RawIndex1) ? RawIndex2 + (RawVerticesCount - RawIndex1) : RawIndex2 - RawIndex1;
				NewVert = (RawIndex1 + IndexDistance / 2) % RawVerticesCount;
			}
		}

		//If the NewVert value is valid, add the vertex to the VerticesSimplified container
		if (NewVert != -1)
		{
			VerticesSimplified.Insert(VerticesRaw[NewVert], VertA + 1);
			SimplifiedVerticesCount++;
		}
		//Otherwise increase the loop
		else
		{
			VertA++;
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

		for (int Index = 0; Index < TempContainer.Num(); Index++)
		{
			/*FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ATextRenderActor* Text = CurrentWorld->SpawnActor<ATextRenderActor>(TempContainer[Index], FRotator(0.f, 180.f, 0.f), SpawnInfo);
			Text->GetTextRender()->SetText(FString::FromInt(Index));
			Text->GetTextRender()->SetTextRenderColor(FColor::Red);*/

			/*DrawDebugSphere(CurrentWorld, TempContainer[Index], 4.f, 2.f, FColor::Red, false, 20.f, 0.f, 2.f);*/
		}

		FColor Color = FColor::MakeRandomColor();

		UUtilityDebug::DrawPolygon(CurrentWorld, TempContainer, Color, 100.0f, 4.0f);
		TempContainer.Empty();
		LoopIndex++;
	}
}

