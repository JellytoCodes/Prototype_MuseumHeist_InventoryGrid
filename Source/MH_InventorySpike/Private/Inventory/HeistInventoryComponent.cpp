#include "Inventory/HeistInventoryComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

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

void UHeistInventoryComponent::Server_RequestMoveItem_Implementation(
	const int32 InstanceId,
	const int32 NewTopLeftIndex,
	const bool bRotated)
{
	if (CanProcessServerRequest())
	{
		MoveItem(InstanceId, NewTopLeftIndex, bRotated);
	}
}

void UHeistInventoryComponent::Server_RequestDropItem_Implementation(const int32 InstanceId)
{
	if (!CanProcessServerRequest())
	{
		return;
	}

	FHeistInventoryItem DroppedItem;
	DropItem(InstanceId, DroppedItem);
}

void UHeistInventoryComponent::Server_RequestAssignQuickSlot_Implementation(const int32 SlotIndex, const int32 InstanceId)
{
	if (CanProcessServerRequest())
	{
		AssignQuickSlot(SlotIndex, InstanceId);
	}
}

bool UHeistInventoryComponent::IsInventoryMutationAllowed_Implementation() const
{
	return true;
}

void UHeistInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHeistInventoryComponent, Items);
	DOREPLIFETIME(UHeistInventoryComponent, QuickSlots);
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
