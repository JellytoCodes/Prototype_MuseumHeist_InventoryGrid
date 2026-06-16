#include "UI/HeistInventoryDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Inventory/HeistInventoryComponent.h"
#include "UI/HeistInventoryDragDropOperation.h"
#include "UI/HeistInventoryItemWidget.h"
#include "UI/HeistInventorySlotWidget.h"
#include "UI/HeistQuickSlotPrototypeWidget.h"

namespace
{
	void SetPrototypeTextSize(UTextBlock* TextBlock, const int32 Size)
	{
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
	}

	FString GetInventoryRequestLabel(const EHeistInventoryRequestType RequestType)
	{
		switch (RequestType)
		{
		case EHeistInventoryRequestType::MoveItem: return TEXT("Move");
		case EHeistInventoryRequestType::DropItem: return TEXT("Drop");
		case EHeistInventoryRequestType::AssignQuickSlot: return TEXT("QuickSlot");
		default: return TEXT("Unknown");
		}
	}

	FString GetInventoryResultLabel(const EHeistInventoryRequestResult Result)
	{
		switch (Result)
		{
		case EHeistInventoryRequestResult::Success: return TEXT("confirmed by server");
		case EHeistInventoryRequestResult::AuthorityDenied: return TEXT("authority denied");
		case EHeistInventoryRequestResult::ItemNotFound: return TEXT("item not found");
		case EHeistInventoryRequestResult::InvalidPlacement: return TEXT("invalid placement");
		case EHeistInventoryRequestResult::InvalidQuickSlotIndex: return TEXT("invalid QuickSlot index");
		case EHeistInventoryRequestResult::InvalidQuickSlotItem: return TEXT("invalid QuickSlot item");
		case EHeistInventoryRequestResult::MutationRejected: return TEXT("mutation rejected");
		default: return TEXT("unknown result");
		}
	}
}

void UHeistInventoryDebugWidget::SetInventoryComponent(UHeistInventoryComponent* InInventoryComponent)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshInventory);
		InventoryComponent->OnInventoryRequestResolved.RemoveDynamic(this, &ThisClass::HandleInventoryRequestResolved);
	}

	InventoryComponent = InInventoryComponent;
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshInventory);
		InventoryComponent->OnInventoryRequestResolved.AddUniqueDynamic(this, &ThisClass::HandleInventoryRequestResolved);
	}

	for (int32 QuickSlotIndex = 0; QuickSlotIndex < QuickSlotWidgets.Num(); ++QuickSlotIndex)
	{
		if (QuickSlotWidgets[QuickSlotIndex])
		{
			QuickSlotWidgets[QuickSlotIndex]->InitializeQuickSlot(InventoryComponent, QuickSlotIndex);
		}
	}
	RefreshInventory();
}

void UHeistInventoryDebugWidget::SetSelectedInstanceId(const int32 InSelectedInstanceId)
{
	SelectedInstanceId = InSelectedInstanceId;
	if (InventoryComponent)
	{
		RefreshSelectionDetails(InventoryComponent->GetItems());
	}
}

void UHeistInventoryDebugWidget::SelectItem(const int32 InstanceId)
{
	SelectedInstanceId = InstanceId;
	if (InventoryComponent)
	{
		RefreshSelectionDetails(InventoryComponent->GetItems());
	}
}

void UHeistInventoryDebugWidget::RequestRotateItem(const int32 InstanceId)
{
	if (!InventoryComponent)
	{
		return;
	}

	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	const FHeistInventoryItem* Item = Items.FindByPredicate([InstanceId](const FHeistInventoryItem& Candidate)
	{
		return Candidate.InstanceId == InstanceId;
	});
	if (Item)
	{
		InventoryComponent->Server_RequestMoveItem(Item->InstanceId, Item->TopLeftIndex, !Item->bRotated);
	}
}

void UHeistInventoryDebugWidget::NotifyDragStarted(UHeistInventoryDragDropOperation* DragOperation)
{
	ActiveDragOperation = DragOperation;
	UpdateDragVisualAnchor(DragOperation);
	SelectItem(DragOperation ? DragOperation->InstanceId : INDEX_NONE);
	if (DragStatusText)
	{
		DragStatusText->SetText(FText::FromString(TEXT("Dragging: move over grid for placement preview")));
		DragStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.78f, 0.9f)));
	}
}

