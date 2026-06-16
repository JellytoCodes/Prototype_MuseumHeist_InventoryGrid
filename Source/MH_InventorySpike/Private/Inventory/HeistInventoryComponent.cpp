#include "Inventory/HeistInventoryComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogHeistInventory, Log, All);

UHeistInventoryComponent::UHeistInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	QuickSlots.Init(INDEX_NONE, QuickSlotCount);
}

bool UHeistInventoryComponent::IsValidSlotIndex(const int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < GridWidth * GridHeight;
}

FIntPoint UHeistInventoryComponent::IndexToCoord(const int32 SlotIndex) const
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	return FIntPoint(SlotIndex % GridWidth, SlotIndex / GridWidth);
}

int32 UHeistInventoryComponent::CoordToIndex(const int32 X, const int32 Y) const
{
	if (X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight)
	{
		return INDEX_NONE;
	}

	return Y * GridWidth + X;
}

int32 UHeistInventoryComponent::GetEffectiveWidth(const FHeistInventoryItem& Item, const bool bRotated) const
{
	return bRotated ? Item.Height : Item.Width;
}

int32 UHeistInventoryComponent::GetEffectiveHeight(const FHeistInventoryItem& Item, const bool bRotated) const
{
	return bRotated ? Item.Width : Item.Height;
}

bool UHeistInventoryComponent::CanPlaceItem(
	const FHeistInventoryItem& Item,
	const int32 TopLeftIndex,
	const bool bRotated,
	const int32 IgnoredInstanceId) const
{
	if (!IsValidSlotIndex(TopLeftIndex) || Item.Width <= 0 || Item.Height <= 0)
	{
		return false;
	}

	const FIntPoint TopLeft = IndexToCoord(TopLeftIndex);
	const int32 EffectiveWidth = GetEffectiveWidth(Item, bRotated);
	const int32 EffectiveHeight = GetEffectiveHeight(Item, bRotated);
	if (TopLeft.X + EffectiveWidth > GridWidth || TopLeft.Y + EffectiveHeight > GridHeight)
	{
		return false;
	}

	TArray<int32> Occupancy;
	BuildOccupancyGrid(Occupancy, IgnoredInstanceId);

	for (int32 Y = TopLeft.Y; Y < TopLeft.Y + EffectiveHeight; ++Y)
	{
		for (int32 X = TopLeft.X; X < TopLeft.X + EffectiveWidth; ++X)
		{
			const int32 SlotIndex = CoordToIndex(X, Y);
			if (!Occupancy.IsValidIndex(SlotIndex) || Occupancy[SlotIndex] != INDEX_NONE)
			{
				return false;
			}
		}
	}

	return true;
}

bool UHeistInventoryComponent::AddItem(
	const FName ItemId,
	const FGameplayTag& ItemTag,
	const int32 Width,
	const int32 Height,
	const int32 Weight,
	const int32 ScoreValue)
{
	if (!HasMutationAuthority() || ItemId.IsNone() || Width <= 0 || Height <= 0 || Weight < 0 || ScoreValue < 0)
	{
		return false;
	}
	if (NextInstanceId <= 0 || NextInstanceId == MAX_int32 || FindItemIndexByInstanceId(NextInstanceId) != INDEX_NONE)
	{
		UE_LOG(LogHeistInventory, Error, TEXT("AddItem rejected for %s: unsafe NextInstanceId=%d"), *GetNameSafe(GetOwner()), NextInstanceId);
		return false;
	}

	FHeistInventoryItem NewItem;
	NewItem.InstanceId = NextInstanceId;
	NewItem.ItemId = ItemId;
	NewItem.ItemTag = ItemTag;
	NewItem.Width = Width;
	NewItem.Height = Height;
	NewItem.Weight = Weight;
	NewItem.ScoreValue = ScoreValue;

	for (int32 SlotIndex = 0; SlotIndex < GridWidth * GridHeight; ++SlotIndex)
	{
		if (!CanPlaceItem(NewItem, SlotIndex, false))
		{
			continue;
		}

		NewItem.TopLeftIndex = SlotIndex;
		Items.Add(NewItem);
		++NextInstanceId;
		NotifyInventoryChanged();
		return true;
	}

	return false;
}

