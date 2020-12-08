// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "NavMeshGenerator.generated.h"

class UBillboardComponent;
class ASolidHeightfield;
class AOpenHeightfield;

UCLASS()
class NAVMESH_GENERATION_API ANavMeshGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	//Sets default values for this actor's properties
	ANavMeshGenerator();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Gather all the valid geometry in the level, meaning the overlapping ones, world static, that can affect navigation
	void GatherValidOverlappingGeometries();

	//Generate the navmesh
	void GenerateNavmesh();

	//Create the solid heightfield and return the voxel data
	void CreateSolidHeightfield(ASolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh);

	//Create an open heightfield based on the data retrieved from the solid one and return it
	void CreateOpenHeightfield(AOpenHeightfield* OpenHeightfield, const ASolidHeightfield* SolidHeightfield, bool PerformFullGeneration);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	//Intialize the needed components listed below
	void InitializeComponents();

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BoxBounds;

	UPROPERTY(VisibleDefaultsOnly)
	UBillboardComponent* Icon;

	TArray<UStaticMeshComponent*> Geometries;
};