void UHeistInventoryDebugWidget::NotifyDragFinished()
{
	ActiveDragOperation = nullptr;
	ClearPlacementPreview();
}

void UHeistInventoryDebugWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	BuildPrototypeLayout();
}

void UHeistInventoryDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshInventory);
		InventoryComponent->OnInventoryRequestResolved.AddUniqueDynamic(this, &ThisClass::HandleInventoryRequestResolved);
	}
	RefreshInventory();
}

void UHeistInventoryDebugWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshInventory);
		InventoryComponent->OnInventoryRequestResolved.RemoveDynamic(this, &ThisClass::HandleInventoryRequestResolved);
	}
	Super::NativeDestruct();
}

void UHeistInventoryDebugWidget::HandleInventoryRequestResolved(
	const EHeistInventoryRequestType RequestType,
	const EHeistInventoryRequestResult Result)
{
	if (Result != EHeistInventoryRequestResult::Success)
	{
		RefreshInventory();
	}

	if (DragStatusText)
	{
		const bool bSuccess = Result == EHeistInventoryRequestResult::Success;
		DragStatusText->SetText(FText::FromString(FString::Printf(
			TEXT("%s: %s%s"),
			*GetInventoryRequestLabel(RequestType),
			*GetInventoryResultLabel(Result),
			bSuccess ? TEXT("") : TEXT("; restored confirmed state"))));
		DragStatusText->SetColorAndOpacity(FSlateColor(bSuccess
			? FLinearColor(0.25f, 0.9f, 0.42f)
			: FLinearColor(0.95f, 0.35f, 0.25f)));
	}
}

FReply UHeistInventoryDebugWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::R && SelectedInstanceId != INDEX_NONE)
	{
		if (ActiveDragOperation)
		{
			const bool bWasRotated = ActiveDragOperation->bRotated;
			const int32 OldWidth = InventoryComponent->GetEffectiveWidth(
				ActiveDragOperation->ItemSnapshot,
				bWasRotated);
			const int32 OldHeight = InventoryComponent->GetEffectiveHeight(
				ActiveDragOperation->ItemSnapshot,
				bWasRotated);
			const FIntPoint OldGrabCell = ActiveDragOperation->GrabCellOffset;
			const FVector2D OldGrabWithinCell = ActiveDragOperation->GrabOffsetWithinCell;

			ActiveDragOperation->bRotated = !bWasRotated;
			if (!bWasRotated)
			{
				ActiveDragOperation->GrabCellOffset = FIntPoint(
					OldHeight - 1 - OldGrabCell.Y,
					OldGrabCell.X);
				ActiveDragOperation->GrabOffsetWithinCell = FVector2D(
					CellSize - OldGrabWithinCell.Y,
					OldGrabWithinCell.X);
			}
			else
			{
				ActiveDragOperation->GrabCellOffset = FIntPoint(
					OldGrabCell.Y,
					OldWidth - 1 - OldGrabCell.X);
				ActiveDragOperation->GrabOffsetWithinCell = FVector2D(
					OldGrabWithinCell.Y,
					CellSize - OldGrabWithinCell.X);
			}
			if (UHeistInventoryItemWidget* DragVisual = Cast<UHeistInventoryItemWidget>(ActiveDragOperation->DefaultDragVisual))
			{
				DragVisual->SetPreviewRotation(ActiveDragOperation->bRotated);
			}
			UpdateDragVisualAnchor(ActiveDragOperation);
			ClearPlacementPreview();
			if (DragStatusText)
			{
				DragStatusText->SetText(FText::FromString(TEXT("Rotation preview changed; move over grid to validate")));
			}
			return FReply::Handled();
		}

		RequestRotateItem(SelectedInstanceId);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UHeistInventoryDebugWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UHeistInventoryDragDropOperation* DragOperation = Cast<UHeistInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	UpdatePlacementPreview(DragOperation, InDragDropEvent.GetScreenSpacePosition());
	return true;
}

void UHeistInventoryDebugWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ClearPlacementPreview();
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
}

