#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2DArray.h"
#include "AssetTypeActions/AssetTypeActions_Texture.h"

class FAssetTypeActions_Texture2DArray : public FAssetTypeActions_Texture
{
public:
	// IAssetTypeActions Implementation
	FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Texture2DArray", "Texture2D Array"); }
	FColor GetTypeColor() const override { return FColor(128, 192, 64); }
	UClass* GetSupportedClass() const override { return UTexture2DArray::StaticClass(); }
	bool CanFilter() override { return true; }
	bool IsImportedAsset() const override { return false; }
};