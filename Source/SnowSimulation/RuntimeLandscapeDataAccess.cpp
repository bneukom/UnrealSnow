// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SnowSimulation.h"
#include "Landscape.h"

#include "RuntimeLandscapeDataAccess.h"
#include "LandscapeComponent.h"
#include "LandscapeInfo.h"

SNOWSIMULATION_API FRuntimeLandscapeComponentDataInterface::FRuntimeLandscapeComponentDataInterface(ULandscapeComponent* InComponent, FRuntimeMipAccessor* DataInterface, int32 MipLevel = 0) :
	Component(InComponent),
	HeightMipData(NULL),
	XYOffsetMipData(NULL),
	bNeedToDeleteDataInterface(false),
	MipLevel(MipLevel)
{
	// Offset and stride for this component's data in heightmap texture
	HeightmapStride = Component->HeightmapTexture->GetSizeX() >> MipLevel;
	HeightmapComponentOffsetX = FMath::RoundToInt((float)(Component->HeightmapTexture->GetSizeX() >> MipLevel) * Component->HeightmapScaleBias.Z);
	HeightmapComponentOffsetY = FMath::RoundToInt((float)(Component->HeightmapTexture->GetSizeY() >> MipLevel) * Component->HeightmapScaleBias.W);
	HeightmapSubsectionOffset = (Component->SubsectionSizeQuads + 1) >> MipLevel;

	ComponentSizeVerts = (Component->ComponentSizeQuads + 1) >> MipLevel;
	SubsectionSizeVerts = (Component->SubsectionSizeQuads + 1) >> MipLevel;
	ComponentNumSubsections = Component->NumSubsections;

	
	if (MipLevel < Component->HeightmapTexture->GetNumMips())
	{
		HeightMipData = (FColor*)DataInterface->LockMip(Component->HeightmapTexture, MipLevel);
		if (Component->XYOffsetmapTexture)
		{
			XYOffsetMipData = (FColor*)DataInterface->LockMip(Component->XYOffsetmapTexture, MipLevel);
		}
	}
}

SNOWSIMULATION_API FRuntimeLandscapeComponentDataInterface::~FRuntimeLandscapeComponentDataInterface()
{
	
}


SNOWSIMULATION_API void FRuntimeLandscapeComponentDataInterface::GetHeightmapTextureData(TArray<FColor>& OutData, bool bOkToFail)
{
	if (bOkToFail && !HeightMipData)
	{
		OutData.Empty();
		return;
	}
#if LANDSCAPE_VALIDATE_DATA_ACCESS
	check(HeightMipData);
#endif
	int32 HeightmapSize = ((Component->SubsectionSizeQuads + 1) * Component->NumSubsections) >> MipLevel;
	OutData.Empty(FMath::Square(HeightmapSize));
	OutData.AddUninitialized(FMath::Square(HeightmapSize));

	for (int32 SubY = 0; SubY < HeightmapSize; SubY++)
	{
		// X/Y of the vertex we're looking at in component's coordinates.
		int32 CompY = SubY;

		// UV coordinates of the data offset into the texture
		int32 TexV = SubY + HeightmapComponentOffsetY;

		// Copy the data
		FMemory::Memcpy(&OutData[CompY * HeightmapSize], &HeightMipData[HeightmapComponentOffsetX + TexV * HeightmapStride], HeightmapSize * sizeof(FColor));
	}
}

SNOWSIMULATION_API FColor* FRuntimeLandscapeComponentDataInterface::GetXYOffsetData(int32 LocalX, int32 LocalY) const
{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
	check(Component);
	check(LocalX >= 0 && LocalY >= 0 && LocalX < Component->ComponentSizeQuads + 1 && LocalY < Component->ComponentSizeQuads + 1);
#endif

	const int32 WeightmapSize = (Component->SubsectionSizeQuads + 1) * Component->NumSubsections;
	int32 SubNumX;
	int32 SubNumY;
	int32 SubX;
	int32 SubY;
	ComponentXYToSubsectionXY(LocalX, LocalY, SubNumX, SubNumY, SubX, SubY);

	return &XYOffsetMipData[SubX + SubNumX*SubsectionSizeVerts + (SubY + SubNumY*SubsectionSizeVerts)*WeightmapSize];
}

SNOWSIMULATION_API FVector FRuntimeLandscapeComponentDataInterface::GetLocalVertex(int32 LocalX, int32 LocalY) const
{
	const float ScaleFactor = (float)Component->ComponentSizeQuads / (float)(ComponentSizeVerts - 1);
	float XOffset, YOffset;
	GetXYOffset(LocalX, LocalY, XOffset, YOffset);
	return FVector(LocalX * ScaleFactor + XOffset, LocalY * ScaleFactor + YOffset, RuntimeLandscapeDataAccess::GetLocalHeight(GetHeight(LocalX, LocalY)));
}

SNOWSIMULATION_API FVector FRuntimeLandscapeComponentDataInterface::GetWorldVertex(int32 LocalX, int32 LocalY) const
{
	return Component->ComponentToWorld.TransformPosition(GetLocalVertex(LocalX, LocalY));
}

SNOWSIMULATION_API void FRuntimeLandscapeComponentDataInterface::GetWorldTangentVectors(int32 LocalX, int32 LocalY, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const
{
	FColor* Data = GetHeightData(LocalX, LocalY);
	WorldTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
	WorldTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
	WorldTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(WorldTangentZ.X) + FMath::Square(WorldTangentZ.Y)));
	WorldTangentX = FVector(-WorldTangentZ.Z, 0.f, WorldTangentZ.X);
	WorldTangentY = FVector(0.f, WorldTangentZ.Z, -WorldTangentZ.Y);

	WorldTangentX = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentX);
	WorldTangentY = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentY);
	WorldTangentZ = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentZ);
}

SNOWSIMULATION_API void FRuntimeLandscapeComponentDataInterface::GetWorldPositionTangents(int32 LocalX, int32 LocalY, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ) const
{
	FColor* Data = GetHeightData(LocalX, LocalY);

	WorldTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
	WorldTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
	WorldTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(WorldTangentZ.X) + FMath::Square(WorldTangentZ.Y)));
	WorldTangentX = FVector(WorldTangentZ.Z, 0.f, -WorldTangentZ.X);
	WorldTangentY = WorldTangentZ ^ WorldTangentX;

	uint16 Height = (Data->R << 8) + Data->G;

	const float ScaleFactor = (float)Component->ComponentSizeQuads / (float)(ComponentSizeVerts - 1);
	float XOffset, YOffset;
	GetXYOffset(LocalX, LocalY, XOffset, YOffset);
	WorldPos = Component->ComponentToWorld.TransformPosition(FVector(LocalX * ScaleFactor + XOffset, LocalY * ScaleFactor + YOffset, RuntimeLandscapeDataAccess::GetLocalHeight(Height)));
	WorldTangentX = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentX);
	WorldTangentY = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentY);
	WorldTangentZ = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentZ);
}