bool UHeistInventoryDebugWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UHeistInventoryDragDropOperation* DragOperation = Cast<UHeistInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !InventoryComponent)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	bool bInsideGrid = false;
	int32 HoveredSlot = INDEX_NONE;
	int32 TopLeftIndex = INDEX_NONE;
	GetDropCoordinates(InDragDropEvent.GetScreenSpacePosition(), DragOperation, bInsideGrid, HoveredSlot, TopLeftIndex);

	if (bInsideGrid)
	{
		const bool bValid = TopLeftIndex != INDEX_NONE && InventoryComponent->CanPlaceItem(
			DragOperation->ItemSnapshot,
			TopLeftIndex,
			DragOperation->bRotated,
			DragOperation->InstanceId);
		if (bValid)
		{
			InventoryComponent->Server_RequestMoveItem(DragOperation->InstanceId, TopLeftIndex, DragOperation->bRotated);
		}
	}
	else
	{
		InventoryComponent->Server_RequestDropItem(DragOperation->InstanceId);
	}

	ClearPlacementPreview();
	ActiveDragOperation = nullptr;
	return true;
}

void UHeistInventoryDebugWidget::BuildPrototypeLayout()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(790.0f);
	PanelSize->SetHeightOverride(670.0f);
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
	Title->SetText(FText::FromString(TEXT("INV-SPIKE-002  /  DRAG & DROP BACKPACK")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.78f, 0.28f)));
	Title->SetJustification(ETextJustify::Center);
	SetPrototypeTextSize(Title, 25);
	Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventorySubtitle"));
	Subtitle->SetText(FText::FromString(TEXT("Drag items between cells, onto Q/E/R, or outside the grid")));
	Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.46f, 0.58f, 0.7f)));
	Subtitle->SetJustification(ETextJustify::Center);
	SetPrototypeTextSize(Subtitle, 13);
	Content->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainRow"));
	UVerticalBoxSlot* MainRowSlot = Content->AddChildToVerticalBox(MainRow);
	MainRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* GridColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GridColumn"));
	UHorizontalBoxSlot* GridColumnSlot = MainRow->AddChildToHorizontalBox(GridColumn);
	GridColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	GridColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));

	UTextBlock* GridHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GridHeader"));
	GridHeader->SetText(FText::FromString(TEXT("4 x 5 STORAGE")));
	GridHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.82f, 0.9f)));
	SetPrototypeTextSize(GridHeader, 16);
	GridColumn->AddChildToVerticalBox(GridHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	USizeBox* GridSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GridSizeBox"));
	GridSizeBox->SetWidthOverride(CellPitch * UHeistInventoryComponent::DefaultGridWidth - CellGap);
	GridSizeBox->SetHeightOverride(CellPitch * UHeistInventoryComponent::DefaultGridHeight - CellGap);
	GridColumn->AddChildToVerticalBox(GridSizeBox)->SetHorizontalAlignment(HAlign_Center);

	GridCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryGridCanvas"));
	GridSizeBox->SetContent(GridCanvas);

	SlotWidgets.Reset();
	constexpr int32 SlotCount = UHeistInventoryComponent::DefaultGridWidth * UHeistInventoryComponent::DefaultGridHeight;
	SlotWidgets.Reserve(SlotCount);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UHeistInventorySlotWidget* SlotWidget = CreateWidget<UHeistInventorySlotWidget>(GetOwningPlayer(), UHeistInventorySlotWidget::StaticClass());
		SlotWidget->InitializeSlot(SlotIndex);
		SlotWidgets.Add(SlotWidget);

		const int32 X = SlotIndex % UHeistInventoryComponent::DefaultGridWidth;
		const int32 Y = SlotIndex / UHeistInventoryComponent::DefaultGridWidth;
		UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(SlotWidget);
		CanvasSlot->SetPosition(FVector2D(X * CellPitch, Y * CellPitch));
		CanvasSlot->SetSize(FVector2D(CellSize, CellSize));
		CanvasSlot->SetZOrder(0);
	}

	StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryStats"));
	StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.8f, 0.88f)));
	StatsText->SetJustification(ETextJustify::Center);
	SetPrototypeTextSize(StatsText, 14);
	GridColumn->AddChildToVerticalBox(StatsText)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

	DragStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DragStatus"));
	DragStatusText->SetText(FText::FromString(TEXT("Green = valid placement    Red = rejected placement")));
	DragStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.62f, 0.72f)));
	DragStatusText->SetJustification(ETextJustify::Center);
	SetPrototypeTextSize(DragStatusText, 12);
	GridColumn->AddChildToVerticalBox(DragStatusText)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

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
	SetPrototypeTextSize(SelectedHeader, 16);
	DetailColumn->AddChildToVerticalBox(SelectedHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	SelectedItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemText"));
	SelectedItemText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.93f, 0.96f)));
	SelectedItemText->SetAutoWrapText(true);
	SetPrototypeTextSize(SelectedItemText, 15);
	DetailColumn->AddChildToVerticalBox(SelectedItemText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	UTextBlock* QuickSlotHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuickSlotHeader"));
	QuickSlotHeader->SetText(FText::FromString(TEXT("DROP ITEM ON QUICK SLOT")));
	QuickSlotHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.72f, 0.88f)));
	SetPrototypeTextSize(QuickSlotHeader, 14);
	DetailColumn->AddChildToVerticalBox(QuickSlotHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	QuickSlotWidgets.Reset();
	for (int32 QuickSlotIndex = 0; QuickSlotIndex < UHeistInventoryComponent::QuickSlotCount; ++QuickSlotIndex)
	{
		UHeistQuickSlotPrototypeWidget* QuickSlotWidget = CreateWidget<UHeistQuickSlotPrototypeWidget>(GetOwningPlayer(), UHeistQuickSlotPrototypeWidget::StaticClass());
		QuickSlotWidget->InitializeQuickSlot(InventoryComponent, QuickSlotIndex);
		QuickSlotWidgets.Add(QuickSlotWidget);
		DetailColumn->AddChildToVerticalBox(QuickSlotWidget)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
	}

	UTextBlock* Controls = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryControls"));
	Controls->SetText(FText::FromString(
		TEXT("LEFT DRAG  Move item\nR / RIGHT CLICK  Rotate selected\nDROP OUTSIDE GRID  Drop to world\nDROP ON Q/E/R  Assign reference\nI or Tab  Close")));
	Controls->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.62f, 0.72f)));
	Controls->SetAutoWrapText(true);
	SetPrototypeTextSize(Controls, 13);
	DetailColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
}

