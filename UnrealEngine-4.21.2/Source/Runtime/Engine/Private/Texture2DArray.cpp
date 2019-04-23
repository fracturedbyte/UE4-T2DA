#include "Engine/Texture2DArray.h"
#include "RenderUtils.h"
#include "TextureResource.h"
#include "EngineUtils.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Containers/ResourceArray.h"

const int32 MAX_TEXTURE_2D_ARRAY_SLICES = 512;

UTexture2DArray::UTexture2DArray(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SRGB = true;
}

bool UTexture2DArray::UpdateSourceFromSourceTextures()
{
	bool bSourceValid = false;

#if WITH_EDITOR
	int32 NumSlices = Source2DTextures.Num();
	if (NumSlices > 0)
	{
		// Check source textures
		bool bSourceTexturesAreValid = true;
		int32 SizeX = -1;
		int32 SizeY = -1;
		int32 NumMips = -1;
		ETextureSourceFormat TextureFormat = TSF_Invalid;
		for (UTexture2D* TextureSlice : Source2DTextures)
		{
			if (!TextureSlice)
			{
				bSourceTexturesAreValid = false;
				break;
			}

			FTextureSource& InitialSource = TextureSlice->Source;
			int32 SliceSizeX = InitialSource.GetSizeX();
			int32 SliceSizeY = InitialSource.GetSizeY();
			int32 SliceNumMips = InitialSource.GetNumMips();
			ETextureSourceFormat SliceFormat = InitialSource.GetFormat();

			if (SizeX == -1)
				SizeX = SliceSizeX;
			if (SizeY == -1)
				SizeY = SliceSizeY;
			if (NumMips == -1)
				NumMips = SliceNumMips;
			if (TextureFormat == TSF_Invalid)
				TextureFormat = SliceFormat;

			if (SizeX != SliceSizeX ||
				SizeY != SliceSizeY ||
				NumMips != SliceNumMips ||
				TextureFormat != SliceFormat)
			{
				bSourceTexturesAreValid = false;
				break;
			}
		}

		if (bSourceTexturesAreValid)
		{
			const FPixelFormatInfo& InitialFormat = GPixelFormats[TextureFormat];
			const int32 FormatDataSize = InitialFormat.BlockBytes;
			const int32 OneTextureSize = SizeX * SizeY * FormatDataSize;


			uint8* TextureData = (uint8*)FMemory::Malloc(OneTextureSize * NumSlices);
			uint8* CurPos = TextureData;

			for (UTexture2D* TextureSlice : Source2DTextures)
			{
				TArray<uint8> Ref2DData;
				FTextureSource& InitialSource = TextureSlice->Source;
				if (InitialSource.GetMipData(Ref2DData, 0))
				{
					FMemory::Memcpy(CurPos, Ref2DData.GetData(), OneTextureSize);
				}

				CurPos += OneTextureSize;
			}


			Source.Init(SizeX, SizeY, NumSlices, 1, TextureFormat, TextureData);
			bSourceValid = true;

			FMemory::Free(TextureData);
		}
	}

	if (bSourceValid)
	{
		SetLightingGuid(); // Because the content has changed, use a new GUID.
	}
	else
	{
		Source.Init(0, 0, 0, 0, TSF_Invalid, nullptr);
	}

	UpdateMipGenSettings();
#endif // WITH_EDITOR

	return bSourceValid;
}

//~ Begin UObject Interface.

void UTexture2DArray::Serialize(FArchive& Ar)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UTexture2DArray::Serialize"), STAT_Texture2DArray_Serialize, STATGROUP_LoadTime);

	Super::Serialize(Ar);

	FStripDataFlags StripFlags(Ar);
	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (bCooked || Ar.IsCooking())
	{
		SerializeCookedPlatformData(Ar);
	}

#if WITH_EDITOR
	if (Ar.IsLoading() && !Ar.IsTransacting() && !bCooked)
	{
		BeginCachePlatformData();
	}
#endif // #if WITH_EDITOR
}

void UTexture2DArray::PostLoad()
{
#if WITH_EDITOR
	FinishCachePlatformData();

	UpdateSourceFromSourceTextures();
#endif // #if WITH_EDITOR

	Super::PostLoad();
}

void UTexture2DArray::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if WITH_EDITOR
	int32 SizeX = Source.GetSizeX();
	int32 SizeY = Source.GetSizeY();
	int32 SizeZ = Source.GetNumSlices();
