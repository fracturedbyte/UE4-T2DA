// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SPaintModeWidget.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScrollBorder.h" // FB Bulgakov - Texture2D Array
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PaintModeSettingsCustomization.h"
#include "PaintModePainter.h"
#include "PaintModeSettings.h"

#include "Modules/ModuleManager.h"
#include "PaintModeCommands.h"

#define LOCTEXT_NAMESPACE "PaintModePainter"

void SPaintModeWidget::Construct(const FArguments& InArgs, FPaintModePainter* InPainter)
{
	MeshPainter = InPainter;
	PaintModeSettings = Cast<UPaintModeSettings>(MeshPainter->GetPainterSettings());
	SettingsObjects.Add(MeshPainter->GetBrushSettings());
	SettingsObjects.Add(PaintModeSettings);
	CreateDetailsView();

	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(64)); // FB Bulgakov - Texture2D Array
	
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.0f)
		[
			SNew(SVerticalBox)
			/** Toolbar containing buttons to switch between different paint modes */
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.HAlign(HAlign_Center)
				[
					CreateToolBarWidget()->AsShared()
				]
			]
				
			/** (Instance) Vertex paint action buttons widget */
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				CreateVertexPaintWidget()->AsShared()
			]
				
			/** Texture paint action buttons widget */
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				CreateTexturePaintWidget()->AsShared()
			]

			/** DetailsView containing brush and paint settings */
			+ SVerticalBox::Slot()
			.AutoHeight()				
			[
				SettingsDetailsView->AsShared()
			]
		]
	];
}

void SPaintModeWidget::CreateDetailsView()
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs(
		/*bUpdateFromSelection=*/ false,
		/*bLockable=*/ false,
		/*bAllowSearch=*/ false,
		FDetailsViewArgs::HideNameArea,
		/*bHideSelectionTip=*/ true,
		/*InNotifyHook=*/ nullptr,
		/*InSearchInitialKeyFocus=*/ false,
		/*InViewIdentifier=*/ NAME_None);
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;

	SettingsDetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	SettingsDetailsView->SetRootObjectCustomizationInstance(MakeShareable(new FPaintModeSettingsRootObjectCustomization));
	SettingsDetailsView->SetObjects(SettingsObjects);
}

TSharedPtr<SWidget> SPaintModeWidget::CreateVertexPaintWidget()
{
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);

	TSharedPtr<SWidget> VertexColorWidget;
	TSharedPtr<SHorizontalBox> VertexColorActionBox;
	TSharedPtr<SHorizontalBox> InstanceColorActionBox;

	static const FText SkelMeshNotificationText = LOCTEXT("SkelMeshAssetPaintInfo", "Paint is directly propagated to Skeletal Mesh Asset(s)");
	static const FText StaticMeshNotificationText = LOCTEXT("StaticMeshAssetPaintInfo", "Paint is directly applied to all LODs");	
		
	SAssignNew(VertexColorWidget, SVerticalBox)
	.Visibility(this, &SPaintModeWidget::IsVertexPaintModeVisible)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	.HAlign(HAlign_Center)
	[	
		SAssignNew(VertexColorActionBox, SHorizontalBox)
	]
	
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)	
	.HAlign(HAlign_Center)
	[
		SAssignNew(InstanceColorActionBox, SHorizontalBox)
	]

	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("SettingsEditor.CheckoutWarningBorder"))
		.BorderBackgroundColor(FColor(166,137,0))				
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]() -> EVisibility 
			{
				bool bVisible = MeshPainter && MeshPainter->GetSelectedComponents<USkeletalMeshComponent>().Num();
				bVisible |= ((PaintModeSettings->PaintMode == EPaintMode::Vertices) && !PaintModeSettings->VertexPaintSettings.bPaintOnSpecificLOD);
				return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
			})
		
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(6.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("ClassIcon.SkeletalMeshComponent"))
			]
		
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(.8f)
			.Padding(StandardPadding)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text_Lambda([this]() -> FText
				{
					const bool bSkelMeshText = MeshPainter && MeshPainter->GetSelectedComponents<USkeletalMeshComponent>().Num();
					const bool bLODPaintText = (PaintModeSettings->PaintMode == EPaintMode::Vertices) && !PaintModeSettings->VertexPaintSettings.bPaintOnSpecificLOD;
					return FText::Format(FTextFormat::FromString(TEXT("{0}{1}{2}")), bSkelMeshText ? SkelMeshNotificationText : FText::GetEmpty(), bSkelMeshText && bLODPaintText ? FText::FromString(TEXT("\n")) : FText::GetEmpty(), bLODPaintText ? StaticMeshNotificationText : FText::GetEmpty());
				})
			]
		]
	]
	
	// FB Bulgakov Begin - Texture2D Array
	// Visualize texture array
	+ SVerticalBox::Slot()
	[
		SNew(SBorder)
		.Padding(StandardPadding)
		.Visibility_Lambda([this]() -> EVisibility
		{
			bool bVisible = PaintModeSettings->PaintMode == EPaintMode::Vertices &&
				PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintNumbers;
			return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			CreateTextureArrayPaletteViews()
		]
	];
	// FB Bulgakov End
	
	FToolBarBuilder ColorToolbarBuilder(MeshPainter->GetUICommandList(), FMultiBoxCustomization::None);
	ColorToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	ColorToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Fill, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Fill"));
	ColorToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Propagate, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Propagate"));
	ColorToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Import, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Import"));
	ColorToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Save, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Save"));

	VertexColorActionBox->AddSlot()
	.FillWidth(1.0f)
	[
		ColorToolbarBuilder.MakeWidget()
	];

	FToolBarBuilder InstanceToolbarBuilder(MeshPainter->GetUICommandList(), FMultiBoxCustomization::None);
	InstanceToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	InstanceToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Copy, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Copy"));
	InstanceToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Paste, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Paste"));
	InstanceToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Remove, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Remove"));
	InstanceToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().Fix, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Fix"));
	InstanceToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().PropagateVertexColorsToLODs, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Propagate"));

	InstanceColorActionBox->AddSlot()
	.FillWidth(1.0f)
	[
		InstanceToolbarBuilder.MakeWidget()
	];

	return VertexColorWidget->AsShared();
}
 
