// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavmeshController.generated.h"

class UBillboardComponent;
class USolidHeightfield;
class UOpenHeightfield;
class UContour;
class UPolygonMesh;
class FNavMeshGenerator;

UCLASS()
class NAVMESH_GENERATION_API ANavMeshController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANavMeshController();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Needed to rebuilt the navmesh when a parameters is changed in editor
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	//Initialize utility components for better visualization inside the editor
	void InitComponents();

	void SetNavGenerator(TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NavGenerator);

	void DeleteDebugPlanes();

	void DeleteDebugText();

	void ToggleDebugOptions();

	UPROPERTY(VisibleAnywhere)
	UBillboardComponent* Icon;

	/*
	* List of modifiable NavMesh parameters
	*/

	//Enable/Disable the option to automatically rebuild the navmesh when a valid geometry is moved inside the NavMesh bound
	//If disabled go to the Build->Build Path editor option to manually rebuild the navmesh  
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters", meta = (DisplayName = "EnableDirtyAreasRebuild"))
	bool EnableDirtyAreasRebuild = true;

	//Size of the single cells (voxels) in which the heightfiels is subdivided, the cells are squared
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "CellSize"))
	float CellSize = 40.f;

	//Height of the single cells (voxels) in which the heightfiels is subdivided
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "CellHeight"))
	float CellHeight = 40.f;

	//Represent the maximum slope angle (in degree) that is considered traversable
	//Cells that pass the value specified are flagged as UNWALKABLE
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "MaxTraversableAngle"))
	float MaxTraversableAngle = 45.f;

	//Represent the minimum height distance between 2 spans (min of the upper and max of the lower one)
	//that allow for the min one to be still considered walkable
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "MinTraversableHeight"))
	float MinTraversableHeight = 100.f;

	//Represents the maximum ledge height that is considered to be still traversable
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "MaxTraversableStep"))
	float MaxTraversableStep = 50.f;

	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|SolidHeightfield", meta = (DisplayName = "EnableHeightSpanDebug"))
	bool EnableHeightSpanDebug = false;

	//The amount of smoothing to be performed when generating the distance field
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "SmoothingThreshold"))
	int SmoothingThreshold = 2;

	//Closest distance any part of a mesh can get to an obstruction in the source geometry
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "TraversableAreaBorderSize"))
	int TraversableAreaBorderSize = 1;

	//Minimum span size of the island region to remove
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "MinUnconnectedRegionSize"))
	int MinUnconnectedRegionSize = 4;

	//Minimum span size of the region to merge
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "MinMergeRegionSize"))
	int MinMergeRegionSize = 20;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "PerformFullGeneration"))
	bool PerformFullGeneration = true;

	//Applies extra algorithms to regions to help prevent poorly formed regions from forming at the cost of extra processing power
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "UseConservativeExpansion"))
	bool UseConservativeExpansion = true;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "EnableOpenSpanData"))
	bool EnableOpenSpanDebug = false;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "EnableDistanceFieldDebug"))
	bool EnableDistanceFieldDebug = false;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|OpenHeightfield", meta = (DisplayName = "EnableRegionsDebug"))
	bool EnableRegionsDebug = false;

	//The maximum distance the edge of the contour may deviate from the source geometry - less the distance, more precise and intense the calculation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|Contour", meta = (DisplayName = "EdgeMaxDeviation"))
	float EdgeMaxDeviation = 60.f;

	//The maximum length of polygon edges that represent the border of meshes 
	//More vertices will be added to border edges if this value is exceeded for a particular edge
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|Contour", meta = (DisplayName = "EdgeMaxDeviation"))
	float MaxEdgeLenght = 60.f;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|Contour", meta = (DisplayName = "EnableContourDebug"))
	bool EnableContourDebug = false;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|PolygonMesh", meta = (DisplayName = "MaxVertexPerPoly"))
	int MaxVertexPerPoly = 6;

	//Specify if the generation of the Openfield must perform all the steps up to the region creation or stop at the open span creation
	UPROPERTY(EditAnywhere, Category = "NavmeshParameters|PolygonMesh", meta = (DisplayName = "EnablePolyMeshDebug"))
	bool EnablePolyMeshDebug = false;

private:
	TSharedPtr<FNavMeshGenerator, ESPMode::ThreadSafe> NavMeshGenerator;
};
