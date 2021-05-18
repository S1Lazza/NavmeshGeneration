// Fill out your copyright notice in the Description page of Project Settings.


#include "OpenHeightfield.h"
#include "SolidHeightfield.h"
#include "NavMeshController.h"
#include "Engine/TextRenderActor.h"
#include "Region.h"
#include "../Utility/MeshDebug.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Utility/UtilityDebug.h"

void UOpenHeightfield::InitializeParameters(const USolidHeightfield* SolidHeightfield, const ANavMeshController* NavController)
{
	BoundMin = SolidHeightfield->GetBoundMin();
	BoundMax = SolidHeightfield->GetBoundMax();

	CurrentWorld = NavController->GetWorld();
	CellSize = NavController->CellSize;
	CellHeight = NavController->CellHeight;
	MinTraversableHeight = NavController->MinTraversableHeight;
	MaxTraversableStep = NavController->MaxTraversableStep;

	TraversableAreaBorderSize = NavController->TraversableAreaBorderSize;
	MinMergeRegionSize = NavController->MinMergeRegionSize;
	MinUnconnectedRegionSize = NavController->MinUnconnectedRegionSize;
	PerformFullGeneration = NavController->PerformFullGeneration;
	UseConservativeExpansion = NavController->UseConservativeExpansion;

	CalculateWidthDepthHeight();
}

void UOpenHeightfield::FindOpenSpanData(const USolidHeightfield* SolidHeightfield)
{
	if (!SolidHeightfield)
	{
		return;
	}

	//Iterate through the span of the solid heightfield
	for (auto& It : SolidHeightfield->GetSpans())
	{
		UHeightSpan* CurrentSpan = It.Value;

		UOpenSpan* BaseSpan = nullptr;
		UOpenSpan* PreviousSpan = nullptr;

		//As long as there's a span valid in the column considered
		while (CurrentSpan)
		{
			//Check if it's walkable, if not skip to the next one in the column
			if (CurrentSpan->SpanAttribute == PolygonType::UNWALKABLE)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			//Determine the open space between this span and the next higher span
			int Floor = CurrentSpan->Max;
			int Ceiling;

			if (CurrentSpan->nextSpan)
			{
				Ceiling = CurrentSpan->nextSpan->Min;
			}
			else
			{
				Ceiling = INT_MAX;
			}

			//And check if it can be traversed or not
			if ((Ceiling - Floor) * CellHeight < MinTraversableHeight)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			UOpenSpan* NewSpan = NewObject<UOpenSpan>(UOpenSpan::StaticClass());
			NewSpan->Width = CurrentSpan->Width;
			NewSpan->Depth = CurrentSpan->Depth;
			NewSpan->Min = Floor;
			NewSpan->Max = Ceiling;

			//Create a new span and add it to the array of open span if it's the first in the column
			if (!BaseSpan)
			{
				BaseSpan = NewSpan;
			}

			if (PreviousSpan)
			{
				//Otherwise there is a span at a lower location, link it (previous) span to this span
				PreviousSpan->nextSpan = NewSpan;
			}
			
			//Progress in scaling the column by updating the values of the previous and current span until the loop is complete
			PreviousSpan = NewSpan;
			CurrentSpan = CurrentSpan->nextSpan;
		}

		if (BaseSpan)
		{
			Spans.Add(It.Key, BaseSpan);
		}
	}
}

