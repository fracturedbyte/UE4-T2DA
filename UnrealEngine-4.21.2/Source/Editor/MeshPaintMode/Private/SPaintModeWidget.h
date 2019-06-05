// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
// FB Bulgakov Begin - Texture2D Array
#include "Widgets/Views/STileView.h"
#include "Engine/Texture2DArray.h"
// FB Bulgakov End


class FPaintModePainter;
class UPaintModeSettings;
class IDetailsView;

// FB Bulgakov Begin - Texture2D Array
class FTextureArrayPaletteItemModel : public TSharedFromThis<FTextureArrayPaletteItemModel>
{
public:
	FTextureArrayPaletteItemModel(UTexture2D* InTexture2D, int32 InTextureId, TSharedRef<class SPaintModeWidget> InPaintModeWidget, TSharedPtr<class FAssetThumbnailPool> InThumbnailPool, FPaintModePainter* InMeshPainter);

	TSharedRef<SWidget> GetThumbnailWidget() const
	{
		return ThumbnailWidget.ToSharedRef();
	}

	int32 GetTextureId() const
	{
		return TextureId;
	}

private:
	TSharedPtr<SWidget> ThumbnailWidget;
	FName DisplayFName;
	UTexture2D* Texture2D;
	int32 TextureId;
	FPaintModePainter* MeshPainter;
};

struct FTextureArrayMeshUIInfo
{
	UTexture2DArray* TextureArray;
	int32 ID;

	FTextureArrayMeshUIInfo(UTexture2DArray* InTextureArray, int32 InID)
		: TextureArray(InTextureArray)
		, ID(InID)
	{}

	bool operator==(const FTextureArrayMeshUIInfo& Other) const
	{
		return TextureArray == Other.TextureArray && ID == Other.ID;
	}

	FText GetNameText() const
	{
		FName DisplayFName(TEXT("InvalidTexture"));
		
		if (TextureArray && TextureArray->Source2DTextures.IsValidIndex(ID))
		{
			UTexture2D* Texture = TextureArray->Source2DTextures[ID];
			if (Texture)
			{
				DisplayFName = Texture->GetFName();
			}
		}

		return FText::FromName(DisplayFName);
	}
};

typedef TSharedPtr<FTextureArrayMeshUIInfo> FTextureArrayMeshUIInfoPtr;

class STextureArrayPaletteItemTile : public STableRow<FTextureArrayMeshUIInfoPtr>
{
public:
	SLATE_BEGIN_ARGS(STextureArrayPaletteItemTile) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FTextureArrayPaletteItemModel>& InModel);

private:
	TSharedPtr<FTextureArrayPaletteItemModel> Model;
};

typedef TSharedPtr<FTextureArrayPaletteItemModel> FTextureArrayPaletteItemModelPtr;
typedef STileView<FTextureArrayPaletteItemModelPtr> STextureArrayTypeTileView;
// FB Bulgakov End

/** Widget representing the state / functionality and settings for PaintModePainter*/
class SPaintModeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPaintModeWidget) {}
	SLATE_END_ARGS()

	/** Slate widget construction */
	void Construct(const FArguments& InArgs, FPaintModePainter* InPainter);

protected:
	/** Creates and sets up details view */
	void CreateDetailsView();
	
	/** Returns a widget comprising special UI elements  for vertex color painting */
	TSharedPtr<SWidget> CreateVertexPaintWidget();

	/** Returns a widget comprising UI elements for texture painting */
	TSharedPtr<SWidget> CreateTexturePaintWidget();	

	/** Returns the toolbar widget instance */
	TSharedPtr<SWidget> CreateToolBarWidget();

	/** Getters for whether or not a specific mode is visible */
	EVisibility IsVertexPaintModeVisible() const;
	EVisibility IsTexturePaintModeVisible() const;
protected:	
	/** Objects displayed in the details view */
	TArray<UObject*> SettingsObjects;
	/** Details view for brush and paint settings */
	TSharedPtr<IDetailsView> SettingsDetailsView;
	/** Ptr to painter for which this widget is the ui representation */
	FPaintModePainter* MeshPainter;
	/** Paint settings instance */
	UPaintModeSettings* PaintModeSettings;

	// FB Bulgakov Begin - Texture2D Array
public:
	FReply RefreshTextureArrayPalette(); 

protected:
	TSharedRef<SWidget> CreateTextureArrayPaletteViews();

	TArray<FTextureArrayPaletteItemModelPtr> TextureArrayPaletteItems;

	TSharedPtr<STextureArrayTypeTileView> TileViewWidget;
	TSharedPtr<class SScrollBorder> TileViewScrollWidget;

	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;

	TSharedRef<ITableRow> GenerateTile(FTextureArrayPaletteItemModelPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(FTextureArrayPaletteItemModelPtr Item, ESelectInfo::Type SelectInfo);
	// FB Bulgakov End
};
