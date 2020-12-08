// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "UtilityDebug.generated.h"


UCLASS()
class NAVMESH_GENERATION_API UUtilityDebug : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/*
	* Draw an AABB box based on its min and max bound
	*/
	UFUNCTION(BlueprintCallable, Category = Debug, meta = (WorldContext = "WorldContextObject"), meta = (ToolTip = "Draw an AABB box based on its min and max bound"))
		static void DrawMinMaxBox(const UObject* WorldContextObject, const FVector& Min, const FVector& Max, const FColor Color, const float Time, const float Thickness)
	{
		//Blueprint cannot access the world so it can't be passed as parameter
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		FVector Center = (Max + Min) / 2;
		FVector BoxExtension = FVector((Max - Min) / 2);
		DrawDebugBox(World, Center, BoxExtension, Color, false, Time, 0, Thickness);
	}

	/*
	* Draw an X amount of triangles, representing the faces of a mesh, based on the input data passed in
	* It assumes the amount of data passed in can be divided by 3
	*/
	UFUNCTION(BlueprintCallable, Category = Debug, meta = (WorldContext = "WorldContextObject"), meta = (ToolTip = "Draw a series of debug triangles"))
		static void DrawMeshFaces(const UObject* WorldContextObject, const TArray<FVector>& InputData, const FColor Color, const float Time, const float Thickness)
	{
		int ArrayLength = InputData.Num();
		if (ArrayLength % 3 != 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Incorrect number of vertices passed in, the number must be divided by 3"));
			return;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		int Step = 0;

		while (Step < ArrayLength - 2)
		{
			DrawDebugLine(World, InputData[Step], InputData[Step + 1], Color, false, Time, 10, Thickness);
			DrawDebugLine(World, InputData[Step + 1], InputData[Step + 2], Color, false, Time, 10, Thickness);
			DrawDebugLine(World, InputData[Step], InputData[Step + 2], Color, false, Time, 10, Thickness);

			Step += 3;
		}
	}

	/*
	* Draw a sequence of line to represent a polygon
	* It assumes the amount of data passed in is greater than 3
	*/
	UFUNCTION(BlueprintCallable, Category = Debug, meta = (WorldContext = "WorldContextObject"), meta = (ToolTip = "Draw a polygon"))
		static void DrawPolygon(const UObject* WorldContextObject, const TArray<FVector>& InputData, const FColor Color, const float Time, const float Thickness)
	{
		int ArrayLength = InputData.Num();
		if (ArrayLength < 3)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Insufficient number of vertices passed in, they need to be more than 3"));
			return;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		FVector SecondPoint = InputData[ArrayLength - 1];

		for (FVector FirstPoint : InputData)
		{
			DrawDebugLine(World, FirstPoint, SecondPoint, Color, false, Time, 10, Thickness);
			SecondPoint = FirstPoint;
		}
	}
};
