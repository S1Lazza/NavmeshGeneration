// Fill out your copyright notice in the Description page of Project Settings.


#include "OpenSpan.h"

UOpenSpan::UOpenSpan(const int MinHeight, const int MaxHeight)
	:Min(MinHeight),
     Max(MaxHeight)
{
}

void UOpenSpan::SetAxisNeighbor(const int Direction, UOpenSpan* NeighborSpan)
{
    switch (Direction)
    {
    case 0: NeighborConnection0 = NeighborSpan; break;
    case 1: NeighborConnection1 = NeighborSpan; break;
    case 2: NeighborConnection2 = NeighborSpan; break;
    case 3: NeighborConnection3 = NeighborSpan; break;
    }
}

UOpenSpan* UOpenSpan::GetAxisNeighbor(const int Direction)
{
    switch (Direction)
    {
    case 0: return NeighborConnection0;
    case 1: return NeighborConnection1;
    case 2: return NeighborConnection2;
    case 3: return NeighborConnection3;
    default: return nullptr;
    }
}

UOpenSpan* UOpenSpan::GetDiagonalNeighbor(const int Direction)
{
    UOpenSpan* DiagonalNeighborSpan = nullptr;

    if (Direction == 3)
    {
        DiagonalNeighborSpan = GetAxisNeighbor(0);
    }
    else
    {
        DiagonalNeighborSpan = GetAxisNeighbor(Direction + 1);
    }

    return DiagonalNeighborSpan;
}

void UOpenSpan::GetNeighborRegionIDs(TArray<int>& NeighborIDs)
{
    //Initialize all the elements of the array as it has expected to return values (null) laos for the possibly non existent neighbor
    for (int Index = 0; Index < 8; Index++)
    {
        NeighborIDs.Add(NULL_REGION);
    }

    UOpenSpan* AxisNeighborSpan = nullptr;
    UOpenSpan* DiagonalNeighborSpan = nullptr;

    for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
    {
        AxisNeighborSpan = GetAxisNeighbor(NeighborDir);
        if (AxisNeighborSpan)
        {
            NeighborIDs[NeighborDir] = AxisNeighborSpan->RegionID;
            DiagonalNeighborSpan = AxisNeighborSpan->GetDiagonalNeighbor(NeighborDir);
            if (DiagonalNeighborSpan)
            {
                NeighborIDs[NeighborDir + 4] = DiagonalNeighborSpan->RegionID;
            }
        }
    }
}

int UOpenSpan::SelectedRegionID(int BorderDirection, int CornerDirection)
{
    TArray<int> NeighborRegionIDs;
    GetNeighborRegionIDs(NeighborRegionIDs);

    int NeighborID = NeighborRegionIDs[IncreaseNeighborDirection(BorderDirection, 2)];
    if (NeighborID == RegionID || NeighborID == NULL_REGION)
    {
        return RegionID;
    }

    int PotentialRegion = NeighborID;
    
    NeighborID = NeighborRegionIDs[IncreaseNeighborDirection(CornerDirection, 2)];
    if (NeighborID == RegionID || NeighborID == NULL_REGION)
    {
        return RegionID;
    }

    int PotentialCount = 0;
    int CurrentCount = 0;

    for (int Iter = 0; Iter < 8; Iter++)
    {
        if (NeighborRegionIDs[Iter] == RegionID)
        {
            CurrentCount++;
        }
        else if (NeighborRegionIDs[Iter] == PotentialRegion)
        {
            PotentialCount++;
        }
    }

    if (PotentialCount < CurrentCount)
    {
        return RegionID;
    }

    return PotentialRegion;
}

