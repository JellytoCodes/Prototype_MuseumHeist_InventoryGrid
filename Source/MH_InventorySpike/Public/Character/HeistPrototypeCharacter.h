#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inventory/HeistInventoryTypes.h"
#include "HeistPrototypeCharacter.generated.h"

class UCameraComponent;
class UHeistInventoryComponent;
class UHeistInventoryDebugWidget;
class USpringArmComponent;

UCLASS()
class MH_INVENTORYSPIKE_API AHeistPrototypeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHeistPrototypeCharacter();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UHeistInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryOpen() const { return bInventoryOpen; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ToggleInventory();
	void SetInventoryOpen(bool bOpen);
	void Interact();
	void SelectPreviousItem();
	void SelectNextItem();
	void MoveSelectedUp();
	void MoveSelectedDown();
	void MoveSelectedLeft();
	void MoveSelectedRight();
	void DropSelectedItem();
	void AssignSelectedToQuickSlot0();
	void AssignSelectedToQuickSlot1();
	void AssignSelectedToQuickSlot2();
	void MoveSelectedItem(int32 DeltaX, int32 DeltaY);
	void AssignSelectedToQuickSlot(int32 QuickSlotIndex);
	void RefreshSelectedItem();

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleItemDropped(FHeistInventoryItem DroppedItem);

	UFUNCTION(Server, Reliable)
	void Server_RequestInteract();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeistInventoryComponent> InventoryComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Debug")
	TSubclassOf<UHeistInventoryDebugWidget> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionRadius = 250.0f;

	UPROPERTY(Transient)
	TObjectPtr<UHeistInventoryDebugWidget> InventoryWidget;

	int32 SelectedInstanceId = INDEX_NONE;
	bool bInventoryOpen = false;
};
