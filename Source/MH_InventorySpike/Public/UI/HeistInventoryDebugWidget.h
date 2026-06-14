#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeistInventoryDebugWidget.generated.h"

class UHeistInventoryComponent;
class UBorder;
class UTextBlock;
class UUniformGridPanel;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistInventoryDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryComponent(UHeistInventoryComponent* InInventoryComponent);

	void SetSelectedInstanceId(int32 InSelectedInstanceId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void RefreshInventory();

	void BuildDebugLayout();

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> GridPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> GridCells;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> GridCellBorders;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedItemText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> QuickSlotTexts;

	int32 SelectedInstanceId = INDEX_NONE;
};
