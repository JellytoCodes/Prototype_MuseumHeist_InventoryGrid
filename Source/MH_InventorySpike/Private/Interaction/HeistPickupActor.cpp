#include "Interaction/HeistPickupActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Inventory/HeistInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AHeistPickupActor::AHeistPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupMesh->SetCollisionObjectType(ECC_WorldDynamic);
	PickupMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PickupMesh->SetStaticMesh(CubeMesh.Object);
	}

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(180.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
	PickupLabel->SetupAttachment(RootComponent);
	PickupLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
	PickupLabel->SetHorizontalAlignment(EHTA_Center);
	PickupLabel->SetVerticalAlignment(EVRTA_TextCenter);
	PickupLabel->SetWorldSize(28.0f);
	PickupLabel->SetTextRenderColor(FColor(255, 220, 90));
	PickupLabel->SetText(FText::FromString(TEXT("CASE 1\nCoin 1x1\n[F] Pick Up")));
}

void AHeistPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bUseCasePreset)
	{
		ApplyCasePreset();
	}
	UpdateVisuals();
}

void AHeistPickupActor::ConfigureCase(const EHeistPickupCase NewPickupCase)
{
	PickupCase = NewPickupCase;
	bUseCasePreset = true;
	ApplyCasePreset();
	UpdateVisuals();
	ForceNetUpdate();
}

void AHeistPickupActor::InitializeFromInventoryItem(const FHeistInventoryItem& Item)
{
	bUseCasePreset = false;
	ItemId = Item.ItemId;
	ItemTag = Item.ItemTag;
	ItemWidth = Item.Width;
	ItemHeight = Item.Height;
	ItemWeight = Item.Weight;
	ItemScoreValue = Item.ScoreValue;
	UpdateVisuals();
	ForceNetUpdate();
}

bool AHeistPickupActor::TryPickup(UHeistInventoryComponent* InventoryComponent)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return false;
	}

	if (!InventoryComponent->AddItem(ItemId, ItemTag, ItemWidth, ItemHeight, ItemWeight, ItemScoreValue))
	{
		return false;
	}

	Destroy();
	return true;
}

FText AHeistPickupActor::GetInteractionText() const
{
	return FText::FromString(FString::Printf(
		TEXT("Pick up %s (%dx%d)"),
		*ItemId.ToString(),
		ItemWidth,
		ItemHeight));
}

void AHeistPickupActor::ApplyCasePreset()
{
	switch (PickupCase)
	{
	case EHeistPickupCase::Case1_Coin:
		ItemId = TEXT("Loot_Coin");
		ItemWidth = 1;
		ItemHeight = 1;
		ItemWeight = 1;
		ItemScoreValue = 100;
		break;

	case EHeistPickupCase::Case2_GlueTrap:
		ItemId = TEXT("Tool_GlueTrap");
		ItemWidth = 2;
		ItemHeight = 1;
		ItemWeight = 2;
		ItemScoreValue = 0;
		break;

	case EHeistPickupCase::Case3_Painting:
		ItemId = TEXT("Loot_Painting");
		ItemWidth = 1;
		ItemHeight = 3;
		ItemWeight = 3;
		ItemScoreValue = 600;
		break;
	}
}

void AHeistPickupActor::UpdateVisuals()
{
	if (!PickupMesh || !PickupLabel)
	{
		return;
	}

	PickupMesh->SetRelativeScale3D(FVector(
		0.45f + 0.18f * ItemWidth,
		0.45f + 0.18f * ItemHeight,
		0.35f));

	const FString CaseText = bUseCasePreset
		? FString::Printf(TEXT("CASE %d"), static_cast<int32>(PickupCase) + 1)
		: FString(TEXT("DROPPED"));
	PickupLabel->SetText(FText::FromString(FString::Printf(
		TEXT("%s\n%s %dx%d\n[F] Pick Up"),
		*CaseText,
		*ItemId.ToString(),
		ItemWidth,
		ItemHeight)));
}

void AHeistPickupActor::OnRep_ItemData()
{
	UpdateVisuals();
}

void AHeistPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeistPickupActor, PickupCase);
	DOREPLIFETIME(AHeistPickupActor, bUseCasePreset);
	DOREPLIFETIME(AHeistPickupActor, ItemId);
	DOREPLIFETIME(AHeistPickupActor, ItemTag);
	DOREPLIFETIME(AHeistPickupActor, ItemWidth);
	DOREPLIFETIME(AHeistPickupActor, ItemHeight);
	DOREPLIFETIME(AHeistPickupActor, ItemWeight);
	DOREPLIFETIME(AHeistPickupActor, ItemScoreValue);
}
