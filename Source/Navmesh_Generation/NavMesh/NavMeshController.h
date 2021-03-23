// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavmeshController.generated.h"

class UBillboardComponent;

UCLASS()
class NAVMESH_GENERATION_API ANavMeshController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANavMeshController();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void InitComponents();

private:
	UPROPERTY(VisibleAnywhere)
	UBillboardComponent* Icon;
};
