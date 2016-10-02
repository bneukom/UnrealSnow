#pragma once
#include "Engine/Texture2D.h"

void UpdateTexture(UTexture2D* Texture, TArray<FColor>& TextureData)
{
	FUpdateTextureRegion2D* RegionData = new FUpdateTextureRegion2D(0, 0, 0, 0, Texture->GetSizeX(), Texture->GetSizeY());

	auto CleanupFunction = [](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
	{
		delete Regions;
	};

	// Update the texture
	Texture->UpdateTextureRegions(
		0, 1,
		RegionData, Texture->GetSizeX() * 4, 4,
		(uint8*)TextureData.GetData(),
		CleanupFunction
		);
}