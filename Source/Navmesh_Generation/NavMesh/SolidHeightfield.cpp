// Fill out your copyright notice in the Description page of Project Settings.


#include "SolidHeightfield.h"
#include "OpenHeightfield.h"
#include "NavMeshController.h"
#include "../Utility/UtilityDebug.h"
#include "../Utility/UtilityGeneral.h"

void USolidHeightfield::InitializeParameters(const ANavMeshController* NavController)
{
	CurrentWorld = NavController->GetWorld();
	CellSize = NavController->CellSize;
	CellHeight = NavController->CellHeight;
	MaxTraversableAngle = NavController->MaxTraversableAngle;
	MinTraversableHeight = NavController->MinTraversableHeight;
	MaxTraversableStep = NavController->MaxTraversableStep;
	CalculateUpNormal();
}

void USolidHeightfield::DefineFieldsBounds(const FVector AreaCenter, const FVector AreaExtent)
{
	BoundMin = AreaCenter - AreaExtent;
	BoundMax = AreaCenter + AreaExtent;

	//Ricalculate width, depth and height based on the new bounds
	CalculateWidthDepthHeight();

	//Draw debug info relative to the bounding box surrounding the mesh
	//UUtilityDebug::DrawMinMaxBox(CurrentWorld, BoundMin, BoundMax, FColor::Red, 20.0f, 2.0f);
}