void UHeistInventoryDebugWidget::RefreshInventory()
{
	if (!InventoryComponent || !GridCanvas || !StatsText || !SelectedItemText)
	{
		return;
	}

	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	if (!Items.ContainsByPredicate([this](const FHeistInventoryItem& Item) { return Item.InstanceId == SelectedInstanceId; }))
	{
		SelectedInstanceId = Items.IsEmpty() ? INDEX_NONE : Items[0].InstanceId;
	}

	ClearPlacementPreview();
	RebuildItemWidgets(Items);
	RefreshSelectionDetails(Items);
	RefreshQuickSlots(Items);

	StatsText->SetText(FText::FromString(FString::Printf(
		TEXT("ITEMS %d    USED %d / %d    WEIGHT %d    SCORE %d"),
		Items.Num(),
		InventoryComponent->GetUsedSlotCount(),
		InventoryComponent->GetGridWidth() * InventoryComponent->GetGridHeight(),
		InventoryComponent->GetTotalWeight(),
		InventoryComponent->GetTotalScore())));
}

void UHeistInventoryDebugWidget::RebuildItemWidgets(const TArray<FHeistInventoryItem>& Items)
{
	for (UHeistInventoryItemWidget* ItemWidget : ItemWidgets)
	{
		if (ItemWidget)
		{
			ItemWidget->RemoveFromParent();
		}
	}
	ItemWidgets.Reset();

	for (const FHeistInventoryItem& Item : Items)
	{
		if (!InventoryComponent->IsValidSlotIndex(Item.TopLeftIndex))
		{
			continue;
		}

		UHeistInventoryItemWidget* ItemWidget = CreateWidget<UHeistInventoryItemWidget>(GetOwningPlayer(), UHeistInventoryItemWidget::StaticClass());
		ItemWidget->InitializeItem(this, Item, CellPitch);
		ItemWidgets.Add(ItemWidget);

		const FIntPoint Coord = InventoryComponent->IndexToCoord(Item.TopLeftIndex);
		const int32 Width = InventoryComponent->GetEffectiveWidth(Item, Item.bRotated);
		const int32 Height = InventoryComponent->GetEffectiveHeight(Item, Item.bRotated);
		UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(ItemWidget);
		CanvasSlot->SetPosition(FVector2D(Coord.X * CellPitch, Coord.Y * CellPitch));
		CanvasSlot->SetSize(FVector2D(Width * CellPitch - CellGap, Height * CellPitch - CellGap));
		CanvasSlot->SetZOrder(10);
	}
}