#else
	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 SizeZ = 0;
#endif

	const FString Dimensions = FString::Printf(TEXT("%dx%dx%d"), SizeX, SizeY, SizeZ);
	OutTags.Add(FAssetRegistryTag("Dimensions", Dimensions, FAssetRegistryTag::TT_Dimensional));
	OutTags.Add(FAssetRegistryTag("Format", GPixelFormats[GetPixelFormat()].Name, FAssetRegistryTag::TT_Alphabetical));

	Super::GetAssetRegistryTags(OutTags);
}

FString UTexture2DArray::GetDesc()
{
	return FString::Printf(TEXT("Texture 2D Array: %dx%dx%d [%s]"),
		GetSizeX(),
		GetSizeY(),
		GetSizeZ(),
		GPixelFormats[GetPixelFormat()].Name
		);
}

void UTexture2DArray::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);
	CumulativeResourceSize.AddUnknownMemoryBytes(CalcTextureMemorySizeEnum(TMC_ResidentMips));
}

//~ End UObject Interface.

//~ Begin UTexture Interface

class FTexture2DArray2BulkData : public FResourceBulkDataInterface
{
public:
	FTexture2DArray2BulkData(int32 InFirstMip, int32 InNumSlices)
		: FirstMip(InFirstMip)
		, NumSlices(InNumSlices)
	{
		FMemory::Memzero(MipData, sizeof(MipData));
		FMemory::Memzero(MipSize, sizeof(MipSize));
	}

	~FTexture2DArray2BulkData()
	{
		Discard();
	}

	const void* GetResourceBulkData() const override
	{
		return MipData[FirstMip];
	}

	uint32 GetResourceBulkDataSize() const override
	{
		return MipSize[FirstMip];
	}

	void Discard() override
	{
		for (int32 MipIndex = 0; MipIndex < MAX_TEXTURE_MIP_COUNT; ++MipIndex)
		{
			if (MipData[MipIndex])
			{
				FMemory::Free(MipData[MipIndex]);
				MipData[MipIndex] = nullptr;
			}
			MipSize[MipIndex] = 0;
		}
	}

	void** GetMipData() { return MipData; }
	uint32* GetMipSize() { return MipSize; }
	int32 GetFirstMip() const { return FirstMip; }

	void MergeMips(int32 NumMips)
	{
		check(NumMips < MAX_TEXTURE_MIP_COUNT);

		uint64 MergedSize = 0;
		for (int32 MipIndex = FirstMip; MipIndex < NumMips; ++MipIndex)
		{
			MergedSize += MipSize[MipIndex];
		}

		// Don't do anything if there is nothing to merge
		if (MergedSize > MipSize[FirstMip])
		{
			uint8* MergedAlloc = (uint8*)FMemory::Malloc(MergedSize);
			uint8* CurrPos = MergedAlloc;
			for (int32 TextureSlice = 0; TextureSlice < NumSlices; ++TextureSlice)
			{
				for (int32 MipIndex = FirstMip; MipIndex < NumMips; ++MipIndex)
				{
					int32 MipSliceSize = MipSize[MipIndex] / NumSlices;
					int32 SliceOffset = TextureSlice * MipSliceSize;
					if (MipData[MipIndex])
					{
						FMemory::Memcpy(CurrPos, (uint8*)MipData[MipIndex] + SliceOffset, MipSliceSize);
					}
					CurrPos += MipSliceSize;
				}
			}

			Discard();

			MipData[FirstMip] = MergedAlloc;
			MipSize[FirstMip] = MergedSize;
		}
	}

protected:

	void* MipData[MAX_TEXTURE_MIP_COUNT];
	uint32 MipSize[MAX_TEXTURE_MIP_COUNT];
	int32 FirstMip;
	int32 NumSlices;
};

