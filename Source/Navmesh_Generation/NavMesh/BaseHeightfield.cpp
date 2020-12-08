// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseHeightfield.h"
#include "../Utility/UtilityDebug.h"

ABaseHeightfield::ABaseHeightfield()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CalculateWidthDepthHeight();
}

void ABaseHeightfield::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseHeightfield::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseHeightfield::CalculateWidthDepthHeight()
{
	FVector BoundExtension = BoundMax - BoundMin;
	Width = FMath::RoundFromZero(BoundExtension.X / CellSize);
	Depth = FMath::RoundFromZero(BoundExtension.Y / CellSize);
	Height = FMath::RoundFromZero(BoundExtension.Z / CellHeight);
}

int ABaseHeightfield::GetGridIndex(const int WidthIndex, const int DepthIndex)
{
	if (WidthIndex < 0 || DepthIndex < 0 || WidthIndex >= Width || DepthIndex >= Depth)
	{
		/*GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("GridIndex out of bounds"));*/
		return -1;
	}

	return DepthIndex * Width + WidthIndex;
}

int ABaseHeightfield::GetDirOffSetWidth(const int Direction)
{
	TArray<int> Offset = { -1, 0, 1, 0 };

	return Offset[Direction & 0X03];
}

int ABaseHeightfield::GetDirOffSetDepth(const int Direction)
{
	TArray<int> Offset = { 0, -1, 0, 1 };

	return Offset[Direction & 0X03];
}