void USolidHeightfield::VoxelizeTriangles(const TArray<FVector>& Vertices, const TArray<int> Indices)
{
	const float InvertCellSize = 1 / CellSize;
	const float InvertCellHeight = 1 / CellHeight;

	float SolidFieldMinHeight = 0;
	float SolidFieldMaxHeight = 0;
	FindGeometryHeight(Vertices, SolidFieldMinHeight, SolidFieldMaxHeight);

	int PolyCount = Indices.Num() / 3;

	for (int PolyIndex = 0; PolyIndex < PolyCount; PolyIndex++)
	{
		TArray<FVector> PolyVertices;

		//Find the vertices coordinates for every triangle of the mesh and add them to the array
		FVector VertexA = FVector(Vertices[PolyIndex * 3].X, Vertices[PolyIndex * 3].Y, Vertices[PolyIndex * 3].Z);
		FVector VertexB = FVector(Vertices[PolyIndex * 3 + 1].X, Vertices[PolyIndex * 3 + 1].Y, Vertices[PolyIndex * 3 + 1].Z);
		FVector VertexC = FVector(Vertices[PolyIndex * 3 + 2].X, Vertices[PolyIndex * 3 + 2].Y, Vertices[PolyIndex * 3 + 2].Z);

		PolyVertices.Add(VertexA);
		PolyVertices.Add(VertexB);
		PolyVertices.Add(VertexC);

		//Find the walkable polygons inside the mesh
		PolygonType Type = FilterWalkablePolygon(PolyVertices);

		//Find the bounding box surrounding the triangle by comparing the vertices coordinates
		//Default to the first vertex
		FVector TriBoundsMin(VertexA.X, VertexA.Y, VertexA.Z);
		FVector TriBoundsMax(VertexA.X, VertexA.Y, VertexA.Z);

		for (int It = 0; It < 3; It++)
		{
			TriBoundsMin.X = FMath::Min(TriBoundsMin.X, PolyVertices[It].X);
			TriBoundsMin.Y = FMath::Min(TriBoundsMin.Y, PolyVertices[It].Y);
			TriBoundsMin.Z = FMath::Min(TriBoundsMin.Z, PolyVertices[It].Z);

			TriBoundsMax.X = FMath::Max(TriBoundsMax.X, PolyVertices[It].X);
			TriBoundsMax.Y = FMath::Max(TriBoundsMax.Y, PolyVertices[It].Y);
			TriBoundsMax.Z = FMath::Max(TriBoundsMax.Z, PolyVertices[It].Z);
		}

		//Draw debug info relative to the polygon of the mesh  
		/*UUtilityDebug::DrawMeshFaces(CurrentWorld, PolyVertices, FColor::Blue, 20, 1.0f);*/

		//Draw debug info relative the the bounding box delimiting the polygon 
		/*UUtilityDebug::DrawMinMaxBox(CurrentWorld, TriBoundsMin, TriBoundsMax, FColor::Red, 20.0f, 1.0f);*/

		//Based on the bounding box data found, retrieve the width and depth and height covered by the cells inside it
		int TriWidthMin = int((TriBoundsMin.X - BoundMin.X) * InvertCellSize);
		int TriDepthMin = int((TriBoundsMin.Y - BoundMin.Y) * InvertCellSize);
		int TriHeightMin = int((TriBoundsMin.Z - BoundMin.Z) * InvertCellHeight);
		int TriWidthMax = int((TriBoundsMax.X - BoundMin.X) * InvertCellSize);
		int TriDepthMax = int((TriBoundsMax.Y - BoundMin.Y) * InvertCellSize);
		int TriHeightMax = int((TriBoundsMax.Z - BoundMin.Z) * InvertCellHeight);

		//And make sure the cells are inside the bounds of the grid
		TriWidthMin = FMath::Clamp(TriWidthMin, 0, Width - 1);
		TriDepthMin = FMath::Clamp(TriDepthMin, 0, Depth - 1);
		TriHeightMin = FMath::Clamp(TriHeightMin, 0, Height - 1);
		TriWidthMax = FMath::Clamp(TriWidthMax, 0, Width - 1);
		TriDepthMax = FMath::Clamp(TriDepthMax, 0, Depth - 1);
		TriHeightMax = FMath::Clamp(TriHeightMax, 0, Height - 1);

		//Loop through all the cells and find the clipped polygons from the base triangle contained inside each of them
		//If the output number of vertices of the clipped polygon is less than 3 it is not a polygon and therefore the cell is invalid and can be ignored
		TArray<FVector> ClippedPolygonVertices;
		ClippedPolygonVertices.Reserve(9);

		//The height extension of the heightfield
		float FieldHeight = SolidFieldMaxHeight - BoundMin.Z;

		for (int DepthIndex = TriDepthMin; DepthIndex <= TriDepthMax; DepthIndex++)
		{
			for (int WidthIndex = TriWidthMin; WidthIndex <= TriWidthMax; ++WidthIndex)
			{
				//Find the min and max coord of each cell
				FVector CellMinCoord = FVector(BoundMin.X + CellSize * WidthIndex, BoundMin.Y + CellSize * DepthIndex, SolidFieldMinHeight);
				FVector CellMaxCoord = FVector(CellMinCoord.X + CellSize, CellMinCoord.Y + CellSize, SolidFieldMaxHeight);

				ClippedPolygonVertices = PolyVertices;
				ClipPolygon(ClippedPolygonVertices, CellMinCoord, CellMaxCoord);

				int ClippedVerticesNumber = ClippedPolygonVertices.Num();
				if (ClippedVerticesNumber < 3)
				{
					continue;
				}

				//Draw debug info relative to the clipped polygon inside the cell
				/*UUtilityDebug::DrawPolygon(CurrentWorld, ClippedPolygonVertices, FColor::Blue, 20.0f, 1.0f);*/

				//Based on the clipped vertex coordinates, find the z axis range for the cell
				//Default value to the first vertex
				float HeightMin = ClippedPolygonVertices[0].Z;
				float HeightMax = ClippedPolygonVertices[0].Z;

				for (int It = 1; It < ClippedVerticesNumber; It++)
				{
					HeightMin = FMath::Min(HeightMin, ClippedPolygonVertices[It].Z);
					HeightMax = FMath::Max(HeightMax, ClippedPolygonVertices[It].Z);
				}

				// Convert to height above the base of the heightfield.
				HeightMin -= BoundMin.Z;
				HeightMax -= BoundMin.Z;

				// The height of the cell is entirely outside the bounds of the heightfield, skip the cell
				if (HeightMax < 0.0f || HeightMin > FieldHeight)
				{
					continue;
				}

				//Make sure the height min and max are clamped to the bound of the bounding box  
				if (HeightMin < 0.0f)
				{
					HeightMin = 0.0f;
				}

				if (HeightMax > FieldHeight)
				{
					HeightMax = FieldHeight;
				}

				//Conver the height coordinate data to voxel/grid data
				int HeightIndexMin = FMath::Clamp(int(FMath::FloorToInt(HeightMin * InvertCellHeight)), 0, INT_MAX);
				int HeightIndexMax = FMath::Clamp(int(FMath::CeilToInt(HeightMax * InvertCellHeight)), 0, INT_MAX);

				AddSpanData(WidthIndex, DepthIndex, HeightIndexMin, HeightIndexMax, Type);

				//Draw debug info relative to single valid cells
				/*for (int It = HeightIndexMin; It <= HeightIndexMax; ++It)
				{
					FVector CellMinDebug = FVector(CellMinCoord.X, CellMinCoord.Y, BoundMin.Z + CellSize * It);
					FVector CellMaxDebug = FVector(CellMaxCoord.X, CellMaxCoord.Y, BoundMin.Z + CellSize * It + CellSize);

					UUtilityDebug::DrawMinMaxBox(CurrentWorld, CellMinDebug, CellMaxDebug, FColor::Green, 20.0f, 0.5f);
				}*/	
			}
		}
	}
}

