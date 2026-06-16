#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HeistInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class EHeistInventoryRequestType : uint8
{
	MoveItem,
	DropItem,
	AssignQuickSlot
};

UENUM(BlueprintType)
enum class EHeistInventoryRequestResult : uint8
{
	Success,
	AuthorityDenied,
	ItemNotFound,
	InvalidPlacement,
	InvalidQuickSlotIndex,
	InvalidQuickSlotItem,
	MutationRejected
};

USTRUCT(BlueprintType)
struct MH_INVENTORYSPIKE_API FHeistInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Height = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0"))
	int32 ScoreValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 TopLeftIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bRotated = false;
};