void UOpenSpan::PartialFloodRegion(int BorderDirection, int NewRegionID)
{
    int AntiBorderDirection = IncreaseNeighborDirection(BorderDirection, 2);
    int CurrRegionID = RegionID;

   RegionID = NewRegionID;
   DistanceToRegionCore = 0;

    TArray<UOpenSpan*> TempOpenSpans;
    TArray<int> TempBorderDistances;
    TempOpenSpans.Add(this);
    TempBorderDistances.Add(0);

    while (TempOpenSpans.Num() > 0)
    {
        UOpenSpan* TempSpan = TempOpenSpans.Last();
        TempOpenSpans.RemoveAt((TempOpenSpans.Num() - 1));

        int TempBorderDistance = TempBorderDistances.Last();
        TempBorderDistances.RemoveAt((TempBorderDistances.Num() - 1));

        for (int Index = 0; Index < 4; Index++)
        {
            UOpenSpan* NeighborSpan = TempSpan->GetAxisNeighbor(Index);
            if (!NeighborSpan || NeighborSpan->RegionID != CurrRegionID)
            {
                continue;
            }

            int NeighborDistance = TempBorderDistance;
            if (Index == BorderDirection)
            {
                if (TempBorderDistance == 0)
                {
                    continue;
                }

                NeighborDistance--;
            }
            else if (Index == AntiBorderDirection)
            {
                NeighborDistance++;
            }

            NeighborSpan->RegionID = NewRegionID;
            NeighborSpan->DistanceToRegionCore = 0;

            TempOpenSpans.Add(NeighborSpan);
            TempBorderDistances.Add(NeighborDistance);
        }
    }
}

bool UOpenSpan::ProcessNullRegion(int StartDirection)
{
    int BorderRegionID = RegionID;

    UOpenSpan* Span = this;
    UOpenSpan* NeighborSpan;
    int Direction = StartDirection;

    int LoopCount = 0;
    int AcuteCornerCount = 0;
    int ObtuseCornerCount = 0;
    int StepsWithoutBorder = 0;
    bool BorderSeenLastLoop = false;
    bool IsBorder = true;

    bool HasSingleConnection = true;

    while (LoopCount < INT_MAX)
    {
        NeighborSpan = Span->GetAxisNeighbor(Direction);

        if (!NeighborSpan)
        {
            IsBorder = true;
        }
        else
        {
            NeighborSpan->ProcessedForRegionFixing = true;
            if (NeighborSpan->RegionID == NULL_REGION)
            {
                IsBorder = true;
            }
            else
            {
                IsBorder = false;
                if (NeighborSpan->RegionID != BorderRegionID)
                {
                    HasSingleConnection = false;
                }
            }
        }

        if (IsBorder)
        {
            if (BorderSeenLastLoop)
            {
                AcuteCornerCount++;
            }
            else if (StepsWithoutBorder > 1)
            {
                ObtuseCornerCount++;
                StepsWithoutBorder = 0;

                if (Span->ProcessOuterCorner(Direction))
                {
                    HasSingleConnection = false;
                }
            }

            Direction = IncreaseNeighborDirection(Direction, 1);
            BorderSeenLastLoop = true;
            StepsWithoutBorder = 0;
        }
        else
        {
            Span = NeighborSpan;
            Direction = IncreaseNeighborDirection(Direction, 3);
            BorderSeenLastLoop = false;
            StepsWithoutBorder++;
        }

        if (this == Span && StartDirection == Direction)
        {
            return (HasSingleConnection && ObtuseCornerCount > AcuteCornerCount);
        }

        LoopCount++;
    }

    return false;
}