void UOpenHeightfield::GenerateNeightborLinks()
{
	//Iterate through all the base span
	for (auto& Span : Spans)
	{
		//As long as the current span has a next span valid
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			//Find the neightbor span in the same way done in the solid heightfield
			for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
			{
				int NeighborWidth = CurrentSpan->Width + GetDirOffSetWidth(NeighborDir);
				int NeighborDepth = CurrentSpan->Depth + GetDirOffSetDepth(NeighborDir);

				int NeightborIndex = GetGridIndex(NeighborWidth, NeighborDepth);

				//If the neightbor span is valid
				if (Spans.Contains(NeightborIndex))
				{
					UOpenSpan* NeighborSpan = *Spans.Find(NeightborIndex);

					do {

						//Check if the different in height between 2 neightbor spans is large enough for an agent to pass through
						int MaxFloor = FMath::Max(CurrentSpan->Min, NeighborSpan->Min);
						int MinCeiling = FMath::Min(CurrentSpan->Max, NeighborSpan->Max);

						if ((MinCeiling - MaxFloor) * CellHeight >= MinTraversableHeight && UKismetMathLibrary::Abs(NeighborSpan->Min - CurrentSpan->Min) * CellHeight <= MaxTraversableStep)
						{
							//And if it is set the neighbor span and exit the loop
							CurrentSpan->SetAxisNeighbor(NeighborDir, NeighborSpan);
							break;
						}

						//Iterate through the span of a neighbor column as there could be multiple span in it and the first one found could not be valid
						NeighborSpan = NeighborSpan->nextSpan;
					} 
					while (NeighborSpan);
				}
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void UOpenHeightfield::FindBorderSpan()
{
	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			bool IsBorder = false;

			//Iterate through the neighbor span, both axis and diagonal ones (8 neighbor surrounding the one considered)
			for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
			{
				UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);
				
				//If one axis is not valid, then we are sure the span we are considering is a border span and we can exit the loop
				if (!NeighborSpan)
				{
					IsBorder = true;
					break;
				}
				
				//Same for the neighbor spans
				UOpenSpan* DiagonalNeightborSpan = NeighborSpan->GetDiagonalNeighbor(NeighborDir);

				if (!DiagonalNeightborSpan)
				{
					IsBorder = true;
					break;
				}
			}

			//Based on the data found, the DistanceToBorder variable is set to 0, if it's a border span
			//or to a default value which indicates that the current distance of the span is unknown
			if (IsBorder)
			{
				CurrentSpan->DistanceToBorder = 0;
			}
			else
			{
				CurrentSpan->DistanceToBorder = REGION_MAX_BORDER;
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void UOpenHeightfield::GenerateDistanceField()
{
	FindBorderSpan();

	//Iterate through all the spans to make sure they are iterated upon in order
	
	//This could also be achieved by using a sorted map instead of an unsorted one, but 
	//1 - because only this passage step of the algorithm requires this general loop and the overhead of a sorted map can become quite considerable
	//2 - because there is no way in Unreal to easily reverse a map (needed in the second part of the algorithm)
	//This solution is preferred
	for (int DepthIndex = 0; DepthIndex <= Depth; DepthIndex++)
	{
		for (int WidthIndex = 0; WidthIndex <= Width; ++WidthIndex)
		{
			int GridIndex = GetGridIndex(WidthIndex, DepthIndex);
			
			//Check if the span is contained in the map of valid open span
			if (Spans.Contains(GridIndex))
			{
				UOpenSpan* CurrentSpan = *Spans.Find(GridIndex);

				do
				{
					int Distance = CurrentSpan->DistanceToBorder;

					//If the distance is equal to 0 the span is a border span and it can be skipped
					if (Distance == 0)
					{
						CurrentSpan = CurrentSpan->nextSpan;
						continue;
					}

					//Create an array an put in it all the value retrieved from the neightbor span both axis
					TArray<int> TempContainer;

					TempContainer.Add(CurrentSpan->GetAxisNeighbor(0)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(1)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(2)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(3)->DistanceToBorder);


					//And diagonal ones (for these an additional check for their validity is required)
					//To check the diagonal one we use the axis neighbor of a neighbor span
					for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
					{
						UOpenSpan* NeightborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);
						UOpenSpan* DiagonalNeightborSpan = CurrentSpan->GetDiagonalNeighbor(NeighborDir);

						if (DiagonalNeightborSpan)
						{
							TempContainer.Add(DiagonalNeightborSpan->DistanceToBorder);
						}
					}

					//Find the current min distance of all the neighbor span
					int MinDistance = FMath::Min(TempContainer);

					//Set the current distance equal to the min found + a small amount
					CurrentSpan->DistanceToBorder = MinDistance + 2;

					CurrentSpan = CurrentSpan->nextSpan;
				} while (CurrentSpan);
			}
		}
	}

	//Reverse iteration of the algortihm needed because a single iteration is not enough to establish the exact value of some span distance
	//In this second iteration all the span distance are initiliazed properly with no one having the REGION_MAX_BORDER value
	//Same logic as before, comments above
	for (int DepthIndex = Depth; DepthIndex >= 0; DepthIndex--)
	{
		for (int WidthIndex = Width; WidthIndex >= 0; WidthIndex--)
		{
			int GridIndex = GetGridIndex(WidthIndex, DepthIndex);
			if (Spans.Contains(GridIndex))
			{
				UOpenSpan* CurrentSpan = *Spans.Find(GridIndex);

				do
				{
					int Distance = CurrentSpan->DistanceToBorder;

					if (Distance == 0)
					{
						CurrentSpan = CurrentSpan->nextSpan;
						continue;
					}

					TArray<int> TempContainer;
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(0)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(1)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(2)->DistanceToBorder);
					TempContainer.Add(CurrentSpan->GetAxisNeighbor(3)->DistanceToBorder);

					for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
					{
						UOpenSpan* NeightborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);
						UOpenSpan* DiagonalNeightborSpan = CurrentSpan->GetDiagonalNeighbor(NeighborDir);

						if (DiagonalNeightborSpan)
						{
							TempContainer.Add(DiagonalNeightborSpan->DistanceToBorder);
						}
					}
					
					int MinDistance = FMath::Min(TempContainer);

					CurrentSpan->DistanceToBorder = MinDistance + 2;
					
					//At this point of the algorithm, after having established all the value of the single spans
				    //Find the min and max distance from the border
					CalculateBorderDistanceMinMax(CurrentSpan->DistanceToBorder);

					CurrentSpan = CurrentSpan->nextSpan;
				} while (CurrentSpan);
			}
		}
	}
}

void UOpenHeightfield::CalculateBorderDistanceMinMax(const int DistanceToBorder)
{
	MinBorderDistance = FMath::Min(MinBorderDistance, DistanceToBorder);
	MaxBorderDistance = FMath::Max(MaxBorderDistance, DistanceToBorder);
}

void UOpenHeightfield::GenerateRegions()
{
	int MinDist = TraversableAreaBorderSize + MinBorderDistance;
	int CurrentDist = MaxBorderDistance;

	//TODO - Check what is this magic iteration number
	int ExpandIterations = 4 + (TraversableAreaBorderSize * 2);
	
	//0 is the NULL_REGION, therefore the count starts from 1
	int NextRegionID = 1;

	TArray<UOpenSpan*> FloodedSpans;
	TArray<UOpenSpan*> WorkingStack;

	while (CurrentDist > MinDist)
	{
		for (auto& Span : Spans)
		{
			UOpenSpan* CurrentSpan = Span.Value;

			do
			{
				//If a span is unassigned and its distance from border is greater than the current distance considered
				//Add it to the array of the span to process
				if (CurrentSpan->RegionID == NULL_REGION && CurrentSpan->DistanceToBorder >= CurrentDist)
				{
					FloodedSpans.Add(CurrentSpan);
				}

				CurrentSpan = CurrentSpan->nextSpan;
			} 
			while (CurrentSpan);
		}

		//After a region has been created, iterate through the current spans to flood, try to check if they can be added to the new region
		if (NextRegionID > 1)
		{
			if (CurrentDist > 0)
			{
				ExpandRegions(FloodedSpans, ExpandIterations);
			}
			else
			{
				ExpandRegions(FloodedSpans, -1);
			}
		}

		//Try creating a new region for all the spans that couldn't be added to existing regions or if it's the first iteration of the loop
		for (UOpenSpan*& CurrentSpan : FloodedSpans)
		{
			if (!CurrentSpan || CurrentSpan->RegionID != 0)
			{
				continue;
			}

			int FillTo = FMath::Max(CurrentDist - 2, MinDist);
			FloodNewRegion(CurrentSpan, FillTo, NextRegionID);
		}

		//Decrease the distance considered (water level in term of watershed algorithm and repeat the loop)
		CurrentDist = FMath::Max(CurrentDist - 2, MinBorderDistance);
	}


	FloodedSpans.Empty();
	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			if (CurrentSpan->DistanceToBorder >= MinDist && CurrentSpan->RegionID == NULL_REGION)
			{
				FloodedSpans.Add(CurrentSpan);
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}

	if (MinDist > 0)
	{
		ExpandRegions(FloodedSpans, ExpandIterations * 8);
	}
	else
	{
		ExpandRegions(FloodedSpans, -1);
	}

	RegionCount = NextRegionID;
}

void UOpenHeightfield::ExpandRegions(TArray<UOpenSpan*>& FloodedSpans, const int MaxIterations)
{
	int TotalFloodedSpans = FloodedSpans.Num();

	//If there are no spans to flood, return
	if (TotalFloodedSpans == 0)
	{
		return;
	}

	int IterCount = 0;

	while (true)
	{
		int Skipped = 0;

		for (UOpenSpan*& Span : FloodedSpans)
		{
			//The current span considered already possess a non-null region assigned to it, it can be skipped
			if (Span->RegionID != NULL_REGION)
			{
				Skipped++;
				continue;
			}

			int SpanRegion = NULL_REGION;
			int RegionCenterDistance = INT_MAX;

			//Loop through the neighbor 
			for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
			{
				UOpenSpan* NeighborSpan = Span->GetAxisNeighbor(NeighborDir);
				if (!NeighborSpan)
				{
					continue;
				}

				//Neighbor not assigned to null region
				if (NeighborSpan->RegionID > NULL_REGION)
				{
					//Neighbor closer to region core than previously considered neighbors
					if (NeighborSpan->DistanceToRegionCore + 2 < RegionCenterDistance)
					{
						int SameRegionCount = 0;
						if (UseConservativeExpansion)
						{
							//Check if his neighbor has at least two other neighbors in its region.
						    //This makes sure that adding this span to this neighbor's region will not result in a single width line of voxels.
							for (int NNeighborDir = 0; NNeighborDir < 4; NNeighborDir++)
							{
								UOpenSpan* NNeighborSpan = NeighborSpan->GetAxisNeighbor(NNeighborDir);
								if (!NNeighborSpan)
								{
									continue;
								}

								if (NNeighborSpan->RegionID == NeighborSpan->RegionID)
								{
									SameRegionCount++;
								}
							}
						}

						//If the conservative expansion is turned off or the condition considered above is met
						//Set the SpanRegion equal to the neighbor one and set the current distance to center as slightly further than this neighbor
						if (!UseConservativeExpansion || SameRegionCount > 1)
						{
							SpanRegion = NeighborSpan->RegionID;
							RegionCenterDistance = NeighborSpan->DistanceToRegionCore + 2;
						}
					}
				}
			}

			//An appropriate region has been found for the current span, add it to it, otherwise skip it
			if (SpanRegion != NULL_REGION)
			{
				Span->RegionID = SpanRegion;
				Span->DistanceToRegionCore = RegionCenterDistance;
			}
			else
			{
				Skipped++;
			}
		}

		//All spans were already processed or skipped, exit the iteration loop
		if (Skipped == TotalFloodedSpans)
		{
			break;
		}

		//Iteration limit reached, exit the loop
		if (MaxIterations != -1)
		{
			IterCount++;
			if (IterCount > MaxIterations)
			{
				break;
			}
		}
	}
}

void UOpenHeightfield::FloodNewRegion(UOpenSpan* RootSpan, const int FillToDistance, int& RegionID)
{
	int RegionSize = 0;

	//Create a temporary span container and add the span passed in to it
	TArray<UOpenSpan*> TempSpanStack;

	RootSpan->RegionID = RegionID;
	RootSpan->DistanceToRegionCore = 0;
	TempSpanStack.Add(RootSpan);

	while (TempSpanStack.Num() > 0)
	{
		UOpenSpan* CurrentSpan = TempSpanStack[0];

		bool IsOnRegionBorder = false;

		//Neighbor search, both axis and diagonal
		for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
		{
			UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);
			if (!NeighborSpan)
			{
				continue;
			}

			//Current span border the null region or another region, exit the loop
			if (NeighborSpan->RegionID != NULL_REGION && NeighborSpan->RegionID != RegionID)
			{
				IsOnRegionBorder = true;
				break;
			}

			//Same as above + the check on the diagonal neighbor validity
			NeighborSpan = NeighborSpan->GetDiagonalNeighbor(NeighborDir);

			if (NeighborSpan && NeighborSpan->RegionID != NULL_REGION && NeighborSpan->RegionID != RegionID)
			{
				IsOnRegionBorder = true;
				break;
			}
		}

		//If the current span border the null or another region, it can't be part of the new one
		//Remove the span considered from the array
		if (IsOnRegionBorder)
		{
			CurrentSpan->RegionID = NULL_REGION;
			TempSpanStack.RemoveAt(0);
			continue;
		}


		//If the logic arrives here, the new span is part of the new region
		RegionSize++;

		for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
		{
			UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(NeighborDir);

			//Check again the neighbor and if the span is valid, it is not assigned and has a distance greater than the one passed in
			if (NeighborSpan && NeighborSpan->DistanceToBorder >= FillToDistance && NeighborSpan->RegionID == 0)
			{
				NeighborSpan->RegionID = RegionID;
				NeighborSpan->DistanceToRegionCore = 0;
				TempSpanStack.Add(NeighborSpan);
			}
		}

		//Remove the span considered from the array and repeat the loop
		TempSpanStack.RemoveAt(0);
	}

	//Reach this point, if the region size is greater than 0, the new region has been created
	//Increment the region ID to consider
	if (RegionSize > 0)
	{
		RegionID++;
	}
}

