// Fill out your copyright notice in the Description page of Project Settings.


#include "Region.h"

void URegion::ResetID(const int NewRegionID)
{
	ID = NewRegionID;
	SpanCount = 0;
	Connections.Empty();
	OverlappingRegions.Empty();
}

bool URegion::CanRegionBeMergedWith(URegion* OtherRegion)
{
	int ValidConnections = 0;
	for (int ConnectionID : Connections)
	{
		if (ConnectionID == OtherRegion->ID)
		{
			ValidConnections++;
		}
	}

	// If the regions compared are 
	// 1 - Connecting in more than one point or they do not connect
	// 2 - Overlapping vertically
	// They cannot be merged
	if (ValidConnections != 1 || OverlappingRegions.Contains(OtherRegion->ID) || OtherRegion->OverlappingRegions.Contains(ID))
	{
		return false;
	}

	return true;
}

bool URegion::PerformRegionMergingIn(URegion* TargetRegion)
{
	//Get the ID of where the target region connects with the current one considered
	int ConnectionPointOnTarget = TargetRegion->Connections.IndexOfByKey(ID);
	if (ConnectionPointOnTarget == -1)
	{
		return false;
	}

	//Get the ID of where the current region connects the target one
	int ConnectionPointOnCurrent = Connections.IndexOfByKey(TargetRegion->ID);
	if (ConnectionPointOnCurrent == -1)
	{
		return false;
	}

	//Save connection information before rebuilding them
	TArray<int> TargetConnections = TargetRegion->Connections;
	TargetRegion->Connections.Empty();
	int ConnectionSize = TargetConnections.Num();

	//To rebuild the target connection start from the connection point after the one found with the current region and loop back to the one prior it 
	for (int i = 0; i < ConnectionSize - 1; i++)
	{
		TargetRegion->Connections.Add(TargetConnections[(ConnectionPointOnTarget + 1 + i) % ConnectionSize]);
	}

	//Insert current connections into target connections at their mutual connection point
	ConnectionSize = Connections.Num();
	for (int i = 0; i < ConnectionSize - 1; i++)
	{
		TargetRegion->Connections.Add(Connections[(ConnectionPointOnCurrent + 1 + i) % ConnectionSize]);
	}

	//Remove duplicates
	TargetRegion->RemoveAdjacentDuplicateConnections();

	//Add overlap data from the current to the target
	for (int i : OverlappingRegions)
	{
		if (!TargetRegion->OverlappingRegions.Contains(i))
		{
			TargetRegion->OverlappingRegions.Add(i);
		}
	}

	//Update the span count to the new total
	TargetRegion->SpanCount += SpanCount;

	return true;
}

void URegion::RemoveAdjacentDuplicateConnections()
{
	int Connection = 0;

	//Loop through all the connection array
	while (Connection < Connections.Num() && Connections.Num() > 1)
	{
		int NextConnection = Connection + 1;
		
		//If the last connection is reached, loop back to the first one
		if (NextConnection >= Connections.Num())
		{
			NextConnection = 0;
		}

		//If an adjacent connection is found, remove the duplicate
		//And stay at the current index to see if additional adjacent are found
		if (Connections[Connection] == Connections[NextConnection])
		{
			Connections.RemoveAt(NextConnection);
		}
		else
		{
			Connection++;
		}
	}
}

void URegion::ReplaceNeighborRegionID(int OldID, int NewID)
{
	bool ConnectionChanged = false;

	//Iterate through the connection array and, if old region id are found, replace them with the new one
	for (int i = 0; i < Connections.Num(); i++)
	{
		if (Connections[i] == OldID)
		{
			Connections[i] = NewID;
			ConnectionChanged = true;
		}
	}

	//Do the same for the overlapping region
	for (int i = 0; i < OverlappingRegions.Num(); i++)
	{
		if (OverlappingRegions[i] == OldID)
		{
			OverlappingRegions[i] = NewID;
		}
	}

	//Make sure no duplicates exists, if the ID are replaced succesfully
	if (ConnectionChanged)
	{
		RemoveAdjacentDuplicateConnections();
	}
}
