#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/Texture2D.h"
#include "Texture2DArray.generated.h"

class FTextureResource;

UCLASS(hidecategories=(Object, Compositing, ImportSettings), MinimalAPI)
class UTexture2DArray : public UTexture
{
	GENERATED_UCLASS_BODY()

public:
	/** Platform data. */
	FTexturePlatformData* PlatformData;
	TMap<FString, FTexturePlatformData*> CookedPlatformData;

#if WITH_EDITORONLY_DATA
	/** A (optional) reference texture from which the texture 2D array was built */
	UPROPERTY(EditAnywhere, Category=Source2D, meta=(DisplayName="Source Texture"))
	TArray<UTexture2D*> Source2DTextures;
#endif

	ENGINE_API bool UpdateSourceFromSourceTextures();

	//~ Begin UObject Interface.
	void Serialize(FArchive& Ar) override;
	void PostLoad() override;
	void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	FString GetDesc() override;
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ End UObject Interface.

	/** Trivial accessors. */
	FORCEINLINE int32 GetSizeX() const
	{
		return PlatformData ? PlatformData->SizeX : 0;
	}
	FORCEINLINE int32 GetSizeY() const
	{
		return PlatformData ? PlatformData->SizeY : 0;
	}
	FORCEINLINE int32 GetSizeZ() const
	{
		return PlatformData ? PlatformData->NumSlices : 0;
	}
	FORCEINLINE int32 GetNumMips() const
	{
		return PlatformData ? PlatformData->Mips.Num() : 0;
	}
	FORCEINLINE EPixelFormat GetPixelFormat() const
	{
		return PlatformData ? PlatformData->PixelFormat : PF_Unknown;
	}

	//~ Begin UTexture Interface
	float GetSurfaceWidth() const override { return GetSizeX(); }
	float GetSurfaceHeight() const override { return GetSizeY(); }
	FTextureResource* CreateResource() override;
#if WITH_EDITOR
	ENGINE_API void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	void UpdateResource() override;
	EMaterialValueType GetMaterialType() const override { return MCT_Texture2DArray; }
	FTexturePlatformData** GetRunningPlatformData() override { return &PlatformData; }
	TMap<FString, FTexturePlatformData*>* GetCookedPlatformData() override { return &CookedPlatformData; }
	//~ End UTexture Interface

	/**
	 * Calculates the size of this texture in bytes if it had MipCount miplevels streamed in.
	 *
	 * @param	MipCount	Number of mips to calculate size for, counting from the smallest 1x1 mip-level and up.
	 * @return	Size of MipCount mips in bytes
	 */
	uint32 CalcTextureMemorySize(int32 MipCount) const;

	/**
	 * Calculates the size of this texture if it had MipCount miplevels streamed in.
	 *
	 * @param	Enum	Which mips to calculate size for.
	 * @return	Total size of all specified mips, in bytes
	 */
	uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const override;

#if WITH_EDITOR
	/**
	* Return maximum dimension for this texture type.
	*/
	uint32 GetMaximumDimension() const override;

#endif

	ENGINE_API static bool ShaderPlatformSupportsCompression(EShaderPlatform ShaderPlatform);

protected:

#if WITH_EDITOR
	void UpdateMipGenSettings();
#endif
};