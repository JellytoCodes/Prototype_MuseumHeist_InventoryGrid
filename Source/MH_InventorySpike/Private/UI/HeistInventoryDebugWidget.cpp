#include "UI/HeistInventoryDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Inventory/HeistInventoryComponent.h"

namespace
{
	void SetTextSize(UTextBlock* TextBlock, const int32 Size)
	{
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
	}

	FString GetShortItemName(const FName ItemId)
	{
		FString Result = ItemId.ToString();
		int32 SeparatorIndex = INDEX_NONE;
		if (Result.FindLastChar(TEXT('_'), SeparatorIndex))
		{
			Result.RightChopInline(SeparatorIndex + 1);
		}
		return Result;
	}

	FLinearColor GetItemColor(const int32 InstanceId)
	{
		static const FLinearColor Colors[] =
		{
			FLinearColor(0.16f, 0.42f, 0.58f, 1.0f),
			FLinearColor(0.35f, 0.25f, 0.55f, 1.0f),
			FLinearColor(0.18f, 0.48f, 0.32f, 1.0f),
			FLinearColor(0.58f, 0.28f, 0.18f, 1.0f),
			FLinearColor(0.48f, 0.38f, 0.16f, 1.0f)
		};
		return Colors[FMath::Abs(InstanceId) % UE_ARRAY_COUNT(Colors)];
	}
}

void UHeistInventoryDebugWidget::SetInventoryComponent(UHeistInventoryComponent* InInventoryComponent)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshInventory);
	}

	InventoryComponent = InInventoryComponent;
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshInventory);
	}
	RefreshInventory();
}

void UHeistInventoryDebugWidget::SetSelectedInstanceId(const int32 InSelectedInstanceId)
{
	SelectedInstanceId = InSelectedInstanceId;
	RefreshInventory();
}

void UHeistInventoryDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildDebugLayout();
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshInventory);
	}
	RefreshInventory();
}

void UHeistInventoryDebugWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshInventory);
	}
	Super::NativeDestruct();
}