bool UHeistInventoryComponent::MoveItem(const int32 InstanceId, const int32 NewTopLeftIndex, const bool bRotated)
{
	if (!HasMutationAuthority())
	{
		return false;
	}

	const int32 ItemIndex = FindItemIndexByInstanceId(InstanceId);
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FHeistInventoryItem& Item = Items[ItemIndex];
	if (!CanPlaceItem(Item, NewTopLeftIndex, bRotated, InstanceId))
	{
		return false;
	}

	Item.TopLeftIndex = NewTopLeftIndex;
	Item.bRotated = bRotated;
	NotifyInventoryChanged();
	return true;
}

bool UHeistInventoryComponent::RemoveItem(const int32 InstanceId)
{
	if (!HasMutationAuthority())
	{
		return false;
	}

	const int32 ItemIndex = FindItemIndexByInstanceId(InstanceId);
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	Items.RemoveAt(ItemIndex);
	ClearQuickSlotReferences(InstanceId);
	NotifyInventoryChanged();
	return true;
}

bool UHeistInventoryComponent::DropItem(const int32 InstanceId, FHeistInventoryItem& DroppedItem)
{
	DroppedItem = FHeistInventoryItem();

	if (!HasMutationAuthority())
	{
		return false;
	}

	const int32 ItemIndex = FindItemIndexByInstanceId(InstanceId);
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	DroppedItem = Items[ItemIndex];
	Items.RemoveAt(ItemIndex);
	ClearQuickSlotReferences(InstanceId);
	NotifyInventoryChanged();
	OnItemDropped.Broadcast(DroppedItem);
	return true;
}

int32 UHeistInventoryComponent::GetTotalWeight() const
{
	int32 TotalWeight = 0;
	for (const FHeistInventoryItem& Item : Items)
	{
		TotalWeight += Item.Weight;
	}
	return TotalWeight;
}

int32 UHeistInventoryComponent::GetTotalScore() const
{
	int32 TotalScore = 0;
	for (const FHeistInventoryItem& Item : Items)
	{
		TotalScore += Item.ScoreValue;
	}
	return TotalScore;
}

int32 UHeistInventoryComponent::GetUsedSlotCount() const
{
	TArray<int32> Occupancy;
	BuildOccupancyGrid(Occupancy);

	int32 UsedSlotCount = 0;
	for (const int32 InstanceId : Occupancy)
	{
		UsedSlotCount += InstanceId != INDEX_NONE ? 1 : 0;
	}
	return UsedSlotCount;
}

bool UHeistInventoryComponent::HasFreeSpaceFor(const FHeistInventoryItem& Item) const
{
	for (int32 SlotIndex = 0; SlotIndex < GridWidth * GridHeight; ++SlotIndex)
	{
		if (CanPlaceItem(Item, SlotIndex, Item.bRotated, Item.InstanceId))
		{
			return true;
		}
	}
	return false;
}

bool UHeistInventoryComponent::AssignQuickSlot(const int32 SlotIndex, const int32 InstanceId)
{
	if (!HasMutationAuthority() || !QuickSlots.IsValidIndex(SlotIndex) || FindItemIndexByInstanceId(InstanceId) == INDEX_NONE)
	{
		return false;
	}

	// One inventory instance maps to at most one input slot, but the item remains in the backpack.
	ClearQuickSlotReferences(InstanceId);
	QuickSlots[SlotIndex] = InstanceId;
	NotifyInventoryChanged();
	return true;
}

