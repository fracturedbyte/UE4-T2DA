#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "Texture2DArrayFactory.generated.h"

UCLASS(hidecategories = Object, MinimalAPI)
class UTexture2DArrayFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<class UTexture2D*> Source2DTextures;

	//~ Begin UFactory Interface
	FText GetDisplayName() const override;
	bool ConfigureProperties() override;
	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface
};