void UHeistInventoryDebugWidget::RefreshSelectionDetails(const TArray<FHeistInventoryItem>& Items)
{
	if (!SelectedItemText)
	{
		return;
	}

	const FHeistInventoryItem* SelectedItem = Items.FindByPredicate([this](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == SelectedInstanceId;
	});
	if (!SelectedItem)
	{
		SelectedItemText->SetText(FText::FromString(TEXT("No item selected")));
		return;
	}

	SelectedItemText->SetText(FText::FromString(FString::Printf(
		TEXT("%s\nInstance #%d\nSize %d x %d\nTop Left %d\nRotated %s\nWeight %d    Score %d"),
		*SelectedItem->ItemId.ToString(),
		SelectedItem->InstanceId,
		InventoryComponent->GetEffectiveWidth(*SelectedItem, SelectedItem->bRotated),
		InventoryComponent->GetEffectiveHeight(*SelectedItem, SelectedItem->bRotated),
		SelectedItem->TopLeftIndex,
		SelectedItem->bRotated ? TEXT("YES") : TEXT("NO"),
		SelectedItem->Weight,
		SelectedItem->ScoreValue)));
}

void UHeistInventoryDebugWidget::RefreshQuickSlots(const TArray<FHeistInventoryItem>& Items)
{
	const TArray<int32> QuickSlots = InventoryComponent->GetQuickSlots();
	for (int32 Index = 0; Index < QuickSlotWidgets.Num(); ++Index)
	{
		const int32 InstanceId = QuickSlots.IsValidIndex(Index) ? QuickSlots[Index] : INDEX_NONE;
		const FHeistInventoryItem* Item = Items.FindByPredicate([InstanceId](const FHeistInventoryItem& Candidate)
		{
			return Candidate.InstanceId == InstanceId;
		});
		QuickSlotWidgets[Index]->SetAssignedItem(InstanceId, Item ? Item->ItemId : NAME_None);
	}
}

bool UHeistInventoryDebugWidget::UpdatePlacementPreview(
	UHeistInventoryDragDropOperation* DragOperation,
	const FVector2D& ScreenPosition)
{
	ClearPlacementPreview();
	if (!InventoryComponent || !DragOperation)
	{
		return false;
	}

	bool bInsideGrid = false;
	int32 HoveredSlot = INDEX_NONE;
	int32 TopLeftIndex = INDEX_NONE;
	GetDropCoordinates(ScreenPosition, DragOperation, bInsideGrid, HoveredSlot, TopLeftIndex);
	if (!bInsideGrid)
	{
		if (DragStatusText)
		{
			DragStatusText->SetText(FText::FromString(TEXT("Release outside grid to drop item into the world")));
			DragStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.55f, 0.25f)));
		}
		return false;
	}

	const bool bValid = TopLeftIndex != INDEX_NONE && InventoryComponent->CanPlaceItem(
		DragOperation->ItemSnapshot,
		TopLeftIndex,
		DragOperation->bRotated,
		DragOperation->InstanceId);
	DragOperation->PreviewTopLeftIndex = TopLeftIndex;
	DragOperation->bPreviewValid = bValid;

	if (TopLeftIndex != INDEX_NONE)
	{
		const FIntPoint TopLeft = InventoryComponent->IndexToCoord(TopLeftIndex);
		const int32 Width = InventoryComponent->GetEffectiveWidth(DragOperation->ItemSnapshot, DragOperation->bRotated);
		const int32 Height = InventoryComponent->GetEffectiveHeight(DragOperation->ItemSnapshot, DragOperation->bRotated);
		for (int32 Y = TopLeft.Y; Y < TopLeft.Y + Height; ++Y)
		{
			for (int32 X = TopLeft.X; X < TopLeft.X + Width; ++X)
			{
				const int32 SlotIndex = InventoryComponent->CoordToIndex(X, Y);
				if (SlotWidgets.IsValidIndex(SlotIndex))
				{
					SlotWidgets[SlotIndex]->SetPreviewState(true, bValid);
				}
			}
		}
	}
	else if (SlotWidgets.IsValidIndex(HoveredSlot))
	{
		SlotWidgets[HoveredSlot]->SetPreviewState(true, false);
	}

	if (DragStatusText)
	{
		DragStatusText->SetText(FText::FromString(bValid
			? FString::Printf(TEXT("Valid target: slot %d"), TopLeftIndex)
			: FString(TEXT("Invalid target: bounds or overlap"))));
		DragStatusText->SetColorAndOpacity(FSlateColor(bValid
			? FLinearColor(0.25f, 0.9f, 0.42f)
			: FLinearColor(0.95f, 0.25f, 0.25f)));
	}
	return bValid;
}