bool UHeistInventoryComponent::ClearQuickSlot(const int32 SlotIndex)
{
	if (!HasMutationAuthority() || !QuickSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (QuickSlots[SlotIndex] == INDEX_NONE)
	{
		return true;
	}

	QuickSlots[SlotIndex] = INDEX_NONE;
	NotifyInventoryChanged();
	return true;
}

int32 UHeistInventoryComponent::GetQuickSlotItemInstanceId(const int32 SlotIndex) const
{
	return QuickSlots.IsValidIndex(SlotIndex) ? QuickSlots[SlotIndex] : INDEX_NONE;
}

FString UHeistInventoryComponent::GetDebugInventoryString() const
{
	TArray<int32> Occupancy;
	BuildOccupancyGrid(Occupancy);

	FString Result = FString::Printf(TEXT("Inventory %dx%d\n"), GridWidth, GridHeight);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const int32 InstanceId = Occupancy[CoordToIndex(X, Y)];
			Result += InstanceId == INDEX_NONE
				? TEXT("[---]")
				: FString::Printf(TEXT("[%3d]"), InstanceId);
		}
		Result += TEXT("\n");
	}

	Result += TEXT("Items:\n");
	for (const FHeistInventoryItem& Item : Items)
	{
		Result += FString::Printf(
			TEXT("  #%d %s TopLeft=%d Size=%dx%d Rotated=%s\n"),
			Item.InstanceId,
			*Item.ItemId.ToString(),
			Item.TopLeftIndex,
			Item.Width,
			Item.Height,
			Item.bRotated ? TEXT("true") : TEXT("false"));
	}

	Result += FString::Printf(TEXT("Weight: %d\nScore: %d\n"), GetTotalWeight(), GetTotalScore());
	static const TCHAR* SlotNames[QuickSlotCount] = { TEXT("Q"), TEXT("E"), TEXT("R") };
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotCount; ++SlotIndex)
	{
		const int32 InstanceId = GetQuickSlotItemInstanceId(SlotIndex);
		Result += InstanceId == INDEX_NONE
			? FString::Printf(TEXT("%s: None\n"), SlotNames[SlotIndex])
			: FString::Printf(TEXT("%s: %d\n"), SlotNames[SlotIndex], InstanceId);
	}

	return Result;
}

FString UHeistInventoryComponent::GetInventoryValidationReport() const
{
	TArray<FString> Errors;
	TSet<int32> SeenInstanceIds;
	int32 HighestInstanceId = 0;
	TArray<int32> Occupancy;
	Occupancy.Init(INDEX_NONE, GridWidth * GridHeight);

	for (const FHeistInventoryItem& Item : Items)
	{
		if (Item.InstanceId <= 0 || SeenInstanceIds.Contains(Item.InstanceId))
		{
			Errors.Add(FString::Printf(TEXT("Invalid or duplicate InstanceId %d"), Item.InstanceId));
		}
		SeenInstanceIds.Add(Item.InstanceId);
		HighestInstanceId = FMath::Max(HighestInstanceId, Item.InstanceId);

		if (!IsValidSlotIndex(Item.TopLeftIndex) || Item.Width <= 0 || Item.Height <= 0)
		{
			Errors.Add(FString::Printf(TEXT("Item #%d has invalid placement metadata"), Item.InstanceId));
			continue;
		}

		const FIntPoint TopLeft = IndexToCoord(Item.TopLeftIndex);
		const int32 EffectiveWidth = GetEffectiveWidth(Item, Item.bRotated);
		const int32 EffectiveHeight = GetEffectiveHeight(Item, Item.bRotated);
		if (TopLeft.X + EffectiveWidth > GridWidth || TopLeft.Y + EffectiveHeight > GridHeight)
		{
			Errors.Add(FString::Printf(TEXT("Item #%d exceeds grid bounds"), Item.InstanceId));
			continue;
		}

		for (int32 Y = TopLeft.Y; Y < TopLeft.Y + EffectiveHeight; ++Y)
		{
			for (int32 X = TopLeft.X; X < TopLeft.X + EffectiveWidth; ++X)
			{
				const int32 SlotIndex = CoordToIndex(X, Y);
				if (!Occupancy.IsValidIndex(SlotIndex))
				{
					Errors.Add(FString::Printf(TEXT("Item #%d references invalid slot"), Item.InstanceId));
					continue;
				}
				if (Occupancy[SlotIndex] != INDEX_NONE)
				{
					Errors.Add(FString::Printf(
						TEXT("Items #%d and #%d overlap at slot %d"),
						Occupancy[SlotIndex],
						Item.InstanceId,
						SlotIndex));
				}
				Occupancy[SlotIndex] = Item.InstanceId;
			}
		}
	}

	if (QuickSlots.Num() != QuickSlotCount)
	{
		Errors.Add(FString::Printf(TEXT("QuickSlot count is %d, expected %d"), QuickSlots.Num(), QuickSlotCount));
	}
	for (int32 SlotIndex = 0; SlotIndex < QuickSlots.Num(); ++SlotIndex)
	{
		const int32 InstanceId = QuickSlots[SlotIndex];
		if (InstanceId != INDEX_NONE && !SeenInstanceIds.Contains(InstanceId))
		{
			Errors.Add(FString::Printf(TEXT("QuickSlot %d has stale InstanceId %d"), SlotIndex, InstanceId));
		}
	}
	if (HasMutationAuthority() && NextInstanceId <= HighestInstanceId)
	{
		Errors.Add(FString::Printf(
			TEXT("NextInstanceId %d is not above active maximum %d"),
			NextInstanceId,
			HighestInstanceId));
	}

	if (!Errors.IsEmpty())
	{
		return FString::Printf(TEXT("INVALID:\n%s"), *FString::Join(Errors, TEXT("\n")));
	}

	return HasMutationAuthority()
		? FString::Printf(TEXT("VALID SERVER: %d items, %d used slots, NextInstanceId=%d"), Items.Num(), GetUsedSlotCount(), NextInstanceId)
		: FString::Printf(TEXT("VALID CLIENT SNAPSHOT: %d items, %d used slots"), Items.Num(), GetUsedSlotCount());
}

