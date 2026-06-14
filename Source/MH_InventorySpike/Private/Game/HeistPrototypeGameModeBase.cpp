#include "Game/HeistPrototypeGameModeBase.h"

#include "Character/HeistPrototypeCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/HeistPickupActor.h"

AHeistPrototypeGameModeBase::AHeistPrototypeGameModeBase()
{
	DefaultPawnClass = AHeistPrototypeCharacter::StaticClass();
}

void AHeistPrototypeGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	if (!bSpawnPickupTestCases)
	{
		return;
	}
	for (AHeistPickupActor* ExistingPickup : TActorRange<AHeistPickupActor>(GetWorld()))
	{
		if (ExistingPickup)
		{
			return;
		}
	}

	FVector Origin = FVector::ZeroVector;
	for (APlayerStart* PlayerStart : TActorRange<APlayerStart>(GetWorld()))
	{
		Origin = PlayerStart->GetActorLocation();
		break;
	}

	static const FVector Offsets[] =
	{
		FVector(300.0f, -180.0f, 50.0f),
		FVector(300.0f, 0.0f, 50.0f),
		FVector(300.0f, 180.0f, 50.0f)
	};
	static const EHeistPickupCase Cases[] =
	{
		EHeistPickupCase::Case1_Coin,
		EHeistPickupCase::Case2_GlueTrap,
		EHeistPickupCase::Case3_Painting
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cases); ++Index)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AHeistPickupActor* Pickup = GetWorld()->SpawnActor<AHeistPickupActor>(
			AHeistPickupActor::StaticClass(),
			Origin + Offsets[Index],
			FRotator::ZeroRotator,
			SpawnParameters))
		{
			Pickup->ConfigureCase(Cases[Index]);
		}
	}
}
