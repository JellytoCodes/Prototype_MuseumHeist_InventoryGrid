#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/HeistInventoryTypes.h"
#include "HeistInventoryItemWidget.generated.h"

class UBorder;
class UDragDropOperation;
class UHeistInventoryDebugWidget;
class USizeBox;
class UTextBlock;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeItem(UHeistInventoryDebugWidget* InInventoryRoot, const FHeistInventoryItem& InItem, float InCellPitch, bool bInDragVisual = false);
	void SetPreviewRotation(bool bInRotated);
	int32 GetInstanceId() const { return Item.InstanceId; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	void BuildLayout();
	void RefreshVisual();

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryDebugWidget> InventoryRoot;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ItemBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ItemText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ItemSizeBox;

	FHeistInventoryItem Item;
	float CellPitch = 76.0f;
	bool bDragVisual = false;
};