void UHeistInventoryComponent::Server_RequestMoveItem_Implementation(
	const int32 InstanceId,
	const int32 NewTopLeftIndex,
	const bool bRotated)
{
	EHeistInventoryRequestResult Result = EHeistInventoryRequestResult::AuthorityDenied;
	if (CanProcessServerRequest())
	{
		const int32 ItemIndex = FindItemIndexByInstanceId(InstanceId);
		if (!Items.IsValidIndex(ItemIndex))
		{
			Result = EHeistInventoryRequestResult::ItemNotFound;
		}
		else if (!CanPlaceItem(Items[ItemIndex], NewTopLeftIndex, bRotated, InstanceId))
		{
			Result = EHeistInventoryRequestResult::InvalidPlacement;
		}
		else
		{
			Result = MoveItem(InstanceId, NewTopLeftIndex, bRotated)
				? EHeistInventoryRequestResult::Success
				: EHeistInventoryRequestResult::MutationRejected;
		}
	}
	ResolveServerRequest(EHeistInventoryRequestType::MoveItem, Result);
}

void UHeistInventoryComponent::Server_RequestDropItem_Implementation(const int32 InstanceId)
{
	EHeistInventoryRequestResult Result = EHeistInventoryRequestResult::AuthorityDenied;
	if (CanProcessServerRequest())
	{
		if (FindItemIndexByInstanceId(InstanceId) == INDEX_NONE)
		{
			Result = EHeistInventoryRequestResult::ItemNotFound;
		}
		else
		{
			FHeistInventoryItem DroppedItem;
			Result = DropItem(InstanceId, DroppedItem)
				? EHeistInventoryRequestResult::Success
				: EHeistInventoryRequestResult::MutationRejected;
		}
	}
	ResolveServerRequest(EHeistInventoryRequestType::DropItem, Result);
}

void UHeistInventoryComponent::Server_RequestAssignQuickSlot_Implementation(const int32 SlotIndex, const int32 InstanceId)
{
	EHeistInventoryRequestResult Result = EHeistInventoryRequestResult::AuthorityDenied;
	if (CanProcessServerRequest())
	{
		if (!QuickSlots.IsValidIndex(SlotIndex))
		{
			Result = EHeistInventoryRequestResult::InvalidQuickSlotIndex;
		}
		else if (FindItemIndexByInstanceId(InstanceId) == INDEX_NONE)
		{
			Result = EHeistInventoryRequestResult::InvalidQuickSlotItem;
		}
		else
		{
			Result = AssignQuickSlot(SlotIndex, InstanceId)
				? EHeistInventoryRequestResult::Success
				: EHeistInventoryRequestResult::MutationRejected;
		}
	}
	ResolveServerRequest(EHeistInventoryRequestType::AssignQuickSlot, Result);
}

