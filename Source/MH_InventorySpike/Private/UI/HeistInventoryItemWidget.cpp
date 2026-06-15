#include "UI/HeistInventoryItemWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "UI/HeistInventoryDebugWidget.h"
#include "UI/HeistInventoryDragDropOperation.h"

namespace
{
	FLinearColor GetInventoryItemColor(const int32 InstanceId)
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

void UHeistInventoryItemWidget::InitializeItem(
	UHeistInventoryDebugWidget* InInventoryRoot,
	const FHeistInventoryItem& InItem,
	const float InCellPitch,
	const bool bInDragVisual)
{
	InventoryRoot = InInventoryRoot;
	Item = InItem;
	CellPitch = InCellPitch;
	bDragVisual = bInDragVisual;
	BuildLayout();
	RefreshVisual();
}

void UHeistInventoryItemWidget::SetPreviewRotation(const bool bInRotated)
{
	Item.bRotated = bInRotated;
	RefreshVisual();
}

void UHeistInventoryItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	RefreshVisual();
}

FReply UHeistInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragVisual || !InventoryRoot)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		InventoryRoot->SelectItem(Item.InstanceId);
		InventoryRoot->RequestRotateItem(Item.InstanceId);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InventoryRoot->SelectItem(Item.InstanceId);
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UHeistInventoryItemWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UHeistInventoryDragDropOperation* DragOperation = Cast<UHeistInventoryDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UHeistInventoryDragDropOperation::StaticClass()));
	if (!DragOperation)
	{
		return;
	}

	const int32 EffectiveWidth = Item.bRotated ? Item.Height : Item.Width;
	const int32 EffectiveHeight = Item.bRotated ? Item.Width : Item.Height;
	const FVector2D LocalPointer = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	DragOperation->InstanceId = Item.InstanceId;
	DragOperation->OriginalTopLeftIndex = Item.TopLeftIndex;
	DragOperation->bRotated = Item.bRotated;
	DragOperation->ItemSnapshot = Item;
	DragOperation->InventoryRoot = InventoryRoot;
	DragOperation->GrabCellOffset = FIntPoint(
		FMath::Clamp(FMath::FloorToInt(LocalPointer.X / CellPitch), 0, EffectiveWidth - 1),
		FMath::Clamp(FMath::FloorToInt(LocalPointer.Y / CellPitch), 0, EffectiveHeight - 1));
	DragOperation->GrabOffsetWithinCell = FVector2D(
		FMath::Clamp(static_cast<float>(FMath::Fmod(LocalPointer.X, static_cast<double>(CellPitch))), 0.0f, CellPitch - 4.0f),
		FMath::Clamp(static_cast<float>(FMath::Fmod(LocalPointer.Y, static_cast<double>(CellPitch))), 0.0f, CellPitch - 4.0f));

	UHeistInventoryItemWidget* DragVisual = CreateWidget<UHeistInventoryItemWidget>(GetOwningPlayer(), StaticClass());
	if (DragVisual)
	{
		DragVisual->InitializeItem(nullptr, Item, CellPitch, true);
		DragOperation->DefaultDragVisual = DragVisual;
	}
	OutOperation = DragOperation;

	if (InventoryRoot)
	{
		InventoryRoot->NotifyDragStarted(DragOperation);
	}
}

void UHeistInventoryItemWidget::BuildLayout()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	ItemSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ItemSizeBox"));
	WidgetTree->RootWidget = ItemSizeBox;

	ItemBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemBorder"));
	ItemBorder->SetPadding(FMargin(5.0f));
	ItemSizeBox->SetContent(ItemBorder);

	ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemText"));
	ItemText->SetJustification(ETextJustify::Center);
	ItemText->SetAutoWrapText(true);
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemBorder->SetContent(ItemText);
}

void UHeistInventoryItemWidget::RefreshVisual()
{
	if (!ItemBorder || !ItemText || !ItemSizeBox)
	{
		return;
	}

	const int32 EffectiveWidth = Item.bRotated ? Item.Height : Item.Width;
	const int32 EffectiveHeight = Item.bRotated ? Item.Width : Item.Height;
	ItemSizeBox->SetWidthOverride(EffectiveWidth * CellPitch - 4.0f);
	ItemSizeBox->SetHeightOverride(EffectiveHeight * CellPitch - 4.0f);

	FLinearColor Color = GetInventoryItemColor(Item.InstanceId);
	if (bDragVisual)
	{
		Color.A = 0.8f;
	}
	ItemBorder->SetBrushColor(Color);
	ItemText->SetText(FText::FromString(FString::Printf(
		TEXT("%s\n#%d\n%dx%d"),
		*Item.ItemId.ToString(),
		Item.InstanceId,
		EffectiveWidth,
		EffectiveHeight)));
}