void USolidHeightfield::FindGeometryHeight(const TArray<FVector>& Vertices, float& MinHeight, float& MaxHeight)
{
	//Assign the bound min and max to the coordinates of the first vertex
	MinHeight = Vertices[0].Z;
	MaxHeight = Vertices[0].Z;

	//Iterate through all the vertices to find the actual bounds
	for (FVector Vertex : Vertices)
	{
		MinHeight = FMath::Min(Vertex.Z, MinHeight);
		MaxHeight = FMath::Max(Vertex.Z, MaxHeight);
	}
}

void USolidHeightfield::ClipVersusPlane(TArray<FVector>& Vertices, FVector MinBound, FVector MaxBound, BoxSide Side)
{
	int VerticesNumber = Vertices.Num();

	if (VerticesNumber < 3)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Insufficient number of vertices passed in, not a polygon, invalid cell"));
		return;
	}

	int It_1 = 0;
	int It_2 = 0;
	TArray<FVector> VerticesOut;

	FVector	V0 = Vertices[0];
	FVector V1 = Vertices[VerticesNumber - 1];

	for (It_1 = 0; It_1 < VerticesNumber; It_1++)
	{
		V0 = Vertices[It_1];
		
		//Sutherland-Hodgeman Algorithm
		//Check if the first point is inside the clipping plane 
		if (InsideClippingPlane(V0, MinBound, MaxBound, Side))
		{
			//Check if the second point is outside the clipping plane
			if (!InsideClippingPlane(V1, MinBound, MaxBound, Side))
			{
				//The second point is outside while the first is inside:
				//The intersection point of the line defining the clipping plane should be included in the output, followed by the first point
				FVector Intersection = FindPlaneIntersection(V0, V1, MinBound, MaxBound, Side) ;
				SetArrayElement(VerticesOut, Intersection, It_2);
				It_2++;
			}

			//If both of the points are inside, only the first one is added to the output
			SetArrayElement(VerticesOut, V0, It_2);
			It_2++;
		}
		//If the first point is outside, check if the second one is inside
		else if (InsideClippingPlane(V1, MinBound, MaxBound, Side))
		{
			//If it is the intersection of the line defining the clipping plane should be included in the output
			FVector Intersection = FindPlaneIntersection(V1, V0, MinBound, MaxBound, Side);
			SetArrayElement(VerticesOut, Intersection, It_2);
			It_2++;
		}

		//If both of the ponits are outside, nothing is added to the output polygon
		V1 = V0;
	}

	Vertices = VerticesOut;
}

void USolidHeightfield::ClipPolygon(TArray<FVector>& Vertices, const FVector MinBound, const FVector MaxBound)
{
	if (Vertices.Num() > 0)
	{
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::LEFT_FACE);
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::RIGHT_FACE);
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::TOP_FACE);
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::BOTTOM_FACE);
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::FRONT_FACE);
		ClipVersusPlane(Vertices, MinBound, MaxBound, BoxSide::BACK_FACE);
	}
}

int USolidHeightfield::InsideClippingPlane(const FVector Vertex, const FVector MinBound, const FVector MaxBound, const BoxSide Side)
{
	//Check if the point is inside the bounds delimited by every plane
	//For each plane we only care about the coordinate of the axis perpendicular to it
	switch (Side)
	{
	case BoxSide::LEFT_FACE:
		return Vertex.Y >= MinBound.Y;

	case BoxSide::RIGHT_FACE:
		return Vertex.Y <= MaxBound.Y;

	case BoxSide::TOP_FACE:
		return Vertex.Z <= MaxBound.Z;

	case BoxSide::BOTTOM_FACE:
		return Vertex.Z >= MinBound.Z;

	case BoxSide::FRONT_FACE:
		return Vertex.X >= MinBound.X;

	case BoxSide::BACK_FACE:
		return Vertex.X <= MaxBound.X;
	}

	return -1;
}