void UOpenHeightfield::HandleSmallRegions()
{
	SetMinRegionParameters();

	//Only null region found, no need to execute the code below
	if (RegionCount < 2)
	{
		return;
	}

	//Create an array of empty region object based on the current total of region known
	TArray<URegion*> Regions;
	for (int Iter = 0; Iter < RegionCount; Iter++)
	{
		URegion* NewRegion = NewObject<URegion>(URegion::StaticClass());
		NewRegion->ID = Iter;
		Regions.Add(NewRegion);
	}

	GatherRegionsData(Regions);
	RemoveSmallUnconnectedRegions(Regions);
	MergeRegions(Regions);
	RemapRegionAndSpansID(Regions);
}

void UOpenHeightfield::GatherRegionsData(TArray<URegion*>& Regions)
{
	//Iterate through the spans
	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			//If a spans belongs to the null region, skip it
			if (CurrentSpan->RegionID == NULL_REGION)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			//Increase the current span count of the region the span belongs to
			URegion* Region = Regions[CurrentSpan->RegionID];
			Region->SpanCount++;

			UOpenSpan* NextSpan = CurrentSpan->nextSpan;

			while (NextSpan)
			{
				//If a spans belongs to the null region, skip it
				if (NextSpan->RegionID == NULL_REGION)
				{
					NextSpan = NextSpan->nextSpan;
					continue;
				}

				//If a undetected region is found above the current one, add it to the overlapping array
				if (!Region->OverlappingRegions.Contains(NextSpan->RegionID))
				{
					Region->OverlappingRegions.Add(NextSpan->RegionID);
				}

				NextSpan = NextSpan->nextSpan;
			}

			//The connection for this region has been already found, skip the rest of the code
			//To understand why this condirtion is applied, check the method below FindRegionConnections
			if (Region->Connections.Num() > 0)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			//If the span is on the region edge
			//Get the direction of the edge and proceeed to find all the region connections
			int EdgeDirection = CurrentSpan->GetRegionEdgeDirection();
			if (EdgeDirection != -1)
			{
				FindRegionConnections(CurrentSpan, EdgeDirection, Region->Connections);
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void UOpenHeightfield::RemoveSmallUnconnectedRegions(TArray<URegion*>& Regions)
{
	for (int RegionID = 1; RegionID < RegionCount; RegionID++)
	{
		URegion* Region = Regions[RegionID];

		//Skip empty regions
		if (Region->SpanCount == 0)
		{
			continue;
		}

		//If a region is only connected to the null one, check its number of spans
		//If it is lower than the minimum amount specified, make the region a null region
		if (Region->Connections.Num() == 1 && Region->Connections[NULL_REGION] == NULL_REGION)
		{
			if (Region->SpanCount < MinUnconnectedRegionSize)
			{
				Region->ResetID(0);
			}
		}
	}
}

void UOpenHeightfield::MergeRegions(TArray<URegion*>& Regions)
{
	int MergeCount;

	do
	{
		MergeCount = 0;

		for (URegion* Region : Regions)
		{
			//Skip null, empty and greater than minimum amount specified regions
			if (Region->ID == NULL_REGION || Region->SpanCount == 0 || Region->SpanCount > MinMergeRegionSize)
			{
				continue;
			}

			URegion* TargetMergeRegion = nullptr;
			int SmallestSizeFound = INT_MAX;

			//Search for the smallest neighbor region to the one considered
			for (int RegionID : Region->Connections)
			{
				//Skip null regions
				if (RegionID == NULL_REGION || RegionID == Region->ID)
				{
					continue;
				}

				//If the region found is the smallest found, set it as target for the merging
				URegion* RegionN = Regions[RegionID];
				if (RegionN->SpanCount < SmallestSizeFound && Region->CanRegionBeMergedWith(RegionN))
				{
					TargetMergeRegion = RegionN;
					SmallestSizeFound = RegionN->SpanCount;
				}
			}

			//If a target is found, try the merge
			if (TargetMergeRegion && Region->PerformRegionMergingIn(TargetMergeRegion))
			{
				// A successful merge took place.
				// Discard the old region
				int OldRegionID = Region->ID;
				Region->ResetID(TargetMergeRegion->ID);

				//And replace it with the region it was merged into
				for (URegion* R : Regions)
				{
					//Skip null region as usual
					if (R->ID == NULL_REGION)
					{
						continue;
					}

					//Repoint the ID of the other regions to the new one
					if (R->ID == OldRegionID)
					{
						R->ID = TargetMergeRegion->ID;
					}
					else
					{
						R->ReplaceNeighborRegionID(OldRegionID, TargetMergeRegion->ID);
					}
				}
				MergeCount++;
			}
		}
	} while (MergeCount > 0);
}

void UOpenHeightfield::RemapRegionAndSpansID(TArray<URegion*>& Regions)
{
	for (URegion* Region : Regions)
	{
		//Set all non null region as suitable for region remapping
		if (Region->ID > 0)
		{
			Region->IDRemapNeeded = true;
		}
	}

	int CurrentRegionID = 0;
	for (URegion* Region : Regions)
	{
		//Region null or already remapped
		if (!Region->IDRemapNeeded)
		{
			continue;
		}

		CurrentRegionID++;
		int OldID = Region->ID;

		// Search all regions for ones using the old region ID.
		for (URegion* R : Regions)
		{
			// Re-assign the ones using the old ID to the new one
			if (R->ID == OldID)
			{
				R->ID = CurrentRegionID;
				R->IDRemapNeeded = false;
			}
		}
	}

	// Update the number of regions in the field
	//The +1 is to take into consideration the null region
	RegionCount = CurrentRegionID + 1;

	//After having update the region ID, update the span ID
	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			//If the span is a part of the null region skip it
			if (CurrentSpan->RegionID == NULL_REGION)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}
			else
			{
				// Re-map by getting the region object at the old index and assigning its current region id to the span.
				CurrentSpan->RegionID = Regions[CurrentSpan->RegionID]->ID;
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} while (CurrentSpan);
	}
}