class FTexture2DArray2Resource : public FTextureResource
{
public:
	FTexture2DArray2Resource(UTexture2DArray* InTexture2DArray, int32 MipBias)
		: SizeX(InTexture2DArray->GetSizeX())
		, SizeY(InTexture2DArray->GetSizeY())
		, SizeZ(InTexture2DArray->GetSizeZ())
		, CurrentFirstMip(INDEX_NONE)
		, NumMips(InTexture2DArray->GetNumMips())
		, PixelFormat(InTexture2DArray->GetPixelFormat())
		, TextureSize(0)
		, TextureReference(&InTexture2DArray->TextureReference)
		, InitialData(MipBias, InTexture2DArray->GetSizeZ())
	{
		check(0 < NumMips && NumMips <= MAX_TEXTURE_MIP_COUNT);
		check(0 <= MipBias && MipBias < NumMips);

		STAT(LODGroupStatName = TextureGroupStatFNames[InTexture2DArray->LODGroup]);
		TextureName = InTexture2DArray->GetFName();

		CreationFlags = (InTexture2DArray->SRGB ? TexCreate_SRGB : 0) | TexCreate_OfflineProcessed | TexCreate_ShaderResource | (InTexture2DArray->bNoTiling ? TexCreate_NoTiling : 0);
		SamplerFilter = (ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(InTexture2DArray);

		bGreyScaleFormat = (PixelFormat == PF_G8) || (PixelFormat == PF_BC4);

		FTexturePlatformData* PlatformData = InTexture2DArray->PlatformData;
		if (PlatformData && PlatformData->TryLoadMips(MipBias, InitialData.GetMipData() + MipBias))
		{
			for (int32 MipIndex = MipBias; MipIndex < NumMips; ++MipIndex)
			{
				const FTexture2DMipMap& MipMap = PlatformData->Mips[MipIndex];

				// The bulk data can be bigger because of memory alignment constraints on each slice and mips.
				InitialData.GetMipSize()[MipIndex] = FMath::Max<int32>(
					MipMap.BulkData.GetBulkDataSize(),
					CalcTextureMipMapSize(SizeX, SizeY, (EPixelFormat)PixelFormat, MipIndex) * SizeZ
					);
			}
		}
	}

	~FTexture2DArray2Resource() {}

	void InitRHI() override
	{
		INC_DWORD_STAT_BY(STAT_TextureMemory, TextureSize);
		INC_DWORD_STAT_FNAME_BY(LODGroupStatName, TextureSize);

		CurrentFirstMip = InitialData.GetFirstMip();

		// Create the RHI texture.
		{
			FRHIResourceCreateInfo CreateInfo;
			InitialData.MergeMips(NumMips);
			CreateInfo.BulkData = &InitialData;

			const uint32 BaseMipSizeX = FMath::Max<uint32>(SizeX >> CurrentFirstMip, 1);
			const uint32 BaseMipSizeY = FMath::Max<uint32>(SizeY >> CurrentFirstMip, 1);

			Texture2DArrayRHI = RHICreateTexture2DArray(BaseMipSizeX, BaseMipSizeY, SizeZ, PixelFormat, NumMips - CurrentFirstMip, CreationFlags, CreateInfo);
			TextureRHI = Texture2DArrayRHI;
		}

		TextureRHI->SetName(TextureName);
		RHIBindDebugLabelName(TextureRHI, *TextureName.ToString());

		if (TextureReference)
		{
			RHIUpdateTextureReference(TextureReference->TextureReferenceRHI, TextureRHI);
		}

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer
		(
			SamplerFilter,
			AM_Clamp,
			AM_Clamp,
			AM_Clamp
		);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}

	void ReleaseRHI() override
	{
		DEC_DWORD_STAT_BY(STAT_TextureMemory, TextureSize);
		DEC_DWORD_STAT_FNAME_BY(LODGroupStatName, TextureSize);
		if (TextureReference)
		{
			RHIUpdateTextureReference(TextureReference->TextureReferenceRHI, FTextureRHIParamRef());
		}
		Texture2DArrayRHI.SafeRelease();
		FTextureResource::ReleaseRHI();
	}

	uint32 GetSizeX() const override
	{
		return FMath::Max<uint32>(SizeX >> CurrentFirstMip, 1);
	}

	uint32 GetSizeY() const override
	{
		return FMath::Max<uint32>(SizeY >> CurrentFirstMip, 1);
	}

private:

#if STATS
	/** The FName of the LODGroup-specific stat	*/
	FName LODGroupStatName;
#endif
	/** The FName of the texture asset */
	FName TextureName;

	/** Dimension X of the resource	*/
	uint32 SizeX;
	/** Dimension Y of the resource	*/
	uint32 SizeY;
	/** Dimension Z of the resource	*/
	uint32 SizeZ;
	/** The first mip cached in the resource. */
	int32 CurrentFirstMip;
	/** Num of mips of the texture */
	int32 NumMips;
	/** Format of the texture */
	uint8 PixelFormat;
	/** Creation flags of the texture */
	uint32 CreationFlags;
	/** Cached texture size for stats. */
	int32 TextureSize;

	/** The filtering to use for this texture */
	ESamplerFilter SamplerFilter;

	/** A reference to the texture's RHI resource as a texture 2D array. */
	FTexture2DArrayRHIRef Texture2DArrayRHI;

	FTextureReference* TextureReference;

	FTexture2DArray2BulkData InitialData;
};

FTextureResource* UTexture2DArray::CreateResource()
{
	const FPixelFormatInfo& FormatInfo = GPixelFormats[GetPixelFormat()];
	const bool bCompressedFormat = FormatInfo.BlockSizeX > 1;
	const bool bFormatIsSupported = FormatInfo.Supported && (!bCompressedFormat || ShaderPlatformSupportsCompression(GMaxRHIShaderPlatform));

	if (GetNumMips() > 0 && GSupportsTexture2DArray && bFormatIsSupported)
	{
		return new FTexture2DArray2Resource(this, GetCachedLODBias());
	}
	else if (GetNumMips() == 0)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s contains no miplevels! Please delete."), *GetFullName());
	}
	else if (!GSupportsTexture3D)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s cannot be created, rhi does not support 3d textures."), *GetFullName());
	}
	else if (!bFormatIsSupported)
	{
		UE_LOG(LogTexture, Warning, TEXT("%s cannot be created, rhi does not support format %s."), *GetFullName(), FormatInfo.Name);
	}
	return nullptr;
}

