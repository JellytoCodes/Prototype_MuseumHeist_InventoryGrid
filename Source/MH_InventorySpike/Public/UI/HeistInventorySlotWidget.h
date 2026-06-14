#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistInventorySlotWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeSlot(int32 InSlotIndex);
	void SetPreviewState(bool bPreviewed, bool bValid);

protected:
	virtual void NativeConstruct() override;

private:
	void BuildLayout();
	void RefreshVisual();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SlotBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlotText;

	int32 SlotIndex = INDEX_NONE;
	bool bIsPreviewed = false;
	bool bIsValidPreview = false;
};
