#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "MaterialExpressionTextureSampleParameter2DArray.generated.h"

class UTexture;

UCLASS(collapsecategories, hidecategories=Object)
class ENGINE_API UMaterialExpressionTextureSampleParameter2DArray : public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	void GetCaption(TArray<FString>& OutCaptions) const override;
#endif // WITH_EDITOR
	//~ End UMaterialExpression Interface

	//~ Begin UMaterialExpressionTextureSampleParameter Interface
	bool TextureIsValid(UTexture* InTexture) override;
	const TCHAR* GetRequirements() override;
	void SetDefaultTexture() override;
	//~ End UMaterialExpressionTextureSampleParameter Interface
};