#include "Character/HeistPrototypeCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/HeistPickupActor.h"
#include "Inventory/HeistInventoryComponent.h"
#include "UI/HeistInventoryDebugWidget.h"
#include "EngineUtils.h"

AHeistPrototypeCharacter::AHeistPrototypeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 900.0f;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetWorldRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	InventoryComponent = CreateDefaultSubobject<UHeistInventoryComponent>(TEXT("InventoryComponent"));
	InventoryWidgetClass = UHeistInventoryDebugWidget::StaticClass();
}

void AHeistPrototypeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Existing Blueprint children may retain older component defaults, so enforce the fixed world rotation at runtime.
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetWorldRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
	InventoryComponent->OnItemDropped.AddUniqueDynamic(this, &ThisClass::HandleItemDropped);
}

void AHeistPrototypeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &ThisClass::ToggleInventory);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ThisClass::Interact);
	PlayerInputComponent->BindAction(TEXT("InventorySelectPrevious"), IE_Pressed, this, &ThisClass::SelectPreviousItem);
	PlayerInputComponent->BindAction(TEXT("InventorySelectNext"), IE_Pressed, this, &ThisClass::SelectNextItem);
	PlayerInputComponent->BindAction(TEXT("InventoryMoveUp"), IE_Pressed, this, &ThisClass::MoveSelectedUp);
	PlayerInputComponent->BindAction(TEXT("InventoryMoveDown"), IE_Pressed, this, &ThisClass::MoveSelectedDown);
	PlayerInputComponent->BindAction(TEXT("InventoryMoveLeft"), IE_Pressed, this, &ThisClass::MoveSelectedLeft);
	PlayerInputComponent->BindAction(TEXT("InventoryMoveRight"), IE_Pressed, this, &ThisClass::MoveSelectedRight);
	PlayerInputComponent->BindAction(TEXT("InventoryDrop"), IE_Pressed, this, &ThisClass::DropSelectedItem);
	PlayerInputComponent->BindAction(TEXT("InventoryQuickSlot0"), IE_Pressed, this, &ThisClass::AssignSelectedToQuickSlot0);
	PlayerInputComponent->BindAction(TEXT("InventoryQuickSlot1"), IE_Pressed, this, &ThisClass::AssignSelectedToQuickSlot1);
	PlayerInputComponent->BindAction(TEXT("InventoryQuickSlot2"), IE_Pressed, this, &ThisClass::AssignSelectedToQuickSlot2);
}

void AHeistPrototypeCharacter::MoveForward(const float Value)
{
	if (!bInventoryOpen && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void AHeistPrototypeCharacter::MoveRight(const float Value)
{
	if (!bInventoryOpen && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(FVector::RightVector, Value);
	}
}

void AHeistPrototypeCharacter::ToggleInventory()
{
	SetInventoryOpen(!bInventoryOpen);
}

void AHeistPrototypeCharacter::SetInventoryOpen(const bool bOpen)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsLocallyControlled() || !PlayerController)
	{
		return;
	}

	if (bOpen)
	{
		if (!InventoryWidget && InventoryWidgetClass)
		{
			InventoryWidget = CreateWidget<UHeistInventoryDebugWidget>(PlayerController, InventoryWidgetClass);
			if (InventoryWidget)
			{
				InventoryWidget->SetInventoryComponent(InventoryComponent);
				InventoryWidget->SetSelectedInstanceId(SelectedInstanceId);
			}
		}

		if (!InventoryWidget)
		{
			return;
		}

		InventoryWidget->AddToViewport(100);
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
		InventoryWidget->SetKeyboardFocus();
	}
	else
	{
		if (InventoryWidget)
		{
			InventoryWidget->RemoveFromParent();
		}

		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}

	bInventoryOpen = bOpen;
}

void AHeistPrototypeCharacter::Interact()
{
	if (!bInventoryOpen)
	{
		Server_RequestInteract();
	}
}

void AHeistPrototypeCharacter::SelectPreviousItem()
{
	if (!bInventoryOpen)
	{
		return;
	}

	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	if (Items.IsEmpty())
	{
		SelectedInstanceId = INDEX_NONE;
		return;
	}

	int32 SelectedIndex = Items.IndexOfByPredicate([this](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == SelectedInstanceId;
	});
	SelectedIndex = SelectedIndex == INDEX_NONE ? 0 : (SelectedIndex - 1 + Items.Num()) % Items.Num();
	SelectedInstanceId = Items[SelectedIndex].InstanceId;
	RefreshSelectedItem();
}

