#include "UI/HeistInventorySlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UHeistInventorySlotWidget::InitializeSlot(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	RefreshVisual();
}

void UHeistInventorySlotWidget::SetPreviewState(const bool bPreviewed, const bool bValid)
{
	bIsPreviewed = bPreviewed;
	bIsValidPreview = bValid;
	RefreshVisual();
}

void UHeistInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	RefreshVisual();
}

void UHeistInventorySlotWidget::BuildLayout()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBorder"));
	SlotBorder->SetPadding(FMargin(4.0f));
	WidgetTree->RootWidget = SlotBorder;

	SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotText"));
	SlotText->SetJustification(ETextJustify::Center);
	SlotText->SetColorAndOpacity(FSlateColor(FLinearColor(0.28f, 0.35f, 0.44f)));
	SlotBorder->SetContent(SlotText);
}

void UHeistInventorySlotWidget::RefreshVisual()
{
	if (!SlotBorder || !SlotText)
	{
		return;
	}

	if (bIsPreviewed)
	{
		SlotBorder->SetBrushColor(bIsValidPreview
			? FLinearColor(0.12f, 0.62f, 0.28f, 0.9f)
			: FLinearColor(0.72f, 0.12f, 0.12f, 0.9f));
	}
	else
	{
		SlotBorder->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.105f, 1.0f));
	}

	SlotText->SetText(SlotIndex == INDEX_NONE
		? FText::GetEmpty()
		: FText::AsNumber(SlotIndex));
}