FVector USolidHeightfield::FindPlaneIntersection(const FVector V0, const FVector V1, const FVector MinBound, const FVector MaxBound, const BoxSide Side)
{
	FVector IntersectionPoint(0.f, 0.f, 0.f);
	FVector PlaneOrigin(0.f, 0.f, 0.f);

	float CenterX = (MaxBound.X + MinBound.X) / 2;
	float CenterY = (MaxBound.Y + MinBound.Y) / 2;
	float CenterZ = (MaxBound.Z + MinBound.Z) / 2;

	//Retrieve the origin coordinates of every plane based on the side considered and find the intersection point
	//The normals can be easily found because the box is axis aligned
	switch (Side)
	{
	case BoxSide::LEFT_FACE:
		PlaneOrigin = FVector(CenterX, MinBound.Y, CenterZ);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(0.0f, -1.0f, 0.0f));
		break;

	case BoxSide::RIGHT_FACE:
		PlaneOrigin = FVector(CenterX, MaxBound.Y, CenterZ);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(0.0f, 1.0f, 0.0f));
		break;

	case BoxSide::TOP_FACE:
		PlaneOrigin = FVector(CenterX, CenterY, MaxBound.Z);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(0.0f, 0.0f, 1.0f));
		break;

	case BoxSide::BOTTOM_FACE:
		PlaneOrigin = FVector(CenterX, CenterY, MinBound.Z);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(0.0f, 0.0f, -1.0f));
		break;

	case BoxSide::FRONT_FACE:
		PlaneOrigin = FVector(MinBound.X, CenterY, CenterZ);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(-1.0f, 0.0f, 0.0f));
		break;

	case BoxSide::BACK_FACE:
		PlaneOrigin = FVector(MaxBound.X, CenterY, CenterZ);
		IntersectionPoint = FMath::LinePlaneIntersection(V0, V1, PlaneOrigin, FVector(1.0f, 0.0f, 0.0f));
		break;

	//Default case to make sure to notify if no intersection is found
	default:
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("No line-plane intersection found"));
		break;
	}

	return IntersectionPoint;
}

PolygonType USolidHeightfield::FilterWalkablePolygon(const TArray<FVector>& Vertices)
{
	FVector DiffAB = Vertices[1] - Vertices[0];
	FVector DiffAC = Vertices[2] - Vertices[0];

	FVector Result = FVector::CrossProduct(DiffAC, DiffAB);
	Result.Normalize();
	
	//Unreal uses the Z as up axis
    if (Result.Z > UpNormal)
	{
		return PolygonType::WALKABLE;
	}

	return PolygonType::UNWALKABLE;
}

void USolidHeightfield::CalculateUpNormal()
{
	float SlopeToRadians = FMath::Abs(MaxTraversableAngle / 180.f * PI);
	UpNormal =FMath::Cos(SlopeToRadians);

	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::SanitizeFloat(UpNormal));
}