void AHeistPrototypeCharacter::SelectNextItem()
{
	if (!bInventoryOpen)
	{
		return;
	}

	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	if (Items.IsEmpty())
	{
		SelectedInstanceId = INDEX_NONE;
		return;
	}

	int32 SelectedIndex = Items.IndexOfByPredicate([this](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == SelectedInstanceId;
	});
	SelectedIndex = SelectedIndex == INDEX_NONE ? 0 : (SelectedIndex + 1) % Items.Num();
	SelectedInstanceId = Items[SelectedIndex].InstanceId;
	RefreshSelectedItem();
}

void AHeistPrototypeCharacter::MoveSelectedUp()
{
	MoveSelectedItem(0, -1);
}

void AHeistPrototypeCharacter::MoveSelectedDown()
{
	MoveSelectedItem(0, 1);
}

void AHeistPrototypeCharacter::MoveSelectedLeft()
{
	MoveSelectedItem(-1, 0);
}

void AHeistPrototypeCharacter::MoveSelectedRight()
{
	MoveSelectedItem(1, 0);
}

void AHeistPrototypeCharacter::DropSelectedItem()
{
	if (bInventoryOpen && SelectedInstanceId != INDEX_NONE)
	{
		InventoryComponent->Server_RequestDropItem(SelectedInstanceId);
	}
}

void AHeistPrototypeCharacter::AssignSelectedToQuickSlot0()
{
	AssignSelectedToQuickSlot(0);
}

void AHeistPrototypeCharacter::AssignSelectedToQuickSlot1()
{
	AssignSelectedToQuickSlot(1);
}

void AHeistPrototypeCharacter::AssignSelectedToQuickSlot2()
{
	AssignSelectedToQuickSlot(2);
}

void AHeistPrototypeCharacter::MoveSelectedItem(const int32 DeltaX, const int32 DeltaY)
{
	if (!bInventoryOpen || SelectedInstanceId == INDEX_NONE)
	{
		return;
	}

	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	const FHeistInventoryItem* Item = Items.FindByPredicate([this](const FHeistInventoryItem& Candidate)
	{
		return Candidate.InstanceId == SelectedInstanceId;
	});
	if (!Item)
	{
		return;
	}

	const FIntPoint CurrentCoord = InventoryComponent->IndexToCoord(Item->TopLeftIndex);
	const int32 NewTopLeftIndex = InventoryComponent->CoordToIndex(CurrentCoord.X + DeltaX, CurrentCoord.Y + DeltaY);
	InventoryComponent->Server_RequestMoveItem(Item->InstanceId, NewTopLeftIndex, Item->bRotated);
}

void AHeistPrototypeCharacter::AssignSelectedToQuickSlot(const int32 QuickSlotIndex)
{
	if (bInventoryOpen && SelectedInstanceId != INDEX_NONE)
	{
		InventoryComponent->Server_RequestAssignQuickSlot(QuickSlotIndex, SelectedInstanceId);
	}
}

void AHeistPrototypeCharacter::RefreshSelectedItem()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetSelectedInstanceId(SelectedInstanceId);
	}
}

void AHeistPrototypeCharacter::HandleInventoryChanged()
{
	const TArray<FHeistInventoryItem> Items = InventoryComponent->GetItems();
	const bool bSelectionStillExists = Items.ContainsByPredicate([this](const FHeistInventoryItem& Item)
	{
		return Item.InstanceId == SelectedInstanceId;
	});

	if (!bSelectionStillExists)
	{
		SelectedInstanceId = Items.IsEmpty() ? INDEX_NONE : Items[0].InstanceId;
	}
	RefreshSelectedItem();
}

void AHeistPrototypeCharacter::HandleItemDropped(FHeistInventoryItem DroppedItem)
{
	if (!HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 180.0f + FVector(0.0f, 0.0f, 45.0f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	if (AHeistPickupActor* Pickup = GetWorld()->SpawnActor<AHeistPickupActor>(AHeistPickupActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters))
	{
		Pickup->InitializeFromInventoryItem(DroppedItem);
	}
}

void AHeistPrototypeCharacter::Server_RequestInteract_Implementation()
{
	AHeistPickupActor* ClosestPickup = nullptr;
	float ClosestDistanceSquared = FMath::Square(InteractionRadius);
	for (AHeistPickupActor* Pickup : TActorRange<AHeistPickupActor>(GetWorld()))
	{
		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Pickup->GetActorLocation());
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPickup = Pickup;
		}
	}

	if (ClosestPickup)
	{
		ClosestPickup->TryPickup(InventoryComponent);
	}
}
