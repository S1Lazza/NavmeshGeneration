// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HeightSpan.generated.h"

UENUM()
enum class PolygonType : uint8
{
	WALKABLE = 0	UMETA(DisplayName = "WALKABLE"),
	UNWALKABLE		UMETA(DisplayName = "UNWALKABLE")
};

UCLASS()
class NAVMESH_GENERATION_API UHeightSpan : public UObject
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UHeightSpan() {};

	UHeightSpan(const int MinHeight, const int MaxHeight, const PolygonType& Type);

	// Called every frame
	virtual void Tick(float DeltaTime) {};

	//Set the new max span height value, auto clamp the value to the span min + 1
	void SetMaxHeight(const int NewHeight);

	//Set the new max span height value, auto clamp the value to the span max - 1
	void SetMinHeight(const int NewHeight);

	//Min height of the span
	int Min = 0;

	//Max height of the span
	int Max = 0;

	//Width of the span
	int Width = 0;

	//Depth of the span
	int Depth = 0;

	/*Next span in the column
	  Flagged as uproperty as it is needed to have Unreal keep track of it, if not the pointer will be invalidated 
	  as soon as the code exit the scope in which it has been assigned*/
	UPROPERTY()
	UHeightSpan* nextSpan;

	//Attribute of the span considered
	PolygonType SpanAttribute;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() {};


};
