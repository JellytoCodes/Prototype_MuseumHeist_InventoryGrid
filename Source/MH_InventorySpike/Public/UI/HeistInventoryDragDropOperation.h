#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/HeistInventoryTypes.h"
#include "HeistInventoryDragDropOperation.generated.h"

class UHeistInventoryDebugWidget;

UCLASS()
class MH_INVENTORYSPIKE_API UHeistInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	int32 OriginalTopLeftIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	bool bRotated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	FIntPoint GrabCellOffset = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	FVector2D GrabOffsetWithinCell = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag")
	FHeistInventoryItem ItemSnapshot;

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryDebugWidget> InventoryRoot;

	int32 PreviewTopLeftIndex = INDEX_NONE;
	bool bPreviewValid = false;
};