void UOpenHeightfield::SetMinRegionParameters()
{
	MinUnconnectedRegionSize = FMath::Max(1, MinUnconnectedRegionSize);
	MinMergeRegionSize = FMath::Max(0, MinMergeRegionSize);
}

void UOpenHeightfield::FindRegionConnections(UOpenSpan* Span, int NeighborDirection, TArray<int>& RegionConnection)
{
	UOpenSpan* CurrentSpan = Span;

	int LastEdgeRegionID = NULL_REGION;

	int Direction = NeighborDirection;

	//Get the neighbor span to the one considered by using the direction passed in and set the LastEdgeRegionID equal to it
	UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(NeighborDirection);
	if (NeighborSpan)
	{
		LastEdgeRegionID = NeighborSpan->RegionID;
	}

	RegionConnection.Add(LastEdgeRegionID);

	//Starting from the current span, loop through all the border spans of a region until the loop goes back to the beginning
	int LoopCount = 0;
	while (LoopCount < UINT16_MAX)
	{
		NeighborSpan = CurrentSpan->GetAxisNeighbor(Direction);
		int CurrentEdgeRegion = NULL_REGION;

		//The neighbor span considered is at the region edge
		if (!NeighborSpan || NeighborSpan->RegionID != CurrentSpan->RegionID)
		{
			//If valid replace the current edge value with the one of the neighbor
			if (NeighborSpan)
			{
				CurrentEdgeRegion = NeighborSpan->RegionID;
			}

			//If the current edge is different from the last edge saved, then a new connections is found
			//Add it to the connection array and update the last edge value
			if (CurrentEdgeRegion != LastEdgeRegionID)
			{
				RegionConnection.Add(CurrentEdgeRegion);
				LastEdgeRegionID = CurrentEdgeRegion;
			}

			//Update the direction value, rotating through the neighbor of the span considered in clockwise direction
			Direction = UOpenSpan::IncreaseNeighborDirection(Direction, 1);
		}
		//The neighbor is in the same region as the current span 
		//Update the current span value and rotate in counterclockwise direction
		else
		{
			CurrentSpan = NeighborSpan;
			Direction = UOpenSpan::DecreaseNeighborDirection(Direction, 1);
		}

		//If the algorithm has returned to the original span, exit the loop
		if (Span == CurrentSpan && NeighborDirection == Direction)
		{
			break;
		}

		LoopCount++;
	}

	//Make sure the first and last region connections found are not the same
	if(RegionConnection.Num() > 1 && RegionConnection[0] == RegionConnection.Last())
	{
		int IndexToRemove = RegionConnection.Num() - 1;
		RegionConnection.RemoveAt(IndexToRemove);
	}
}