void UHeistInventoryDebugWidget::BuildDebugLayout()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(760.0f);
	PanelSize->SetHeightOverride(650.0f);
	UCanvasPanelSlot* PanelCanvasSlot = RootCanvas->AddChildToCanvas(PanelSize);
	PanelCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelCanvasSlot->SetPosition(FVector2D::ZeroVector);
	PanelCanvasSlot->SetAutoSize(true);

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryBackground"));
	Background->SetBrushColor(FLinearColor(0.018f, 0.025f, 0.04f, 0.97f));
	Background->SetPadding(FMargin(26.0f));
	PanelSize->SetContent(Background);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryContent"));
	Background->SetContent(Content);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryTitle"));
	Title->SetText(FText::FromString(TEXT("MUSEUM HEIST  /  FIELD BACKPACK")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.78f, 0.28f)));
	Title->SetJustification(ETextJustify::Center);
	SetTextSize(Title, 28);
	Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventorySubtitle"));
	Subtitle->SetText(FText::FromString(TEXT("4 x 5 GRID  |  SERVER CONFIRMED STATE")));
	Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.46f, 0.58f, 0.7f)));
	Subtitle->SetJustification(ETextJustify::Center);
	SetTextSize(Subtitle, 13);
	Content->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainRow"));
	UVerticalBoxSlot* MainRowSlot = Content->AddChildToVerticalBox(MainRow);
	MainRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* GridColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GridColumn"));
	UHorizontalBoxSlot* GridColumnSlot = MainRow->AddChildToHorizontalBox(GridColumn);
	GridColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	GridColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UTextBlock* GridHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GridHeader"));
	GridHeader->SetText(FText::FromString(TEXT("BACKPACK STORAGE")));
	GridHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.82f, 0.9f)));
	SetTextSize(GridHeader, 16);
	GridColumn->AddChildToVerticalBox(GridHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	GridPanel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("InventoryGrid"));
	GridPanel->SetSlotPadding(FMargin(3.0f));
	GridPanel->SetMinDesiredSlotWidth(72.0f);
	GridPanel->SetMinDesiredSlotHeight(72.0f);
	GridColumn->AddChildToVerticalBox(GridPanel)->SetHorizontalAlignment(HAlign_Center);

	GridCells.Reset();
	GridCellBorders.Reset();
	constexpr int32 CellCount = UHeistInventoryComponent::DefaultGridWidth * UHeistInventoryComponent::DefaultGridHeight;
	GridCells.Reserve(CellCount);
	GridCellBorders.Reserve(CellCount);
	for (int32 Index = 0; Index < CellCount; ++Index)
	{
		UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>();
		CellBorder->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.105f, 1.0f));
		CellBorder->SetPadding(FMargin(4.0f));
		GridCellBorders.Add(CellBorder);

		UTextBlock* CellText = WidgetTree->ConstructWidget<UTextBlock>();
		CellText->SetText(FText::FromString(TEXT("-")));
		CellText->SetJustification(ETextJustify::Center);
		CellText->SetColorAndOpacity(FSlateColor(FLinearColor(0.32f, 0.4f, 0.48f)));
		SetTextSize(CellText, 15);
		CellBorder->SetContent(CellText);
		GridCells.Add(CellText);

		const int32 X = Index % UHeistInventoryComponent::DefaultGridWidth;
		const int32 Y = Index / UHeistInventoryComponent::DefaultGridWidth;
		UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(CellBorder, Y, X);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}

	StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryStats"));
	StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.8f, 0.88f)));
	StatsText->SetJustification(ETextJustify::Center);
	SetTextSize(StatsText, 15);
	GridColumn->AddChildToVerticalBox(StatsText)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

	UBorder* DetailBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBackground"));
	DetailBackground->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.075f, 1.0f));
	DetailBackground->SetPadding(FMargin(18.0f));
	UHorizontalBoxSlot* DetailSlot = MainRow->AddChildToHorizontalBox(DetailBackground);
	DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailColumn"));
	DetailBackground->SetContent(DetailColumn);

	UTextBlock* SelectedHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedHeader"));
	SelectedHeader->SetText(FText::FromString(TEXT("SELECTED ITEM")));
	SelectedHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.78f, 0.28f)));
	SetTextSize(SelectedHeader, 16);
	DetailColumn->AddChildToVerticalBox(SelectedHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	SelectedItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemText"));
	SelectedItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.93f, 0.96f)));
	SelectedItemText->SetAutoWrapText(true);
	SetTextSize(SelectedItemText, 16);
	DetailColumn->AddChildToVerticalBox(SelectedItemText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UTextBlock* QuickSlotHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuickSlotHeader"));
	QuickSlotHeader->SetText(FText::FromString(TEXT("QUICK SLOT REFERENCES")));
	QuickSlotHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.72f, 0.88f)));
	SetTextSize(QuickSlotHeader, 15);
	DetailColumn->AddChildToVerticalBox(QuickSlotHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	QuickSlotTexts.Reset();
	static const TCHAR* QuickSlotNames[] = { TEXT("Q"), TEXT("E"), TEXT("R") };
	for (int32 Index = 0; Index < UHeistInventoryComponent::QuickSlotCount; ++Index)
	{
		UBorder* QuickSlotBorder = WidgetTree->ConstructWidget<UBorder>();
		QuickSlotBorder->SetBrushColor(FLinearColor(0.07f, 0.095f, 0.13f, 1.0f));
		QuickSlotBorder->SetPadding(FMargin(10.0f, 8.0f));
		DetailColumn->AddChildToVerticalBox(QuickSlotBorder)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

		UTextBlock* QuickSlotText = WidgetTree->ConstructWidget<UTextBlock>();
		QuickSlotText->SetText(FText::FromString(FString::Printf(TEXT("%s  /  Empty"), QuickSlotNames[Index])));
		QuickSlotText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.9f)));
		SetTextSize(QuickSlotText, 14);
		QuickSlotBorder->SetContent(QuickSlotText);
		QuickSlotTexts.Add(QuickSlotText);
	}

	UTextBlock* Controls = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryControls"));
	Controls->SetText(FText::FromString(
		TEXT("TEST CONTROLS\n[ / ]  Select item\nArrow Keys  Move\nT  Rotate    Delete  Drop\n1 / 2 / 3  Assign Q / E / R\nI or Tab  Close")));
	Controls->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.62f, 0.72f)));
	Controls->SetAutoWrapText(true);
	SetTextSize(Controls, 13);
	DetailColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
}

