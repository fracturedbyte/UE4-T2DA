#include "Factories/Texture2DArrayFactory.h"
#include "Styling/SlateBrush.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Engine/Texture2DArray.h"

#define LOCTEXT_NAMESPACE "Texture2DArrayFactory"

UTexture2DArrayFactory::UTexture2DArrayFactory(const FObjectInitializer& ObjectInitializer)
 : Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTexture2DArray::StaticClass();
}

FText UTexture2DArrayFactory::GetDisplayName() const
{
	return LOCTEXT("Texture2DArrayFactoryDescription", "Texture 2D Array");
}

bool UTexture2DArrayFactory::ConfigureProperties()
{
	return true;
}

UObject* UTexture2DArrayFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UTexture2DArray* NewTexture2DArray = NewObject<UTexture2DArray>(InParent, Name, Flags);

	NewTexture2DArray->MipGenSettings = TMGS_FromTextureGroup;
	NewTexture2DArray->NeverStream = true;
	NewTexture2DArray->CompressionNone = false;

	NewTexture2DArray->Source2DTextures = Source2DTextures;

	NewTexture2DArray->UpdateSourceFromSourceTextures();
	NewTexture2DArray->UpdateResource();

	return NewTexture2DArray;
}

#undef LOCTEXT_NAMESPACE