void UOpenHeightfield::CleanRegionBorders()
{
	int NextRegionID = RegionCount;

	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			if (CurrentSpan->ProcessedForRegionFixing)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			CurrentSpan->ProcessedForRegionFixing = true;

			UOpenSpan* WorkingSpan;
			int EdgeDirection = -1;

			if (CurrentSpan->RegionID == NULL_REGION)
			{
				EdgeDirection = CurrentSpan->GetNonNullEdgeDirection();
				if (EdgeDirection == -1)
				{
					CurrentSpan = CurrentSpan->nextSpan;
					continue;
				}

				WorkingSpan = CurrentSpan->GetAxisNeighbor(EdgeDirection);
				EdgeDirection = CurrentSpan->IncreaseNeighborDirection(EdgeDirection, 2);
			}
			else if(!UseOnlyNullRegionSpans)
			{
				EdgeDirection = CurrentSpan->GetNullEdgeDirection();
				if (EdgeDirection == -1)
				{
					CurrentSpan = CurrentSpan->nextSpan;
					continue;
				}
				WorkingSpan = CurrentSpan;
			}
			else
			{
				CurrentSpan = CurrentSpan->nextSpan;
				continue;
			}

			bool IsEncompassNullRegion = WorkingSpan->ProcessNullRegion(EdgeDirection);

			if (IsEncompassNullRegion)
			{
				WorkingSpan->PartialFloodRegion(EdgeDirection, NextRegionID);
				NextRegionID++;
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}

	RegionCount = NextRegionID;

	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do 
		{
			CurrentSpan->ProcessedForRegionFixing = false;
			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}

	ReassignBorderSpan();
}

