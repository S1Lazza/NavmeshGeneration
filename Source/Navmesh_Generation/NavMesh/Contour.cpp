// Fill out your copyright notice in the Description page of Project Settings.


#include "Contour.h"
#include "OpenHeightfield.h"
#include "OpenSpan.h"
#include "../Utility/UtilityDebug.h"

// Sets default values
AContour::AContour()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AContour::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AContour::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AContour::Init(const AOpenHeightfield* OpenHeightfield)
{
	BoundMin = OpenHeightfield->BoundMin;
	BoundMax = OpenHeightfield->BoundMax;
	CellSize = OpenHeightfield->CellSize;
	CellHeight = OpenHeightfield->CellHeight;
}

void AContour::GenerateContour(const AOpenHeightfield* OpenHeightfield)
{
	int DiscardedCountour = 0;
	TArray<FVector> TempRawVertices;
	TArray<FVector> TempSimplifiedVertices;

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

			float Offset = 2.f;

			FVector SpanMinCoord = FVector(BoundMin.X + CellSize * CurrentSpan->Width + Offset, BoundMin.Y + CellSize * CurrentSpan->Depth + Offset, BoundMin.Z + CellSize * CurrentSpan->Min);
			FVector SpanMaxCoord = FVector(SpanMinCoord.X + CellSize - Offset, SpanMinCoord.Y + CellSize - Offset, BoundMin.Z + CellSize * (CurrentSpan->Min));
			UUtilityDebug::DrawMinMaxBox(GetWorld(), SpanMinCoord, SpanMaxCoord, FColor::Red, 20.0f, 0.5f);

			//TODO: Consider array initialization here, instead of resetting them
			RawVertices.Empty();
			SimplifiedVertices.Empty();

			int NeighborDir = CurrentSpan->GetRegionEdgeDirection();

			BuildRawContours(CurrentSpan, CurrentSpan->Width, CurrentSpan->Depth, NeighborDir, TempRawVertices);

			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

void AContour::FindNeighborRegionConnection(const AOpenHeightfield* OpenHeightfield, int& NumberOfContourDiscarded)
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

void AContour::BuildRawContours(UOpenSpan* Span, const int SpanWidth, const int SpanDepth, const int NeighborDir, TArray<FVector>& Vertices)
{
	UOpenSpan* CurrentSpan = Span;
	int Direction = NeighborDir;
	int StartWidth = SpanWidth;
	int StartDepth = SpanDepth;

	//Similar logic to the one applied in the FindRegionConnection method
	//Check the OpenHeightfield class for more details
	int LoopCount = 0;

	while (LoopCount < /*UINT16_MAX*/1)
	{
		if (CurrentSpan->GetNeighborRegionFlag(Direction))
		{			
			int test = 0;
		}

		LoopCount++;
	}
	
}

int AContour::GetCornerHeightIndex(UOpenSpan* Span, const int NeighborDir)
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