#if WITH_EDITOR

void UTexture2DArray::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyChangedEvent.Property)
	{
		static const FName SourceTextureName("Source2DTextures");

		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if (PropertyName == SourceTextureName)
		{
			UpdateSourceFromSourceTextures();
		}
	}

	UpdateMipGenSettings();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // #if WITH_EDITOR

void UTexture2DArray::UpdateResource()
{
#if WITH_EDITOR
	// Recache platform data if the source has changed.
	CachePlatformData();
#endif // #if WITH_EDITOR

	// Route to super.
	Super::UpdateResource();
}

//~ End UTexture Interface

uint32 UTexture2DArray::CalcTextureMemorySize(int32 MipCount) const
{
	uint32 Size = 0;
	if (PlatformData)
	{
		const EPixelFormat Format = GetPixelFormat();
		const uint32 Flags = (SRGB ? TexCreate_SRGB : 0) | TexCreate_OfflineProcessed | (bNoTiling ? TexCreate_NoTiling : 0);

		
		FIntPoint SizeXY;
		uint32 SizeZ = GetSizeZ();
		SizeXY = CalcMipMapExtent(GetSizeX(), GetSizeY(), Format, FMath::Max<int32>(0, GetNumMips() - MipCount));

		uint32 TextureAlign = 0;
		Size = (uint32)RHICalcTexture2DPlatformSize(SizeXY.X, SizeXY.Y, SizeZ, Format, MipCount, Flags, TextureAlign);
	}
	return Size;
}

uint32 UTexture2DArray::CalcTextureMemorySizeEnum(ETextureMipCount Enum) const
{
	if (Enum == TMC_ResidentMips || Enum == TMC_AllMipsBiased)
	{
		return CalcTextureMemorySize(GetNumMips() - GetCachedLODBias());
	}
	else
	{
		return CalcTextureMemorySize(GetNumMips());
	}
}

#if WITH_EDITOR

uint32 UTexture2DArray::GetMaximumDimension() const
{
	return GetMax2DTextureDimension();
}

#endif

bool UTexture2DArray::ShaderPlatformSupportsCompression(EShaderPlatform ShaderPlatform)
{
	switch (ShaderPlatform)
	{
	case SP_PCD3D_SM4:
	case SP_PCD3D_SM5:
	case SP_PS4:
	case SP_XBOXONE_D3D12:
	case SP_VULKAN_SM5:
	case SP_VULKAN_SM4:
	case SP_VULKAN_SM5_LUMIN:
		return true;

	default:
		return false;
	}
}

#if WITH_EDITOR

void UTexture2DArray::UpdateMipGenSettings()
{
	if (PowerOfTwoMode == ETexturePowerOfTwoSetting::None && (!Source.IsPowerOfTwo() || !FMath::IsPowerOfTwo(Source.NumSlices)))
	{
		// Force NPT textures to have no mipmaps.
		MipGenSettings = TMGS_NoMipmaps;
		NeverStream = true;
	}
}

#endif // #if WITH_EDITOR