void UOpenHeightfield::ReassignBorderSpan()
{
	bool SpanChanged = true;

	while(SpanChanged) 
	{
		SpanChanged = false;

		//Iterate through all the spans
		for (auto& Span : Spans)
		{
			UOpenSpan* CurrentSpan = Span.Value;

			do
			{
				//Skip the spans in the null region
				if (CurrentSpan->RegionID == NULL_REGION)
				{
					CurrentSpan = CurrentSpan->nextSpan;
					continue;
				}

				//Iterate through the axis spans to the one considered
				for (int Index = 0; Index < 4; Index++)
				{
					UOpenSpan* AdjacentSpan = CurrentSpan->GetAxisNeighbor(Index);
					int PlusOne = CurrentSpan->IncreaseNeighborDirection(Index, 1);
					int MinusOne = CurrentSpan->DecreaseNeighborDirection(Index, 1);

					//Reassign the spans that comply to the following conditions
					//Have more than one adjacent axis span that has a different region ID (different from 0) from the one considered
					//And they have the same region ID
					if (CurrentSpan->RegionID != AdjacentSpan->RegionID && AdjacentSpan->RegionID != NULL &&
					   (AdjacentSpan->RegionID == CurrentSpan->GetAxisNeighbor(PlusOne)->RegionID || AdjacentSpan->RegionID == CurrentSpan->GetAxisNeighbor(MinusOne)->RegionID))
					{
						//Reassign the span id and set the condition to repeat the loop
						CurrentSpan->RegionID = CurrentSpan->GetAxisNeighbor(Index)->RegionID;
						SpanChanged = true;
						break;
					}
				}

				CurrentSpan = CurrentSpan->nextSpan;
			} while (CurrentSpan);
		}
	}
}

