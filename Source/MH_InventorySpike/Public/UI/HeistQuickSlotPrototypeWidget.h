#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistQuickSlotPrototypeWidget.generated.h"

class UBorder;
class UDragDropOperation;
class UHeistInventoryComponent;
class UTextBlock;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistQuickSlotPrototypeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeQuickSlot(UHeistInventoryComponent* InInventoryComponent, int32 InQuickSlotIndex);
	void SetAssignedItem(int32 InstanceId, FName ItemId);

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildLayout();
	void RefreshVisual(bool bDragHover = false);

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SlotBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlotText;

	int32 QuickSlotIndex = INDEX_NONE;
	int32 AssignedInstanceId = INDEX_NONE;
	FName AssignedItemId = NAME_None;
};
