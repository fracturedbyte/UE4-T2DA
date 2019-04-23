#pragma once

#include "CoreMinimal.h"
#include "BatchedElements.h"

class UNREALED_API FBatchedElementTexture2DArrayPreviewParameters : public FBatchedElementParameters
{
public:
	FBatchedElementTexture2DArrayPreviewParameters(float InMipLevel, float InTextureSlice)
		: MipLevel(InMipLevel)
		, TextureSlice(InTextureSlice)
	{}

	void BindShaders(FRHICommandList& RHICmdList, FGraphicsPipelineStateInitializer& GraphicsPSOInit, ERHIFeatureLevel::Type InFeatureLevel, const FMatrix& InTransform, const float InGamma, const FMatrix& ColorWeights, const FTexture* Texture) override;

private:
	float MipLevel;
	float TextureSlice;
};