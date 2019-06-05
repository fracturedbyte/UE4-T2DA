// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMeshPaintGeometryAdapter.h"
#include "TMeshPaintOctree.h"

/** Base mesh paint geometry adapter, handles basic sphere intersection using a Octree */
class MESHPAINT_API FBaseMeshPaintGeometryAdapter : public IMeshPaintGeometryAdapter
{
public:
	/** Start IMeshPaintGeometryAdapter Overrides */
	virtual bool Initialize() override;
	virtual const TArray<FVector>& GetMeshVertices() const override;
	virtual const TArray<uint32>& GetMeshIndices() const override;
	virtual void GetVertexPosition(int32 VertexIndex, FVector& OutVertex) const override;
	// FB Bulgakov Begin - Texture2D Array
	virtual TArray<uint32> SphereIntersectTriangles(const float ComponentSpaceSquaredBrushRadius, const FVector& ComponentSpaceBrushPosition, const FVector& ComponentSpaceBrushNormal, const FVector& ComponentSpaceCameraPosition, const class UPaintBrushSettings* BrushSettings) const override;
	virtual void GetInfluencedVertexIndices(const float ComponentSpaceSquaredBrushRadius, const FVector& ComponentSpaceBrushPosition, const FVector& ComponentSpaceBrushNormal, const FVector& ComponentSpaceCameraPosition, const class UPaintBrushSettings* BrushSettings, TSet<int32> &InfluencedVertices) const override;
	virtual void GetInfluencedVertexData(const float ComponentSpaceSquaredBrushRadius, const FVector& ComponentSpaceBrushPosition, const FVector& ComponentSpaceBrushNormal, const FVector& ComponentSpaceCameraPosition, const class UPaintBrushSettings* BrushSettings, TArray<TPair<int32, FVector>>& OutData) const override;
	virtual TArray<FVector> SphereIntersectVertices(const float ComponentSpaceSquaredBrushRadius, const FVector& ComponentSpaceBrushPosition, const FVector& ComponentSpaceBrushNormal, const FVector& ComponentSpaceCameraPosition, const class UPaintBrushSettings* BrushSettings) const override;
	// FB Bulgakov End
	/** End IMeshPaintGeometryAdapter Overrides */

	virtual bool InitializeVertexData() = 0;
protected:
	bool BuildOctree();
protected:
	/** Index and Vertex data populated by derived classes in InitializeVertexData */
	TArray<FVector> MeshVertices;
	TArray<uint32> MeshIndices;
	/** Octree used for reducing the cost of sphere intersecting with triangles / vertices */
	TUniquePtr<FMeshPaintTriangleOctree> MeshTriOctree;
};
