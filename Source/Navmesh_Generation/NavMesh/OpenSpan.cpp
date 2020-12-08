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