bool USolidHeightfield::AddSpanData(int WidthIndex, int DepthIndex, int HeightIndexMin, int HeightIndexMax, PolygonType Type)
{
	//Check the boundaries of cells passed in and ignore them if they exceed them
	if (WidthIndex < 0 || WidthIndex >= Width || DepthIndex < 0 || DepthIndex >= Depth)
	{
		return false;
	}

	if (HeightIndexMin < 0 || HeightIndexMax < 0 || HeightIndexMin > HeightIndexMax)
	{
		return false;
	}

	//Initialize a new span data with the parameters passed in to the function
	UHeightSpan* NewSpan = NewObject<UHeightSpan>(UHeightSpan::StaticClass());
	NewSpan->Min = HeightIndexMin;
	NewSpan->Max = HeightIndexMax;
	NewSpan->Width = WidthIndex;
	NewSpan->Depth = DepthIndex;
	NewSpan->SpanAttribute = Type;

	//If the grid location contains no data, generate a new span and add it to the container
	int GridIndex = GetGridIndex(WidthIndex, DepthIndex);

	if (!Spans.Contains(GridIndex))
	{
		Spans.Add(GridIndex, NewSpan);
		return true;
	}

	// If a span data already exists, search the spans in the column to see which one should contain this span
	//or if a new span should be created
	UHeightSpan* CurrentSpan = *Spans.Find(GridIndex);
	UHeightSpan* PreviousSpan = nullptr;
	while (CurrentSpan)
	{
		//Check if the new span is below the current span
		if (CurrentSpan->Min > HeightIndexMax + 1)
		{
			//If it is, create a new span and insert it below the current span
			NewSpan->nextSpan = CurrentSpan;

			//If the new span is the first one in this column, insert it at the base
			if (!PreviousSpan)
			{
				Spans.Add(GridIndex, NewSpan);
			}

			//If the new span is between 2 spans, link the previous span to the new one
			else
			{
				PreviousSpan->nextSpan = NewSpan;
			}

			return true;
		}

		//Current span is below the new span
		else if (CurrentSpan->Max < HeightIndexMin - 1)
		{
			//Current span is not adjacent to new span
			if (!CurrentSpan->nextSpan)
			{
				//Locate the new span above the current one
				CurrentSpan->nextSpan = NewSpan;

				return true;
			}

			PreviousSpan = CurrentSpan;
			CurrentSpan = CurrentSpan->nextSpan;
		}

		//There's overlap or adjacency between new and current span, merge is needed
		else
		{
			if (HeightIndexMin < CurrentSpan->Min)
			{
				//Base on the condition above, set the new height min of the current span
				CurrentSpan->SetMinHeight(HeightIndexMin);
			}

			if (HeightIndexMax == CurrentSpan->Max)
			{
				//Base on the condition above, merge the span type
				CurrentSpan->SpanAttribute = Type;
				return true;
			}

			if (CurrentSpan->Max > HeightIndexMax)
			{
				//Current span is higher than new one, no need to preform any action, current one takes priority
				return true;
			}

			//If all the condition above are skipped, the new spans's maximum height is higher than the current span's maximum height
			//Need to check where the merge ends
			UHeightSpan* NextSpan = CurrentSpan->nextSpan;
			while (true)
			{		
				if (NextSpan == nullptr || NextSpan->Min > HeightIndexMax + 1)
				{
					//If there are no spans above the current one or the height increase does not affect the next span
					//the current span max and type can be directly replaced and set
					CurrentSpan->SetMaxHeight(HeightIndexMax);
					CurrentSpan->SpanAttribute = Type;

					//If current span at top of the column, remove any possible link it could have had
					if (NextSpan == nullptr)
					{
						CurrentSpan->nextSpan = nullptr;
					}
					else
					{
						CurrentSpan->nextSpan = NextSpan;
					}

					return true;
				}

				//The new height of the current span is overlapping with another span, merging needed
				//If no gap between current and next span and no overlap as well
				if (NextSpan->Min == HeightIndexMax + 1 || HeightIndexMax <= NextSpan->Max)
				{
					CurrentSpan->SetMaxHeight(NextSpan->Max);
					CurrentSpan->nextSpan = NextSpan->nextSpan;
					CurrentSpan->SpanAttribute = NextSpan->SpanAttribute;

					//If the new span has the same height of the current ne, merge the attribute
					if (HeightIndexMax == CurrentSpan->Max)
					{
						CurrentSpan->SpanAttribute = Type;
						return true;
					}

					return true;
				}

				//The current span overlaps the next one, go up in the column to see when the next will be fully included
				NextSpan = NextSpan->nextSpan;
			}
		}
	}

	//Error in the span calculation
	return false;
}

void USolidHeightfield::DrawDebugSpanData()
{
	//Iterate through all the cells
	for (int i = 0; i < Depth; i++)
	{
		for (int j = 0; j < Width; j++)
		{
			//Find the index of each span and make sure the index exist before accessing the map element
			int SpanIndex = i * Width + j;

			if (Spans.Contains(SpanIndex))
			{
				UHeightSpan* Span = *Spans.Find(SpanIndex);

				do {
					//Retrieve the location value of every span based on the bounds and the width and depth coordinates
					FVector SpanMinCoord = FVector(BoundMin.X + CellSize * j, BoundMin.Y + CellSize * i, BoundMin.Z + CellSize * Span->Min);
					FVector SpanMaxCoord = FVector(SpanMinCoord.X + CellSize, SpanMinCoord.Y + CellSize, BoundMin.Z + CellSize * Span->Max);
					FColor SpanLineColor;

					//Mark the spans with different colors based on their type
					if (Span->SpanAttribute == PolygonType::WALKABLE)
					{
						SpanLineColor = FColor::Green;
					}
					else
					{
						SpanLineColor = FColor::Red;
					}
					UUtilityDebug::DrawMinMaxBox(CurrentWorld, SpanMinCoord, SpanMaxCoord, SpanLineColor, 20.0f, 0.5f);

					Span = Span->nextSpan;

				} while (Span);
			}
		}
	}
}