TSharedPtr<SWidget> SPaintModeWidget::CreateTexturePaintWidget()
{
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);
	TSharedPtr<SWidget> TexturePaintWidget;
	TSharedPtr<SHorizontalBox> ActionBox;

	SAssignNew(TexturePaintWidget, SVerticalBox)
	.Visibility(this, &SPaintModeWidget::IsTexturePaintModeVisible)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	.HAlign(HAlign_Center)
	[
		SAssignNew(ActionBox, SHorizontalBox)
	];
	 
	FToolBarBuilder TexturePaintToolbarBuilder(MeshPainter->GetUICommandList(), FMultiBoxCustomization::None);
	TexturePaintToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	TexturePaintToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().PropagateTexturePaint, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Propagate"));
	TexturePaintToolbarBuilder.AddToolBarButton(FPaintModeCommands::Get().SaveTexturePaint, NAME_None, FText::GetEmpty(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MeshPaint.Save"));

	ActionBox->AddSlot()
	.FillWidth(1.0f)
	[
		TexturePaintToolbarBuilder.MakeWidget()
	];

	return TexturePaintWidget->AsShared();
}

TSharedPtr<SWidget> SPaintModeWidget::CreateToolBarWidget()
{
	FToolBarBuilder ModeSwitchButtons(MakeShareable(new FUICommandList()), FMultiBoxCustomization::None);
	{
		FSlateIcon ColorPaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.ColorPaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Vertices;
			PaintModeSettings->VertexPaintSettings.MeshPaintMode = EMeshPaintMode::PaintColors;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Vertices && PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintColors; })), NAME_None, LOCTEXT("Mode.VertexColorPainting", "Colors"), LOCTEXT("Mode.VertexColor.Tooltip", "Vertex Color Painting mode allows painting of Vertex Colors"), ColorPaintIcon, EUserInterfaceActionType::ToggleButton);

		FSlateIcon WeightPaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.WeightPaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Vertices;
			PaintModeSettings->VertexPaintSettings.MeshPaintMode = EMeshPaintMode::PaintWeights;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Vertices && PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintWeights; })), NAME_None, LOCTEXT("Mode.VertexWeightPainting", " Weights"), LOCTEXT("Mode.VertexWeight.Tooltip", "Vertex Weight Painting mode allows painting of Vertex Weights"), WeightPaintIcon, EUserInterfaceActionType::ToggleButton);

		FSlateIcon TexturePaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.TexturePaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Textures;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Textures; })), NAME_None, LOCTEXT("Mode.TexturePainting", "Textures"), LOCTEXT("Mode.Texture.Tooltip", "Texture Weight Painting mode allows painting on Textures"), TexturePaintIcon, EUserInterfaceActionType::ToggleButton);

		// FB Bulgakov Begin - Texture2D Array
		FSlateIcon AdvancedMeshPaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.AdvancedMeshPaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Vertices;
			PaintModeSettings->VertexPaintSettings.MeshPaintMode = EMeshPaintMode::PaintNumbers;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Vertices && PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintNumbers; })), NAME_None, LOCTEXT("Mode.VertexNumberPainting", " Numbers"), LOCTEXT("Mode.VertexWeight.Tooltip", "Vertex Weight Painting mode allows painting of Vertex Weights"), AdvancedMeshPaintIcon, EUserInterfaceActionType::ToggleButton);
		// FB Bulgakov End
	}

	return ModeSwitchButtons.MakeWidget();
}

