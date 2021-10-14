// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshController.h"
#include "Components/BillboardComponent.h"
#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "Contour.h"
#include "PolygonMesh.h"
#include "DetailedMesh.h"
#include "NavMeshGenerator.h"
#include "../Utility/MeshDebug.h"

#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/TextRenderActor.h"

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

void ANavMeshController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	//Temporary solution to prevent a couple of bugs to happen
	CellHeight = CellSize;
	MaxEdgeLenght = FMath::Clamp(MaxEdgeLenght, CellSize, float(INT_MAX));

	//TODO: Find a way to avoid rebuilding the navmesh every single time a debug option is checked/unchkeched
	NavMeshGenerator.Get()->RebuildAll();
}

void ANavMeshController::InitComponents()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComponent);
	RootComponent->bEditableWhenInherited = true;

	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	Icon->SetupAttachment(RootComponent);
}

void ANavMeshController::SetNavGenerator(TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NavGenerator)
{
	NavMeshGenerator = NavGenerator;
}

void ANavMeshController::UpdateEditorPosition()
{
	SetActorLocation(NavMeshGenerator.Get()->GetNavBounds().GetCenter());
}


void ANavMeshController::DeleteDebugPlanes()
{
	TArray<AActor*> DebugPlanes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMeshDebug::StaticClass(), DebugPlanes);

	for (AActor* Plane : DebugPlanes)
	{
		Plane->Destroy();
	}
}

void ANavMeshController::DeleteDebugText()
{
	TArray<AActor*> DebugText;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATextRenderActor::StaticClass(), DebugText);

	for (AActor* Text : DebugText)
	{
		Text->Destroy();
	}
}

void ANavMeshController::ClearDebugLines()
{
	FlushPersistentDebugLines(GetWorld());
}

void ANavMeshController::DisplayDebugElements()
{
	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("Impossible to display debug elements, world is invalid"));
	}

	ClearDebugLines();
	DeleteDebugPlanes();
	DeleteDebugText();

	if (EnableHeightSpanDebug)
	{
		NavMeshGenerator->GetSolidHeightfield()->DrawDebugSpanData();
	}

	if (EnableOpenSpanDebug)
	{
		NavMeshGenerator->GetOpenHeightfield()->DrawDebugSpanData();
	}

	if (EnableDistanceFieldDebug)
	{
		NavMeshGenerator->GetOpenHeightfield()->DrawDistanceFieldDebugData(false, true);
	}

	if (EnableRegionsDebug)
	{
		NavMeshGenerator->GetOpenHeightfield()->DrawDebugRegions(false, true);
	}

	if (EnableContourDebug)
	{
		NavMeshGenerator->GetContour()->DrawRegionContour();
	}

	if (EnablePolyMeshDebug)
	{
		NavMeshGenerator->GetPolygonMesh()->DrawDebugPolyMeshPolys();
	}

	if (EnablePolyCentroidDebug)
	{
		NavMeshGenerator->GetPolygonMesh()->DrawPolygonCentroid();
	}
}
