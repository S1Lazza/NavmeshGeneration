// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Contour.generated.h"

UCLASS()
class NAVMESH_GENERATION_API AContour : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AContour();

	void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	//Region ID to which the countour refer to
	int RegionID;

	//Total amount of the detailed vertices
	int RawVerticesCount;

	//Total amount of the simplified vertices 
	int SimplifiedVerticesCount;

	//Vertices representing the detailed contour
	TArray<FVector> RawVertices;

	//Vertices representing the simplified contour
	TArray<FVector> SimplifiedVertices;
};
