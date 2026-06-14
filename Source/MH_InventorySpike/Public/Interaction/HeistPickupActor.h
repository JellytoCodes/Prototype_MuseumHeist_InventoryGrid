#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/HeistInventoryTypes.h"
#include "HeistPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UHeistInventoryComponent;

UENUM(BlueprintType)
enum class EHeistPickupCase : uint8
{
	Case1_Coin UMETA(DisplayName = "Case 1 - Coin 1x1"),
	Case2_GlueTrap UMETA(DisplayName = "Case 2 - Glue Trap 2x1"),
	Case3_Painting UMETA(DisplayName = "Case 3 - Painting 1x3")
};

UCLASS(Blueprintable)
class MH_INVENTORYSPIKE_API AHeistPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AHeistPickupActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ConfigureCase(EHeistPickupCase NewPickupCase);

	void InitializeFromInventoryItem(const FHeistInventoryItem& Item);
	bool TryPickup(UHeistInventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintPure, Category = "Pickup")
	FText GetInteractionText() const;

private:
	void ApplyCasePreset();
	void UpdateVisuals();

	UFUNCTION()
	void OnRep_ItemData();

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UTextRenderComponent> PickupLabel;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, BlueprintReadOnly, Category = "Pickup|Test Case", meta = (AllowPrivateAccess = "true"))
	EHeistPickupCase PickupCase = EHeistPickupCase::Case1_Coin;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Test Case")
	bool bUseCasePreset = true;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item")
	FName ItemId = TEXT("Loot_Coin");

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item")
	FGameplayTag ItemTag;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item", meta = (ClampMin = "1"))
	int32 ItemWidth = 1;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item", meta = (ClampMin = "1"))
	int32 ItemHeight = 1;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item", meta = (ClampMin = "0"))
	int32 ItemWeight = 1;

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, Category = "Pickup|Item", meta = (ClampMin = "0"))
	int32 ItemScoreValue = 100;
};