void UOpenHeightfield::DrawDebugSpanData()
{
	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		float Offset = 2.f;

		do
		{
			FVector SpanMinCoord = FVector(BoundMin.X + CellSize * CurrentSpan->Width + Offset, BoundMin.Y + CellSize * CurrentSpan->Depth + Offset, BoundMin.Z + CellSize * CurrentSpan->Min);
			FVector SpanMaxCoord = FVector(SpanMinCoord.X + CellSize - Offset, SpanMinCoord.Y + CellSize - Offset, BoundMin.Z + CellSize * (CurrentSpan->Min));

			UUtilityDebug::DrawMinMaxBox(CurrentWorld, SpanMinCoord, SpanMaxCoord, FColor::Blue, 60.0f, 1.5f);

			CurrentSpan = CurrentSpan->nextSpan;
		} while (CurrentSpan);
	}
}

void UOpenHeightfield::DrawSpanNeighbor(UOpenSpan* Span, const bool DebugNumbersVisible)
{
	float CenterOffset = CellSize / 2;
	float Offset = 2.f;
	int NeighborNumber = 0;
	UOpenSpan* CurrentSpan = Span;

	//Draw the current span
	FVector SpanMinCoord = FVector(BoundMin.X + CellSize * CurrentSpan->Width + Offset, BoundMin.Y + CellSize * CurrentSpan->Depth + Offset, BoundMin.Z + CellSize * CurrentSpan->Min);
	FVector SpanMaxCoord = FVector(SpanMinCoord.X + CellSize - Offset, SpanMinCoord.Y + CellSize - Offset, BoundMin.Z + CellSize * (CurrentSpan->Min));
	UUtilityDebug::DrawMinMaxBox(CurrentWorld, SpanMinCoord, SpanMaxCoord, FColor::Red, 20.0f, 0.5f);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//Neighbor search
	for (int Dir = 0; Dir < 4; Dir++)
	{
		//Axis
		UOpenSpan* NeighborSpan = CurrentSpan->GetAxisNeighbor(Dir);
		if (!NeighborSpan)
		{
			continue;
		}

		//Update the data to draw the axis neighbor span
		SpanMinCoord = FVector(BoundMin.X + CellSize * NeighborSpan->Width + Offset, BoundMin.Y + CellSize * NeighborSpan->Depth + Offset, BoundMin.Z + CellSize * NeighborSpan->Min);
		SpanMaxCoord = FVector(SpanMinCoord.X + CellSize - Offset, SpanMinCoord.Y + CellSize - Offset, BoundMin.Z + CellSize * (NeighborSpan->Min));
		UUtilityDebug::DrawMinMaxBox(CurrentWorld, SpanMinCoord, SpanMaxCoord, FColor::Blue, 20.0f, 0.5f);

		FVector SpanCenterCoord;
		if (DebugNumbersVisible)
		{
			SpanCenterCoord = FVector(BoundMin.X + CellSize * NeighborSpan->Width + CenterOffset, BoundMin.Y + CellSize * NeighborSpan->Depth + CenterOffset, BoundMin.Z + CellSize * NeighborSpan->Min + CenterOffset);
			ATextRenderActor* Text = CurrentWorld->SpawnActor<ATextRenderActor>(SpanCenterCoord, FRotator(0.f, 180.f, 0.f), SpawnInfo);
			Text->GetTextRender()->SetText(FString::FromInt(NeighborNumber));
			Text->GetTextRender()->SetTextRenderColor(FColor::Red);
		}

		//Increase the current neighbor number
		NeighborNumber++;

		//Diagonal
		NeighborSpan = NeighborSpan->GetDiagonalNeighbor(Dir);
		if (!NeighborSpan)
		{
			continue;
		}

		//Update the data to draw the diagonal neighbor span
		SpanMinCoord = FVector(BoundMin.X + CellSize * NeighborSpan->Width + Offset, BoundMin.Y + CellSize * NeighborSpan->Depth + Offset, BoundMin.Z + CellSize * NeighborSpan->Min);
		SpanMaxCoord = FVector(SpanMinCoord.X + CellSize - Offset, SpanMinCoord.Y + CellSize - Offset, BoundMin.Z + CellSize * (NeighborSpan->Min));
		UUtilityDebug::DrawMinMaxBox(CurrentWorld, SpanMinCoord, SpanMaxCoord, FColor::Blue, 20.0f, 0.5f);

		if (DebugNumbersVisible)
		{
			SpanCenterCoord = FVector(BoundMin.X + CellSize * NeighborSpan->Width + CenterOffset, BoundMin.Y + CellSize * NeighborSpan->Depth + CenterOffset, BoundMin.Z + CellSize * NeighborSpan->Min + CenterOffset);
			ATextRenderActor* Text2 = CurrentWorld->SpawnActor<ATextRenderActor>(SpanCenterCoord, FRotator(0.f, 180.f, 0.f), SpawnInfo);
			Text2->GetTextRender()->SetText(FString::FromInt(NeighborNumber));
			Text2->GetTextRender()->SetTextRenderColor(FColor::Red);
		}

		//Increase the current neighbor number
		NeighborNumber++;
	}
}