bool UHeistInventoryDebugWidget::GetDropCoordinates(
	const FVector2D& ScreenPosition,
	const UHeistInventoryDragDropOperation* DragOperation,
	bool& bOutInsideGrid,
	int32& OutHoveredSlot,
	int32& OutTopLeftIndex) const
{
	bOutInsideGrid = false;
	OutHoveredSlot = INDEX_NONE;
	OutTopLeftIndex = INDEX_NONE;
	if (!GridCanvas || !InventoryComponent || !DragOperation)
	{
		return false;
	}

	const FGeometry GridGeometry = GridCanvas->GetCachedGeometry();
	if (!GridGeometry.IsUnderLocation(ScreenPosition))
	{
		return false;
	}

	bOutInsideGrid = true;
	const FVector2D LocalPosition = GridGeometry.AbsoluteToLocal(ScreenPosition);
	const int32 HoverX = FMath::Clamp(FMath::FloorToInt(LocalPosition.X / CellPitch), 0, InventoryComponent->GetGridWidth() - 1);
	const int32 HoverY = FMath::Clamp(FMath::FloorToInt(LocalPosition.Y / CellPitch), 0, InventoryComponent->GetGridHeight() - 1);
	OutHoveredSlot = InventoryComponent->CoordToIndex(HoverX, HoverY);

	OutTopLeftIndex = InventoryComponent->CoordToIndex(
		HoverX - DragOperation->GrabCellOffset.X,
		HoverY - DragOperation->GrabCellOffset.Y);
	return true;
}

void UHeistInventoryDebugWidget::UpdateDragVisualAnchor(UHeistInventoryDragDropOperation* DragOperation) const
{
	if (!InventoryComponent || !DragOperation)
	{
		return;
	}

	const int32 Width = InventoryComponent->GetEffectiveWidth(
		DragOperation->ItemSnapshot,
		DragOperation->bRotated);
	const int32 Height = InventoryComponent->GetEffectiveHeight(
		DragOperation->ItemSnapshot,
		DragOperation->bRotated);
	const FVector2D VisualSize(
		Width * CellPitch - CellGap,
		Height * CellPitch - CellGap);
	const FVector2D MouseAnchor(
		DragOperation->GrabCellOffset.X * CellPitch + DragOperation->GrabOffsetWithinCell.X,
		DragOperation->GrabCellOffset.Y * CellPitch + DragOperation->GrabOffsetWithinCell.Y);

	DragOperation->Pivot = EDragPivot::TopLeft;
	DragOperation->Offset = FVector2D(
		-MouseAnchor.X / VisualSize.X,
		-MouseAnchor.Y / VisualSize.Y);
}

void UHeistInventoryDebugWidget::ClearPlacementPreview()
{
	for (UHeistInventorySlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetPreviewState(false, false);
		}
	}

	if (DragStatusText)
	{
		DragStatusText->SetText(FText::FromString(TEXT("Green = valid placement    Red = rejected placement")));
		DragStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.62f, 0.72f)));
	}
}
