// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UtilityGeneral.generated.h"

UCLASS()
class NAVMESH_GENERATION_API UUtilityGeneral : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*
	* Return the vertices of a mesh without duplicates
	*/
	UFUNCTION(BlueprintCallable, Category = StaticMesh, meta = (ToolTip = "Get the indexed vertices of a mesh"))
		static void GetMeshIndexedVertices(const UStaticMeshComponent* Mesh, TArray<FVector>& Vertices) 
	{
		if (!Mesh)
		{
			return;
		}

		FPositionVertexBuffer* VertexBuffer = &Mesh->GetStaticMesh()->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;

		//Iterate through all the vertices
		int32 VertexCount = VertexBuffer->GetNumVertices();
		for (int32 It = 0; It < VertexCount; It++)
		{
			const FVector VertexLocalPos = VertexBuffer->VertexPosition(It);

			//Converts the UStaticMesh vertex translation, rotation, and scaling in world space
			const FVector VertexWorldPos = Mesh->GetOwner()->GetActorLocation() + Mesh->GetOwner()->GetTransform().TransformVector(VertexLocalPos);;
			Vertices.Add(VertexWorldPos);
		}
	}

	/*
	* Return the indexes of a mesh 
	*/
	UFUNCTION(BlueprintCallable, Category = StaticMesh, meta = (ToolTip = "Get the indexes of a mesh"))
		static void GetMeshIndices(const UStaticMeshComponent* Mesh, TArray<int>& Indexes)
	{
		if (!Mesh)
		{
			return;
		}

		FRawStaticIndexBuffer* IndexBuffers = &Mesh->GetStaticMesh()->RenderData->LODResources[0].IndexBuffer;

		int32 IndexCount = IndexBuffers->GetNumIndices();
		for (int32 I = 0; I < IndexCount; I++)
		{
			const int Index = IndexBuffers->GetIndex(I);
			Indexes.Add(Index);
		}
	}

	/*
	* Get all the vertices of a mesh with duplicates
	*/
	UFUNCTION(BlueprintCallable, Category = StaticMesh, meta = (ToolTip = "Get all the vertices of a mesh"))
		static void GetAllMeshVertices(const UStaticMeshComponent* Mesh, TArray<FVector>& Vertices)
	{
		if (!Mesh)
		{
			return;
		}

		TArray<FVector> IndexedVertices;
		TArray<int> Indices; 

		GetMeshIndexedVertices(Mesh, IndexedVertices);
		GetMeshIndices(Mesh, Indices);

		for (int It = 0; It < Indices.Num(); It++)
		{
			Vertices.Add(IndexedVertices[Indices[It]]);
		}
	}
};

/*
* Based on the index provided resized the specified array if the index exceed the current size
* Otherwise it sets the element indexed to the new one
*/
template <typename T>
static void SetArrayElement(TArray<T>& ItemArray, const T Item, const int Index)
{
	if (ItemArray.Num() - 1 < Index)
	{
		ItemArray.SetNum(Index);
		ItemArray.Insert(Item, Index);
	}
	else
	{
		ItemArray[Index] = Item;
	}
}
