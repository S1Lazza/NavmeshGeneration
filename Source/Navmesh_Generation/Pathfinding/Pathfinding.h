// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "../NavMesh/CustomNavigationData.h"
#include "Algo/Reverse.h"
#include "Pathfinding.generated.h"

UCLASS()
class NAVMESH_GENERATION_API UPathfinding : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Application of the AStar algorithm which returns a list of the polygons traversed to reach the starting location
	* This data are required by the stupid funnel algoritm to determine the shortest path
	* //TODO: Add condition to check if the path can be actually created (both target and requester are on the navmesh)
	*/
		static TArray<FPolygonData*> AStar(const FVector StartPosition, const FVector EndPosition, ACustomNavigationData* Navmesh)
	{
		TArray<bool>ClosedList;
		TArray<FPolygonData*>OpenList;

		FPolygonData* StartPoly = FindClosestPolygon(StartPosition, Navmesh);
		FPolygonData* EndPoly = FindClosestPolygon(EndPosition, Navmesh);

		//Return an empty polygon list if the target to reach is no the same polygon as the requester
		if (DestinationReached(StartPoly->Centroid, EndPoly->Centroid))
		{
			return OpenList;
		}

		OpenList.Add(EndPoly);

		//Ulity array to know the polygon laready traversed by the algorithm and that should be skipped during the iteration
		for (int Index = 0; Index < Navmesh->GetResultingPoly().Num(); Index++)
		{
			ClosedList.Add(false);
		}

		while (OpenList.Num() != 0)
		{
			FPolygonData* CurrentNode = OpenList.Last();

			//Same condition as above, exit the loop when the path is completed
			if (DestinationReached(CurrentNode->Centroid, StartPoly->Centroid))
			{
				break;
			}

			ClosedList[CurrentNode->Index] = true;

			float CurrentMinF = 0.f;
			float CurrentMinG = 0.f;
			int MinIndex = 0;

			//Iterate through the adjacent polygon list to find the one which describe the shortest path to the requester
			//And add it to the list of polygon traversed
			for (int Index = 0; Index < CurrentNode->AdjacentPolygonList.Num(); Index++)
			{
				if (ClosedList[CurrentNode->AdjacentPolygonList[Index].Index])
				{
					continue;
				}

				float GNew = FVector::DistSquared(CurrentNode->Centroid, CurrentNode->AdjacentPolygonList[Index].Centroid) + CurrentNode->G;
				float HNew = FVector::DistSquared(CurrentNode->AdjacentPolygonList[Index].Centroid, StartPoly->Centroid);
				float FNew = GNew + HNew;

				if (CurrentMinF == 0 || FNew < CurrentMinF)
				{
					CurrentMinF = FNew;
					CurrentMinG = GNew;
					MinIndex = Index;
				}
			}

			CurrentNode = FindNewPathPolygon(Navmesh->GetResultingPoly(), CurrentNode->AdjacentPolygonList[MinIndex], CurrentMinF, CurrentMinG);
			OpenList.Add(CurrentNode);
		}

		return OpenList;
	}
		
	/*
	* Retrieve the path locations from the polygon data 
	*/
	UFUNCTION(BlueprintCallable, Category = Pathfinding, meta = (ToolTip = "Return the path locations from starting to ending point"))
		static TArray<FVector> BuildPath(const FVector StartPosition, const FVector EndPosition, ACustomNavigationData* Navmesh)
	{
		TArray<FVector> PathPoints;
		TArray<FPolygonData*> PolygonsVisited = AStar(StartPosition, EndPosition, Navmesh);

		PathPoints.Add(EndPosition);

		//Add the polygon locations to the path points array
		for (int Index = 0; Index < PolygonsVisited.Num(); Index++)
		{
			PathPoints.Add(PolygonsVisited[Index]->Centroid);
		}

		PathPoints.Add(StartPosition);

		//Reverse the array to retrieve the locations in order
		Algo::Reverse(PathPoints);
		return PathPoints;
	}

	UFUNCTION(BlueprintCallable, Category = Pathfinding, meta = (ToolTip = "Return true if the starting location is equal to the target location"))
	static bool DestinationReached(const FVector CurrentPos, const FVector Destination)
	{
		if (CurrentPos == Destination)
		{
			return true;
		}

		return false;
	}

	/*
	* Find the closest polygon to the location passed in
	* //TODO: Return nullptr if the location passed in is not on the navmesh
	*/
	static FPolygonData* FindClosestPolygon(const FVector Position, ACustomNavigationData* Navmesh)
	{
		int PolyIndex = 0;
		float MaxDistance = INT_MAX;

		for (int Index = 0; Index < Navmesh->GetResultingPoly().Num(); Index++)
		{
			float Distance = FVector::DistSquared(Navmesh->GetResultingPoly()[Index].Centroid, Position);

			if (Distance < MaxDistance)
			{
				MaxDistance = Distance;
				PolyIndex = Index;
			}
		}

		return &Navmesh->GetResultingPoly()[PolyIndex];
	}

	/*
	* Compare the adjacent polygon data found to the original polygon array and retrieve the respective polygon from it
	*/
	static FPolygonData* FindNewPathPolygon(TArray<FPolygonData> Polys, const FPolygonData& PolyToSearch, const float F, const float G)
	{
		for (int Index = 0; Index < Polys.Num(); Index++)
		{
			if (Polys[Index].Index == PolyToSearch.Index)
			{
				Polys[Index].F = F;
				Polys[Index].G = G;
				return &Polys[Index];
			}
		}

		return nullptr;
	}
};