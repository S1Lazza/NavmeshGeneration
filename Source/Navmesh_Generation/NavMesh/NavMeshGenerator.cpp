// Fill out your copyright notice in the Description page of Project Settings.


#include "NavMeshGenerator.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "../Utility/UtilityGeneral.h"
#include "../Utility/UtilityDebug.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ANavMeshGenerator::ANavMeshGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	InitializeComponents();
}

// Called when the game starts or when spawned
void ANavMeshGenerator::BeginPlay()
{
	Super::BeginPlay();
	GatherValidOverlappingGeometries();
	GenerateNavmesh();
}

void ANavMeshGenerator::GatherValidOverlappingGeometries()
{
	TArray<AActor*> ValidGeometries;

	//Query parameters for filtering the overlapping actors, ObjectTypeQuery1 = WorldStatic
	TArray< TEnumAsByte<EObjectTypeQuery> > ObjectTypes;
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);
	
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	//Check which actors are overlapping with the box based on the parameters specified
	UKismetSystemLibrary::ComponentOverlapActors(BoxBounds, this->GetTransform(), ObjectTypes, nullptr, ActorsToIgnore, ValidGeometries);

	for (AActor*& actor : ValidGeometries)
	{
		UActorComponent* ActorCmp = actor->GetComponentByClass(UStaticMeshComponent::StaticClass());

		//Check if the overlapping actors have a static mesh component
		if (ActorCmp)
		{
			//Check if the actor is flagged as been able to affect the navigation
			if (ActorCmp->CanEverAffectNavigation())
			{
				//Check if the actor collision has been set to block pawns
				UStaticMeshComponent* Mesh = CastChecked<UStaticMeshComponent>(ActorCmp);

				if (Mesh->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block)
				{
					//Add the mesh to the array
					Geometries.Add(Mesh);
				}
			}
		}
	}
}

void ANavMeshGenerator::GenerateNavmesh()
{
	if (Geometries.Num() == 0)
	{
		return;
	}

	//Initialize spawn parameters, needed to create a new instance of the actors (the always spawn flag is used for safety purposes)
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASolidHeightfield* SolidHF = GetWorld()->SpawnActor<ASolidHeightfield>(GetActorLocation(), FRotator(0.f, 0.f, 0.f), SpawnInfo);
	AOpenHeightfield* OpenHF = GetWorld()->SpawnActor<AOpenHeightfield>(GetActorLocation(), FRotator(0.f, 0.f, 0.f), SpawnInfo);

	SolidHF->DefineFieldsBounds(GetActorLocation(), BoxBounds->GetScaledBoxExtent());

	for (UStaticMeshComponent* Mesh : Geometries)
	{
		CreateSolidHeightfield(SolidHF, Mesh);
	}

	CreateOpenHeightfield(OpenHF, SolidHF, true);
}

void ANavMeshGenerator::CreateSolidHeightfield(ASolidHeightfield* SolidHeightfield, const UStaticMeshComponent* Mesh)
{
	TArray<FVector> Vertices;
	TArray<int> Indices;

	UUtilityGeneral::GetAllMeshVertices(Mesh, Vertices);
	UUtilityGeneral::GetMeshIndices(Mesh, Indices);

	if (Vertices.Num() == 0 || Indices.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("No data to generate the solid heightfield"));
		return;
	}

	SolidHeightfield->VoxelizeTriangles(Vertices, Indices);
	SolidHeightfield->MarkLowHeightSpan();

	//TODO Currently the ledge spans are incorrectly filtered, need to check the method below
	/*SolidHeightfield->MarkLedgeSpan();*/

	/*SolidHeightfield->DrawDebugSpanData();*/
}

void ANavMeshGenerator::CreateOpenHeightfield(AOpenHeightfield* OpenHeightfield, const ASolidHeightfield* SolidHeightfield, bool PerformFullGeneration)
{
	OpenHeightfield->Init(SolidHeightfield);
	OpenHeightfield->FindOpenSpanData(SolidHeightfield);
	/*OpenHeightfield->DrawDebugSpanData();*/
	OpenHeightfield->GenerateNeightborLinks();
	OpenHeightfield->GenerateDistanceField();
	/*OpenHeightfield->DrawDistanceFieldDebugData(false, true);*/
	OpenHeightfield->GenerateRegions();
	OpenHeightfield->HandleSmallRegions();
	OpenHeightfield->CleanNullRegionBorders();
	OpenHeightfield->DrawDebugRegions(false, true);
}

void ANavMeshGenerator::InitializeComponents()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComponent);
	RootComponent->bEditableWhenInherited = true;

	BoxBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxBounds"));
	BoxBounds->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	BoxBounds->AttachTo(RootComponent);

	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	Icon->SetupAttachment(RootComponent);
}

// Called every frame
void ANavMeshGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

