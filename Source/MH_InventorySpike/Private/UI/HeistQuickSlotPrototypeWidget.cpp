#include "UI/HeistQuickSlotPrototypeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Inventory/HeistInventoryComponent.h"
#include "UI/HeistInventoryDebugWidget.h"
#include "UI/HeistInventoryDragDropOperation.h"

void UHeistQuickSlotPrototypeWidget::InitializeQuickSlot(UHeistInventoryComponent* InInventoryComponent, const int32 InQuickSlotIndex)
{
	InventoryComponent = InInventoryComponent;
	QuickSlotIndex = InQuickSlotIndex;
	RefreshVisual();
}

void UHeistQuickSlotPrototypeWidget::SetAssignedItem(const int32 InstanceId, const FName ItemId)
{
	AssignedInstanceId = InstanceId;
	AssignedItemId = ItemId;
	RefreshVisual();
}

void UHeistQuickSlotPrototypeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	RefreshVisual();
}

bool UHeistQuickSlotPrototypeWidget::NativeOnDragOver(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (Cast<UHeistInventoryDragDropOperation>(InOperation))
	{
		RefreshVisual(true);
		return true;
	}
	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

void UHeistQuickSlotPrototypeWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	RefreshVisual(false);
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
}

bool UHeistQuickSlotPrototypeWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	RefreshVisual(false);
	UHeistInventoryDragDropOperation* DragOperation = Cast<UHeistInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !InventoryComponent || QuickSlotIndex == INDEX_NONE)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	InventoryComponent->Server_RequestAssignQuickSlot(QuickSlotIndex, DragOperation->InstanceId);
	if (DragOperation->InventoryRoot)
	{
		DragOperation->InventoryRoot->NotifyDragFinished();
	}
	return true;
}

void UHeistQuickSlotPrototypeWidget::BuildLayout()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuickSlotBorder"));
	SlotBorder->SetPadding(FMargin(12.0f, 10.0f));
	WidgetTree->RootWidget = SlotBorder;

	SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuickSlotText"));
	SlotText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.94f)));
	SlotBorder->SetContent(SlotText);
}

void UHeistQuickSlotPrototypeWidget::RefreshVisual(const bool bDragHover)
{
	if (!SlotBorder || !SlotText)
	{
		return;
	}

	static const TCHAR* SlotNames[] = { TEXT("Q"), TEXT("E"), TEXT("R") };
	const FString SlotName = QuickSlotIndex >= 0 && QuickSlotIndex < UE_ARRAY_COUNT(SlotNames)
		? FString(SlotNames[QuickSlotIndex])
		: FString(TEXT("?"));
	const FString Assignment = AssignedInstanceId == INDEX_NONE
		? FString(TEXT("Empty"))
		: FString::Printf(TEXT("#%d  %s"), AssignedInstanceId, *AssignedItemId.ToString());

	SlotBorder->SetBrushColor(bDragHover
		? FLinearColor(0.15f, 0.55f, 0.28f, 1.0f)
		: FLinearColor(0.07f, 0.095f, 0.13f, 1.0f));
	SlotText->SetText(FText::FromString(FString::Printf(TEXT("%s  /  %s"), *SlotName, *Assignment)));
}