void UOpenHeightfield::DrawDistanceFieldDebugData(const bool DebugNumbersVisible, const bool DebugPlanesVisible)
{
	float CenterOffset = CellSize / 2;

	float OffSet = 0.02;
	float ScalingValue = CellSize / 100 - OffSet;

	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			FVector SpanCenterCoord = FVector(BoundMin.X + CellSize * CurrentSpan->Width + CenterOffset, BoundMin.Y + CellSize * CurrentSpan->Depth + CenterOffset, BoundMin.Z + CellSize * CurrentSpan->Min + CenterOffset);
			
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (DebugNumbersVisible)
			{
				ATextRenderActor* Text = CurrentWorld->SpawnActor<ATextRenderActor>(SpanCenterCoord, FRotator(0.f, 180.f, 0.f), SpawnInfo);
				Text->GetTextRender()->SetText(FString::FromInt(CurrentSpan->DistanceToBorder));
				Text->GetTextRender()->SetTextRenderColor(FColor::Red);
			}

			if (DebugPlanesVisible)
			{
				AMeshDebug* Mesh = CurrentWorld->SpawnActor<AMeshDebug>(SpanCenterCoord, FRotator(0.f, 0.f, 0.f), SpawnInfo);
				Mesh->SetActorScale3D(FVector(ScalingValue, ScalingValue, 0.01f));
				Mesh->SetMaterialColorOnDistance(CurrentSpan->DistanceToBorder, MaxBorderDistance);
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} while (CurrentSpan);
	}
}

void UOpenHeightfield::DrawDebugRegions(const bool DebugNumbersVisible, const bool DebugPlanesVisible)
{
	float CenterOffset = CellSize / 2;

	float OffSet = 0.02;
	float ScalingValue = CellSize / 100 - OffSet;

	for (auto& Span : Spans)
	{
		UOpenSpan* CurrentSpan = Span.Value;

		do
		{
			FVector SpanCenterCoord = FVector(BoundMin.X + CellSize * CurrentSpan->Width + CenterOffset, BoundMin.Y + CellSize * CurrentSpan->Depth + CenterOffset, BoundMin.Z + CellSize * CurrentSpan->Min + CenterOffset);

			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (DebugNumbersVisible)
			{
				ATextRenderActor* Text = CurrentWorld->SpawnActor<ATextRenderActor>(SpanCenterCoord, FRotator(0.f, 180.f, 0.f), SpawnInfo);
				Text->GetTextRender()->SetText(FString::FromInt(CurrentSpan->RegionID));
				Text->GetTextRender()->SetTextRenderColor(FColor::Red);
			}

			if (DebugPlanesVisible)
			{
				AMeshDebug* Mesh = CurrentWorld->SpawnActor<AMeshDebug>(SpanCenterCoord, FRotator(0.f, 0.f, 0.f), SpawnInfo);
				Mesh->SetActorScale3D(FVector(ScalingValue, ScalingValue, 0.01f));
				Mesh->SetMaterialColorOnDistance(CurrentSpan->RegionID, MaxBorderDistance);
			}

			CurrentSpan = CurrentSpan->nextSpan;
		} while (CurrentSpan);
	}
}