void UHeistInventoryComponent::Client_InventoryRequestResolved_Implementation(
	const EHeistInventoryRequestType RequestType,
	const EHeistInventoryRequestResult Result)
{
	if (Result != EHeistInventoryRequestResult::Success)
	{
		// Rejected requests do not change replicated properties, so force the UI to
		// rebuild from the unchanged confirmed state instead of retaining preview state.
		OnInventoryChanged.Broadcast();
	}
	OnInventoryRequestResolved.Broadcast(RequestType, Result);
}

bool UHeistInventoryComponent::IsInventoryMutationAllowed_Implementation() const
{
	return true;
}

void UHeistInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHeistInventoryComponent, Items, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UHeistInventoryComponent, QuickSlots, COND_OwnerOnly);
}

void UHeistInventoryComponent::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
}

bool UHeistInventoryComponent::HasMutationAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

bool UHeistInventoryComponent::CanProcessServerRequest() const
{
	return HasMutationAuthority() && IsInventoryMutationAllowed();
}

int32 UHeistInventoryComponent::FindItemIndexByInstanceId(const int32 InstanceId) const
{
	return Items.IndexOfByPredicate([InstanceId](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == InstanceId;
	});
}

void UHeistInventoryComponent::BuildOccupancyGrid(TArray<int32>& OutOccupancy, const int32 IgnoredInstanceId) const
{
	OutOccupancy.Init(INDEX_NONE, GridWidth * GridHeight);

	for (const FHeistInventoryItem& Item : Items)
	{
		if (Item.InstanceId == IgnoredInstanceId || !IsValidSlotIndex(Item.TopLeftIndex))
		{
			continue;
		}

		const FIntPoint TopLeft = IndexToCoord(Item.TopLeftIndex);
		const int32 EffectiveWidth = GetEffectiveWidth(Item, Item.bRotated);
		const int32 EffectiveHeight = GetEffectiveHeight(Item, Item.bRotated);
		for (int32 Y = TopLeft.Y; Y < TopLeft.Y + EffectiveHeight; ++Y)
		{
			for (int32 X = TopLeft.X; X < TopLeft.X + EffectiveWidth; ++X)
			{
				const int32 SlotIndex = CoordToIndex(X, Y);
				if (OutOccupancy.IsValidIndex(SlotIndex))
				{
					OutOccupancy[SlotIndex] = Item.InstanceId;
				}
			}
		}
	}
}

void UHeistInventoryComponent::ClearQuickSlotReferences(const int32 InstanceId)
{
	for (int32& QuickSlotInstanceId : QuickSlots)
	{
		if (QuickSlotInstanceId == InstanceId)
		{
			QuickSlotInstanceId = INDEX_NONE;
		}
	}
}

void UHeistInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast();
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UHeistInventoryComponent::ResolveServerRequest(
	const EHeistInventoryRequestType RequestType,
	const EHeistInventoryRequestResult Result)
{
	if (Result == EHeistInventoryRequestResult::Success)
	{
		UE_LOG(
			LogHeistInventory,
			Verbose,
			TEXT("Inventory request owner=%s type=%d result=Success state=%s"),
			*GetNameSafe(GetOwner()),
			static_cast<int32>(RequestType),
			*GetInventoryValidationReport());
	}
	else
	{
		UE_LOG(
			LogHeistInventory,
			Warning,
			TEXT("Inventory request rejected owner=%s type=%d result=%d state=%s"),
			*GetNameSafe(GetOwner()),
			static_cast<int32>(RequestType),
			static_cast<int32>(Result),
			*GetInventoryValidationReport());
	}
	Client_InventoryRequestResolved(RequestType, Result);
}
