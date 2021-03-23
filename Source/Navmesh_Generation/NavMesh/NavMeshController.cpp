// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshController.h"
#include "Components/BillboardComponent.h"

// Sets default values
ANavMeshController::ANavMeshController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	InitComponents();
}

// Called when the game starts or when spawned
void ANavMeshController::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANavMeshController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANavMeshController::InitComponents()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComponent);
	RootComponent->bEditableWhenInherited = true;

	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	Icon->SetupAttachment(RootComponent);
}