bool UOpenSpan::ProcessOuterCorner(int BorderDirection)
{
    bool HasMultiRegions = false;

    UOpenSpan* BackOne = GetAxisNeighbor(IncreaseNeighborDirection(BorderDirection, 3));
    UOpenSpan* BackTwo = BackOne->GetAxisNeighbor(BorderDirection);

    UOpenSpan* TestSpan;

    if (BackOne->RegionID != RegionID && BackTwo->RegionID == RegionID)
    {
        HasMultiRegions = true;

        TestSpan = BackOne->GetAxisNeighbor(IncreaseNeighborDirection(BorderDirection, 3));
        int BackTwoConnections = 0;

        if (TestSpan && TestSpan->RegionID == BackOne->RegionID)
        {
            BackTwoConnections++;
            TestSpan = TestSpan->GetAxisNeighbor(BorderDirection);

            if (TestSpan && TestSpan->RegionID == BackOne->RegionID)
            {
                BackTwoConnections++;
            }
        }

        int ReferenceConnections = 0;
        TestSpan = BackOne->GetAxisNeighbor(IncreaseNeighborDirection(BorderDirection, 2));

        if (TestSpan && TestSpan->RegionID == BackOne->RegionID)
        {
            ReferenceConnections++;
            TestSpan = TestSpan->GetAxisNeighbor(IncreaseNeighborDirection(BorderDirection, 2));

            if (TestSpan && TestSpan->RegionID == BackOne->RegionID)
            {
                BackTwoConnections++;
            }
        }

        if (ReferenceConnections > BackTwoConnections)
        {
            RegionID = BackOne->RegionID;
        }
        else
        {
            BackTwo->RegionID = BackOne->RegionID;
        }
    }
    else if (BackOne->RegionID == RegionID && BackTwo->RegionID == RegionID)
    {
        int SelectedRegion = BackTwo->SelectedRegionID(IncreaseNeighborDirection(BorderDirection, 1), IncreaseNeighborDirection(BorderDirection, 2));
        if (SelectedRegion == BackTwo->RegionID)
        {
            SelectedRegion = SelectedRegionID(BorderDirection, IncreaseNeighborDirection(BorderDirection, 3));

            if (SelectedRegion != RegionID)
            {
                RegionID = SelectedRegion;
                HasMultiRegions = true;
            }
        }
        else
        {
            BackTwo->RegionID = SelectedRegion;
            HasMultiRegions = true;
        }
    }
    else
    {
        HasMultiRegions = true;
    }

    return HasMultiRegions;
}

int UOpenSpan::GetRegionEdgeDirection()
{
    for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
    {
        UOpenSpan* NeighborSpan = GetAxisNeighbor(NeighborDir);

        if (!NeighborSpan || NeighborSpan->RegionID != RegionID)
        {
            return NeighborDir;
        }
    }
    return -1;
}

int UOpenSpan::GetNonNullEdgeDirection()
{
    for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
    {
        UOpenSpan* NeighborSpan = GetAxisNeighbor(NeighborDir);

        if (NeighborSpan && NeighborSpan->RegionID != NULL_REGION)
        {
            return NeighborDir;
        }
    }
    return -1;
}

int UOpenSpan::GetNullEdgeDirection()
{
    for (int NeighborDir = 0; NeighborDir < 4; NeighborDir++)
    {
        UOpenSpan* NeighborSpan = GetAxisNeighbor(NeighborDir);

        if (!NeighborSpan || NeighborSpan->RegionID == NULL_REGION)
        {
            return NeighborDir;
        }
    }
    return -1;
}

void UOpenSpan::ResetNeighborRegionFlag()
{
    for (bool Flag : NeighborInDiffRegion)
    {
        Flag = false;
    }
}

bool UOpenSpan::CheckNeighborRegionFlag()
{
    for (bool Flag : NeighborInDiffRegion)
    {
        if (Flag)
        {
            return true;
        }
    }

    return false;
}

bool UOpenSpan::GetNeighborRegionFlag(const int Direction)
{
    return NeighborInDiffRegion[Direction];
}

int UOpenSpan::IncreaseNeighborDirection(int Direction, int Increment)
{
    int IncrementDiff;

    Direction += Increment;

    if (Direction > 3)
    {
        IncrementDiff = Direction % 4;
        Direction = IncrementDiff;
    }

    return Direction;
}

int UOpenSpan::DecreaseNeighborDirection(int Direction, int Decrement)
{
    Direction -= Decrement;
    if (Direction < 0)
    {
        Direction += 4;
    }

    return Direction;
}