EVisibility SPaintModeWidget::IsVertexPaintModeVisible() const
{
	UPaintModeSettings* MeshPaintSettings = (UPaintModeSettings*)MeshPainter->GetPainterSettings();
	return (MeshPaintSettings->PaintMode == EPaintMode::Vertices) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SPaintModeWidget::IsTexturePaintModeVisible() const
{
	UPaintModeSettings* MeshPaintSettings = (UPaintModeSettings*)MeshPainter->GetPainterSettings();
	return (MeshPaintSettings->PaintMode == EPaintMode::Textures) ? EVisibility::Visible : EVisibility::Collapsed;
}

// FB Bulgakov Begin - Texture2D Array
TSharedRef<SWidget> SPaintModeWidget::CreateTextureArrayPaletteViews()
{
	SAssignNew(TileViewWidget, STextureArrayTypeTileView)
		.ListItemsSource(&TextureArrayPaletteItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateTile(this, &SPaintModeWidget::GenerateTile)
		.OnSelectionChanged(this, &SPaintModeWidget::OnSelectionChanged)
		.ItemHeight(64)
		.ItemWidth(64)
		.ItemAlignment(EListItemAlignment::LeftAligned)
		.ClearSelectionOnClick(true)
		;

	SAssignNew(TileViewScrollWidget, SScrollBorder, TileViewWidget.ToSharedRef())
	.Content()
	[
		TileViewWidget.ToSharedRef()
	];

	return TileViewScrollWidget.ToSharedRef();
}

TSharedRef<ITableRow> SPaintModeWidget::GenerateTile(FTextureArrayPaletteItemModelPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureArrayPaletteItemTile, OwnerTable, Item);
}

FReply SPaintModeWidget::RefreshTextureArrayPalette()
{
	TextureArrayPaletteItems.Empty();

	UTexture2DArray* Texture2DArray = nullptr;

	UPaintModeSettings* MeshPaintSettings = (UPaintModeSettings*)MeshPainter->GetPainterSettings();
	if (MeshPaintSettings && MeshPaintSettings->VertexPaintSettings.Texture2DArray)
	{
		Texture2DArray = MeshPaintSettings->VertexPaintSettings.Texture2DArray;
	}
	else
	{
		TArray<UPrimitiveComponent*> Components = MeshPainter->GetSelectedComponents<UPrimitiveComponent>();
		for (UPrimitiveComponent* Component : Components)
		{
			if (!Component)
				continue;

			TArray<UMaterialInterface*> OutMaterials;
			Component->GetUsedMaterials(OutMaterials);
			for (UMaterialInterface* MI : OutMaterials)
			{
				if (!MI)
					continue;
				
				TArray<UTexture*> OutTextures;
				MI->GetUsedTextures(OutTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);

				for (UTexture* Texture : OutTextures)
				{
					UTexture2DArray* t2da = Cast<UTexture2DArray>(Texture);
					if (t2da)
					{
						Texture2DArray = t2da;
						break;
					}
				}

				if (Texture2DArray)
					break;
			}

			if (Texture2DArray)
				break;
		}
	}

	if (Texture2DArray)
	{
		int32 TextureNum = Texture2DArray->Source2DTextures.Num();
		for (int32 TextureId = 0; TextureId < TextureNum; TextureId++)
		{
			UTexture2D* Texture = Texture2DArray->Source2DTextures[TextureId];

			TextureArrayPaletteItems.Add(MakeShareable(new FTextureArrayPaletteItemModel(Texture, TextureId, SharedThis(this), ThumbnailPool, MeshPainter)));
		}
	}

	if (TileViewWidget)
		TileViewWidget->RequestListRefresh();

	return FReply::Handled();
}

void SPaintModeWidget::OnSelectionChanged(FTextureArrayPaletteItemModelPtr Item, ESelectInfo::Type SelectInfo)
{
	// Update number to paint
	auto PaintModeSettings = Cast<UPaintModeSettings>(MeshPainter->GetPainterSettings());
	if (Item)
		PaintModeSettings->VertexPaintSettings.NumberToPaint = Item->GetTextureId();
}


FTextureArrayPaletteItemModel::FTextureArrayPaletteItemModel(UTexture2D* InTexture2D, int32 InTextureId, TSharedRef<SPaintModeWidget> InPaintModeWidget, TSharedPtr<class FAssetThumbnailPool> InThumbnailPool, FPaintModePainter* InMeshPainter)
	: Texture2D(InTexture2D)
	, TextureId(InTextureId)
	, MeshPainter(InMeshPainter)
{
	// Determine the display FName
	DisplayFName = Texture2D->GetFName();

	FAssetData AssetData = FAssetData(Texture2D);

	int32 MaxThumbnailSize = 64;
	TSharedPtr< FAssetThumbnail > Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, MaxThumbnailSize, MaxThumbnailSize, InThumbnailPool));

	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailWidget = Thumbnail->MakeThumbnailWidget(ThumbnailConfig);
}

void STextureArrayPaletteItemTile::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FTextureArrayPaletteItemModel>& InModel)
{
	Model = InModel;

	STableRow<FTextureArrayMeshUIInfoPtr>::Construct(
		STableRow<FTextureArrayMeshUIInfoPtr>::FArguments()
		.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
		.Padding(1.f)
		.Content()
		[
			SNew(SOverlay)
		
			// Thumbnail
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(4.f)
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
				.ForegroundColor(FLinearColor::White)
				[
					Model->GetThumbnailWidget()
				]
			]

			// Texture Id
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(6.f, 8.f))
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(Model->GetTextureId()))
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
			]
		], InOwnerTableView);
}
// FB Bulgakov End

#undef LOCTEXT_NAMESPACE // "PaintModePainter"
