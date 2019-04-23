#include "Texture2DArrayPreview.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "SimpleElementShaders.h"
#include "ShaderParameterUtils.h"
#include "PipelineStateCache.h"

class FSimpleElementTexture2DArrayPreviewPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementTexture2DArrayPreviewPS,Global);
public:

	FSimpleElementTexture2DArrayPreviewPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"), SPF_Mandatory);
		InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));	
		TextureComponentReplicate.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicate"));
		TextureComponentReplicateAlpha.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicateAlpha"));
		ColorWeights.Bind(Initializer.ParameterMap,TEXT("ColorWeights"));
		PackedParameters.Bind(Initializer.ParameterMap,TEXT("PackedParams"));
	}
	FSimpleElementTexture2DArrayPreviewPS() {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4) && !IsConsolePlatform(Parameters.Platform);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FTexture* TextureValue, const FMatrix& ColorWeightsValue, float GammaValue, float MipLevel, float TextureSlice)
	{
		SetTextureParameter(RHICmdList, GetPixelShader(),InTexture,InTextureSampler,TextureValue);
		SetShaderValue(RHICmdList, GetPixelShader(),ColorWeights,ColorWeightsValue);
		FVector4 PackedParametersValue(GammaValue, MipLevel, TextureSlice, 0.0f);
		SetShaderValue(RHICmdList, GetPixelShader(), PackedParameters, PackedParametersValue);

		SetShaderValue(RHICmdList, GetPixelShader(),TextureComponentReplicate,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,0));
		SetShaderValue(RHICmdList, GetPixelShader(),TextureComponentReplicateAlpha,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,1));
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InTexture;
		Ar << InTextureSampler;
		Ar << TextureComponentReplicate;
		Ar << TextureComponentReplicateAlpha;
		Ar << ColorWeights;
		Ar << PackedParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter TextureComponentReplicate;
	FShaderParameter TextureComponentReplicateAlpha;
	FShaderParameter ColorWeights; 
	FShaderParameter PackedParameters;
};

IMPLEMENT_SHADER_TYPE(, FSimpleElementTexture2DArrayPreviewPS, TEXT("/Engine/Private/SimpleElementTexture2DArrayPreviewPixelShader.usf"), TEXT("Main"), SF_Pixel);

void FBatchedElementTexture2DArrayPreviewParameters::BindShaders(
	FRHICommandList& RHICmdList,
	FGraphicsPipelineStateInitializer& GraphicsPSOInit,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FMatrix& InTransform,
	const float InGamma,
	const FMatrix& ColorWeights,
	const FTexture* Texture)
{
	TShaderMapRef<FSimpleElementVS> VertexShader(GetGlobalShaderMap(InFeatureLevel));
	TShaderMapRef<FSimpleElementTexture2DArrayPreviewPS> PixelShader(GetGlobalShaderMap(InFeatureLevel));

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GSimpleElementVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, EApplyRendertargetOption::ForceApply);

	VertexShader->SetParameters(RHICmdList, InTransform);
	PixelShader->SetParameters(RHICmdList, Texture, ColorWeights, InGamma, MipLevel, TextureSlice);
}