void USolidHeightfield::MarkLowHeightSpan()
{
	//Iterate through all the base span
	for (auto& Span : Spans)
	{
		//As long as the current span has a next span valid
		UHeightSpan* CurrentSpan = Span.Value;

		do 
		{
			//If already unwalkable, skip
			if (CurrentSpan->SpanAttribute == PolygonType::UNWALKABLE)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				break;
			}

			//Find the height distance between the current and next span, if less than the MinTraversableHeight flag the current span as unwalkable
			int SpanFloor = CurrentSpan->Max;
			int SpanCeiling;

			if (CurrentSpan->nextSpan)
			{
				SpanCeiling = CurrentSpan->nextSpan->Min;
			}
			else
			{
				SpanCeiling = INT_MAX;
			}

			if ((SpanCeiling - SpanFloor) * CellHeight <= MinTraversableHeight)
			{
				CurrentSpan->SpanAttribute = PolygonType::UNWALKABLE;
			}

			//To iterate through all the spans, let the current span become the next and repeat the loop until valid
			CurrentSpan = CurrentSpan->nextSpan;
		} 

		while (CurrentSpan);
	}
}

void USolidHeightfield::MarkLedgeSpan()
{
	//Iterate through all the base span
	for (auto& Span : Spans)
	{
		//As long as the current span has a next span valid
		UHeightSpan* CurrentSpan = Span.Value;

		do {
			//If already unwalkable, skip
			if (CurrentSpan->SpanAttribute == PolygonType::UNWALKABLE)
			{
				CurrentSpan = CurrentSpan->nextSpan;
				break;
			}

			int CurrentFloor = CurrentSpan->Max;
			int CurrentCeiling;

			if (CurrentSpan->nextSpan)
			{
				CurrentCeiling = CurrentSpan->nextSpan->Min;
			}
			else
			{
				CurrentCeiling = INT_MAX;
			}

			//Minimum height distance from a neightbor span in unit 
			int MinHeightToNeightbor = INT_MAX;

			//Find all the adjacent grid column to the one the span considered is in 
			for (int NeightborDir = 0; NeightborDir < 4; NeightborDir++)
			{
				int NeighborWidth = CurrentSpan->Width + GetDirOffSetWidth(NeightborDir);
				int NeightborDepth = CurrentSpan->Depth + GetDirOffSetDepth(NeightborDir);

				int NeightborIndex = GetGridIndex(NeighborWidth, NeightborDepth);

				//If one of the neightbor is not valid, the span considered is on a edge, therefore is not walkable
				if (!Spans.Contains(NeightborIndex))
				{
					CurrentSpan->SpanAttribute = PolygonType::UNWALKABLE;
					break;
				}

				UHeightSpan* NeightborSpan = *Spans.Find(NeightborIndex);

				do
				{
					//Retrieve the data relative to the neightbor span (need also to take into account the area below the base span)
					//Which is represented by the default value assigned below
					int BaseNeightborFloor = -MaxTraversableStep;
					int BaseNeightborCeiling = NeightborSpan->Min;

					if ((FMath::Min(CurrentCeiling, BaseNeightborCeiling) - CurrentFloor) * CellHeight > MinTraversableHeight)
					{
						MinHeightToNeightbor = FMath::Min(MinHeightToNeightbor, (BaseNeightborFloor - CurrentFloor)) * CellHeight;
					}

					int CurrentNeightborFloor = NeightborSpan->Max;
					int CurrentNeightborCeiling;

					if (NeightborSpan->nextSpan)
					{
						CurrentNeightborCeiling = NeightborSpan->nextSpan->Min;
					}
					else
					{
						CurrentNeightborCeiling = INT_MAX;
					}

					if ((FMath::Min(CurrentCeiling, CurrentNeightborCeiling) - FMath::Max(CurrentFloor, CurrentNeightborFloor) * CellHeight) > MinTraversableHeight)
					{
						MinHeightToNeightbor = FMath::Min(MinHeightToNeightbor, (CurrentNeightborFloor - CurrentFloor)) * CellHeight;
					}

					NeightborSpan = NeightborSpan->nextSpan;
				} 
				while (NeightborSpan);

				if (MinHeightToNeightbor < -MaxTraversableStep)
				{
					CurrentSpan->SpanAttribute = PolygonType::UNWALKABLE;
				}
			}
			CurrentSpan = CurrentSpan->nextSpan;
		} 
		while (CurrentSpan);
	}
}

