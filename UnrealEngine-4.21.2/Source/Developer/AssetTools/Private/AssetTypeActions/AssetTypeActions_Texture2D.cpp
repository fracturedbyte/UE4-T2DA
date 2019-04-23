// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_Texture2D.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/PackageName.h"
#include "EditorStyleSet.h"
#include "Factories/SlateBrushAssetFactory.h"
#include "Slate/SlateBrushAsset.h"
#include "Factories/VolumeTextureFactory.h"
#include "Factories/Texture2DArrayFactory.h" // FB Bulgakov - Texture2D Array
#include "Engine/VolumeTexture.h"
#include "Engine/Texture2DArray.h" // FB Bulgakov - Texture2D Array
#include "AssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Texture2D::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_Texture::GetActions(InObjects, MenuBuilder);

	auto Textures = GetTypedWeakObjectPtrs<UTexture2D>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture2D_CreateSlateBrush", "Create Slate Brush"),
		LOCTEXT("Texture2D_CreateSlateBrushToolTip", "Creates a new slate brush using this texture."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.SlateBrushAsset"),
		FUIAction(FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture2D::ExecuteCreateSlateBrush, Textures ), FCanExecuteAction())
		);

	static const auto AllowVolumeTextureAssetCreationVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowVolumeTextureAssetCreation"));
	if (InObjects.Num() == 1 && AllowVolumeTextureAssetCreationVar->GetValueOnGameThread() != 0)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Texture2D_CreateVolumeTexture", "Create Volume Texture"),
			LOCTEXT("Texture2D_CreateVolumeTextureToolTip", "Creates a new volume texture using this texture."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.Sphere"),
			FUIAction(FExecuteAction::CreateSP( this, &FAssetTypeActions_Texture2D::ExecuteCreateVolumeTexture, Textures ), FCanExecuteAction())
			);
	}

	// FB Bulgakov Begin - Texture2D Array
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Texture2D_CreateTexture2DArray", "Create Texture 2D Array"),
		LOCTEXT("Texture2D_CreateTexture2DArrayTooltip", "Creates a texture 2d array from the selected textures."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.Texture2D"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAssetTypeActions_Texture2D::ExecuteCreateTexture2DArray, Textures),
			FCanExecuteAction()
		)
	);
	// FB Bulgakov End
}

void FAssetTypeActions_Texture2D::ExecuteCreateSlateBrush(TArray<TWeakObjectPtr<UTexture2D>> Objects)
{
	const FString DefaultSuffix = TEXT("_Brush");

	if( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if( Object )
		{
			// Determine the asset name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			USlateBrushAssetFactory* Factory = NewObject<USlateBrushAssetFactory>();
			Factory->InitialTexture = Object;
			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), USlateBrushAsset::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for( auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt )
		{
			auto Object = (*ObjIt).Get();
			if( Object )
			{
				// Determine the asset name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				USlateBrushAssetFactory* Factory = NewObject<USlateBrushAssetFactory>();
				Factory->InitialTexture = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), USlateBrushAsset::StaticClass(), Factory);

				if( NewAsset )
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if( ObjectsToSync.Num() > 0 )
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

void FAssetTypeActions_Texture2D::ExecuteCreateVolumeTexture(TArray<TWeakObjectPtr<UTexture2D>> Objects)
{
	const FString DefaultSuffix = TEXT("_Volume");

	if( Objects.Num() == 1 )
	{
		UTexture2D* Object = Objects[0].Get();

		if( Object )
		{
			// Determine the asset name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UVolumeTextureFactory* Factory = NewObject<UVolumeTextureFactory>();
			Factory->InitialTexture = Object;
			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UVolumeTexture::StaticClass(), Factory);
		}
	}
}

// FB Bulgakov Begin - Texture2D Array
void FAssetTypeActions_Texture2D::ExecuteCreateTexture2DArray(TArray<TWeakObjectPtr<UTexture2D>> Objects)
{
	const FString DefaultSuffix = TEXT("_2DArray");

	if (Objects.Num() > 0)
	{
		UTexture2D* Object = Objects[0].Get();

		if (Object)
		{
			// Determine the asset name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			UTexture2DArrayFactory* Factory = NewObject<UTexture2DArrayFactory>();
			for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
			{
				UTexture2D* Object = (*ObjIt).Get();
				if (Object)
					Factory->Source2DTextures.Add(Object);
			}

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UTexture2DArray::StaticClass(), Factory);
		}
	}
}
// FB Bulgakov End

#undef LOCTEXT_NAMESPACE