void UHeistInventoryDebugWidget::RefreshInventory()
{
	if (!InventoryComponent || GridCells.IsEmpty() || !StatsText || !SelectedItemText)
	{
		return;
	}

	TArray<int32> Occupancy;
	Occupancy.Init(INDEX_NONE, InventoryComponent->GetGridWidth() * InventoryComponent->GetGridHeight());
	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	for (const FHeistInventoryItem& Item : Items)
	{
		if (!InventoryComponent->IsValidSlotIndex(Item.TopLeftIndex))
		{
			continue;
		}

		const FIntPoint TopLeft = InventoryComponent->IndexToCoord(Item.TopLeftIndex);
		const int32 Width = InventoryComponent->GetEffectiveWidth(Item, Item.bRotated);
		const int32 Height = InventoryComponent->GetEffectiveHeight(Item, Item.bRotated);
		for (int32 Y = TopLeft.Y; Y < TopLeft.Y + Height; ++Y)
		{
			for (int32 X = TopLeft.X; X < TopLeft.X + Width; ++X)
			{
				const int32 SlotIndex = InventoryComponent->CoordToIndex(X, Y);
				if (Occupancy.IsValidIndex(SlotIndex))
				{
					Occupancy[SlotIndex] = Item.InstanceId;
				}
			}
		}
	}

	for (int32 Index = 0; Index < GridCells.Num(); ++Index)
	{
		const int32 InstanceId = Occupancy.IsValidIndex(Index) ? Occupancy[Index] : INDEX_NONE;
		if (InstanceId == INDEX_NONE)
		{
			GridCells[Index]->SetText(FText::FromString(TEXT("-")));
			GridCells[Index]->SetColorAndOpacity(FSlateColor(FLinearColor(0.32f, 0.4f, 0.48f)));
			GridCellBorders[Index]->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.105f, 1.0f));
			continue;
		}

		const bool bSelected = InstanceId == SelectedInstanceId;
		GridCells[Index]->SetText(FText::FromString(FString::Printf(TEXT("#%d"), InstanceId)));
		GridCells[Index]->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor(0.08f, 0.06f, 0.02f) : FLinearColor::White));
		GridCellBorders[Index]->SetBrushColor(bSelected
			? FLinearColor(0.95f, 0.72f, 0.2f, 1.0f)
			: GetItemColor(InstanceId));
	}

	StatsText->SetText(FText::FromString(FString::Printf(
		TEXT("ITEMS %d    USED %d / %d\nWEIGHT %d    SCORE %d"),
		Items.Num(),
		InventoryComponent->GetUsedSlotCount(),
		InventoryComponent->GetGridWidth() * InventoryComponent->GetGridHeight(),
		InventoryComponent->GetTotalWeight(),
		InventoryComponent->GetTotalScore())));

	const FHeistInventoryItem* SelectedItem = Items.FindByPredicate([this](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == SelectedInstanceId;
	});
	if (SelectedItem)
	{
		SelectedItemText->SetText(FText::FromString(FString::Printf(
			TEXT("%s\nInstance #%d\nSize  %d x %d\nTop Left  %d\nRotated  %s\nWeight  %d\nScore  %d"),
			*SelectedItem->ItemId.ToString(),
			SelectedItem->InstanceId,
			InventoryComponent->GetEffectiveWidth(*SelectedItem, SelectedItem->bRotated),
			InventoryComponent->GetEffectiveHeight(*SelectedItem, SelectedItem->bRotated),
			SelectedItem->TopLeftIndex,
			SelectedItem->bRotated ? TEXT("YES") : TEXT("NO"),
			SelectedItem->Weight,
			SelectedItem->ScoreValue)));
	}
	else
	{
		SelectedItemText->SetText(FText::FromString(TEXT("No item selected")));
	}

	const TArray<int32> QuickSlots = InventoryComponent->GetQuickSlots();
	static const TCHAR* QuickSlotNames[] = { TEXT("Q"), TEXT("E"), TEXT("R") };
	for (int32 Index = 0; Index < QuickSlotTexts.Num(); ++Index)
	{
		const int32 QuickSlotInstanceId = QuickSlots.IsValidIndex(Index) ? QuickSlots[Index] : INDEX_NONE;
		const FHeistInventoryItem* QuickSlotItem = Items.FindByPredicate([QuickSlotInstanceId](const FHeistInventoryItem& Item)
		{
			return Item.InstanceId == QuickSlotInstanceId;
		});

		const FString QuickSlotValue = QuickSlotItem
			? FString::Printf(TEXT("#%d  %s"), QuickSlotItem->InstanceId, *GetShortItemName(QuickSlotItem->ItemId))
			: FString(TEXT("Empty"));
		QuickSlotTexts[Index]->SetText(FText::FromString(FString::Printf(
			TEXT("%s  /  %s"),
			QuickSlotNames[Index],
			*QuickSlotValue)));
	}
}
