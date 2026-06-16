#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/HeistInventoryTypes.h"
#include "HeistInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHeistInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeistItemDropped, FHeistInventoryItem, DroppedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FHeistInventoryRequestResolved,
	EHeistInventoryRequestType, RequestType,
	EHeistInventoryRequestResult, Result);

UCLASS(ClassGroup = (MuseumHeist), meta = (BlueprintSpawnableComponent))
class MH_INVENTORYSPIKE_API UHeistInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeistInventoryComponent();

	static constexpr int32 DefaultGridWidth = 4;
	static constexpr int32 DefaultGridHeight = 5;
	static constexpr int32 QuickSlotCount = 3;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FHeistInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FHeistItemDropped OnItemDropped;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Replication")
	FHeistInventoryRequestResolved OnInventoryRequestResolved;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	int32 GetGridWidth() const { return GridWidth; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	int32 GetGridHeight() const { return GridHeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FHeistInventoryItem> GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Inventory|QuickSlots")
	TArray<int32> GetQuickSlots() const { return QuickSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	bool IsValidSlotIndex(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	FIntPoint IndexToCoord(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	int32 CoordToIndex(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	int32 GetEffectiveWidth(const FHeistInventoryItem& Item, bool bRotated) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	int32 GetEffectiveHeight(const FHeistInventoryItem& Item, bool bRotated) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Grid")
	bool CanPlaceItem(const FHeistInventoryItem& Item, int32 TopLeftIndex, bool bRotated, int32 IgnoredInstanceId = -1) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Mutation")
	bool AddItem(FName ItemId, const FGameplayTag& ItemTag, int32 Width, int32 Height, int32 Weight, int32 ScoreValue);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Mutation")
	bool MoveItem(int32 InstanceId, int32 NewTopLeftIndex, bool bRotated);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Mutation")
	bool RemoveItem(int32 InstanceId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Mutation")
	bool DropItem(int32 InstanceId, FHeistInventoryItem& DroppedItem);

	UFUNCTION(BlueprintPure, Category = "Inventory|Calculation")
	int32 GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Calculation")
	int32 GetTotalScore() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Calculation")
	int32 GetUsedSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Calculation")
	bool HasFreeSpaceFor(const FHeistInventoryItem& Item) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|QuickSlots")
	bool AssignQuickSlot(int32 SlotIndex, int32 InstanceId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|QuickSlots")
	bool ClearQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory|QuickSlots")
	int32 GetQuickSlotItemInstanceId(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Debug")
	FString GetDebugInventoryString() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Debug")
	FString GetInventoryValidationReport() const;

	UFUNCTION(Server, Reliable, Category = "Inventory|Server Requests")
	void Server_RequestMoveItem(int32 InstanceId, int32 NewTopLeftIndex, bool bRotated);

	UFUNCTION(Server, Reliable, Category = "Inventory|Server Requests")
	void Server_RequestDropItem(int32 InstanceId);

	UFUNCTION(Server, Reliable, Category = "Inventory|Server Requests")
	void Server_RequestAssignQuickSlot(int32 SlotIndex, int32 InstanceId);

	UFUNCTION(Client, Reliable, Category = "Inventory|Server Requests")
	void Client_InventoryRequestResolved(EHeistInventoryRequestType RequestType, EHeistInventoryRequestResult Result);

	UFUNCTION(BlueprintNativeEvent, Category = "Inventory|Authority")
	bool IsInventoryMutationAllowed() const;
	virtual bool IsInventoryMutationAllowed_Implementation() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (AllowPrivateAccess = "true"))
	int32 GridWidth = DefaultGridWidth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (AllowPrivateAccess = "true"))
	int32 GridHeight = DefaultGridHeight;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FHeistInventoryItem> Items;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory|QuickSlots", meta = (AllowPrivateAccess = "true"))
	TArray<int32> QuickSlots;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	int32 NextInstanceId = 1;

	UFUNCTION()
	void OnRep_InventoryState();

	bool HasMutationAuthority() const;
	bool CanProcessServerRequest() const;
	int32 FindItemIndexByInstanceId(int32 InstanceId) const;
	void BuildOccupancyGrid(TArray<int32>& OutOccupancy, int32 IgnoredInstanceId = INDEX_NONE) const;
	void ClearQuickSlotReferences(int32 InstanceId);
	void NotifyInventoryChanged();
	void ResolveServerRequest(EHeistInventoryRequestType RequestType, EHeistInventoryRequestResult Result);
};
