// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshDebug.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AMeshDebug::AMeshDebug()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComponent);
	RootComponent->bEditableWhenInherited = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FoundMesh(TEXT("/Game/Assets/Mesh/Cube"));

	if (FoundMesh.Succeeded())
	{
		Mesh->SetStaticMesh(FoundMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> FoundMaterial(TEXT("/Game/Assets/Material/BaseColor"));
	if (FoundMaterial.Succeeded())
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(FoundMaterial.Object, NULL);
		Mesh->SetMaterial(0, DynamicMaterial);
	}
}

// Called when the game starts or when spawned
void AMeshDebug::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMeshDebug::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AMeshDebug::SetMaterialColorOnDistance(const float DistanceToBorder, const int MaxBorderDistance)
{
	float ColorDifference = DistanceToBorder / MaxBorderDistance;

	DynamicMaterial->SetScalarParameterValue(TEXT("Blend"), ColorDifference);
}

AProceduralMeshDebug::AProceduralMeshDebug()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	Mesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
}

