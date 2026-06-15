#pragma once

#include "CoreMinimal.h"
#include "Inventory/HeistInventoryTypes.h"
#include "Blueprint/UserWidget.h"
#include "HeistInventoryDebugWidget.generated.h"

class UCanvasPanel;
class UDragDropOperation;
class UHeistInventoryComponent;
class UHeistInventoryDragDropOperation;
class UHeistInventoryItemWidget;
class UHeistInventorySlotWidget;
class UHeistQuickSlotPrototypeWidget;
class UTextBlock;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistInventoryDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryComponent(UHeistInventoryComponent* InInventoryComponent);

	void SetSelectedInstanceId(int32 InSelectedInstanceId);
	void SelectItem(int32 InstanceId);
	void RequestRotateItem(int32 InstanceId);
	void NotifyDragStarted(UHeistInventoryDragDropOperation* DragOperation);
	void NotifyDragFinished();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UFUNCTION()
	void RefreshInventory();

	void BuildPrototypeLayout();
	void RebuildItemWidgets(const TArray<FHeistInventoryItem>& Items);
	void RefreshSelectionDetails(const TArray<FHeistInventoryItem>& Items);
	void RefreshQuickSlots(const TArray<FHeistInventoryItem>& Items);
	bool UpdatePlacementPreview(UHeistInventoryDragDropOperation* DragOperation, const FVector2D& ScreenPosition);
	bool GetDropCoordinates(const FVector2D& ScreenPosition, const UHeistInventoryDragDropOperation* DragOperation, bool& bOutInsideGrid, int32& OutHoveredSlot, int32& OutTopLeftIndex) const;
	void UpdateDragVisualAnchor(UHeistInventoryDragDropOperation* DragOperation) const;
	void ClearPlacementPreview();

	static constexpr float CellSize = 72.0f;
	static constexpr float CellGap = 4.0f;
	static constexpr float CellPitch = CellSize + CellGap;

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHeistInventorySlotWidget>> SlotWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHeistInventoryItemWidget>> ItemWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHeistQuickSlotPrototypeWidget>> QuickSlotWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedItemText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DragStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryDragDropOperation> ActiveDragOperation;

	int32 SelectedInstanceId = INDEX_NONE;
};
