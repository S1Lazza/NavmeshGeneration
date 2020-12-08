// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "MeshDebug.generated.h"

class UMaterialInstanceDynamic;

UCLASS()
class NAVMESH_GENERATION_API AMeshDebug : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshDebug();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetMaterialColorOnDistance(const float DistanceToBorder, const int MaxBorderDistance);

private:
	UStaticMeshComponent* Mesh;
	UMaterialInstanceDynamic* DynamicMaterial;
};
