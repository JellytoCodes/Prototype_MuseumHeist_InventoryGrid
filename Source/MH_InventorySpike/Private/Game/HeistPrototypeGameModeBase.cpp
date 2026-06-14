#include "Game/HeistPrototypeGameModeBase.h"

#include "Character/HeistPrototypeCharacter.h"
#include "Engine/World.h"
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

	if (!bSpawnRandomPickupField)
	{
		return;
	}

	// Respect manually placed pickup actors when a level designer wants a controlled layout.
	for (AHeistPickupActor* ExistingPickup : TActorRange<AHeistPickupActor>(GetWorld()))
	{
		if (ExistingPickup)
		{
			return;
		}
	}

	SpawnRandomPickupField();
}

void AHeistPrototypeGameModeBase::SpawnRandomPickupField()
{
	FVector Origin = FVector::ZeroVector;
	for (APlayerStart* PlayerStart : TActorRange<APlayerStart>(GetWorld()))
	{
		Origin = PlayerStart->GetActorLocation();
		break;
	}

	FRandomStream RandomStream(FMath::Rand());
	TArray<EHeistPickupCase> CasesToSpawn;
	CasesToSpawn.Reserve(RandomPickupCount);

	constexpr int32 PickupCaseCount = 3;
	const int32 GuaranteedPerCase = FMath::Min(MinimumPerPickupCase, RandomPickupCount / PickupCaseCount);
	for (int32 CaseIndex = 0; CaseIndex < PickupCaseCount; ++CaseIndex)
	{
		for (int32 Count = 0; Count < GuaranteedPerCase; ++Count)
		{
			CasesToSpawn.Add(static_cast<EHeistPickupCase>(CaseIndex));
		}
	}

	while (CasesToSpawn.Num() < RandomPickupCount)
	{
		CasesToSpawn.Add(static_cast<EHeistPickupCase>(RandomStream.RandRange(0, PickupCaseCount - 1)));
	}
	for (int32 Index = CasesToSpawn.Num() - 1; Index > 0; --Index)
	{
		CasesToSpawn.Swap(Index, RandomStream.RandRange(0, Index));
	}

	TArray<FVector> UsedLocations;
	UsedLocations.Reserve(CasesToSpawn.Num());
	for (const EHeistPickupCase PickupCase : CasesToSpawn)
	{
		FVector SpawnLocation;
		if (!FindRandomPickupLocation(Origin, RandomStream, UsedLocations, SpawnLocation))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AHeistPickupActor* Pickup = GetWorld()->SpawnActor<AHeistPickupActor>(
			AHeistPickupActor::StaticClass(),
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParameters))
		{
			Pickup->ConfigureCase(PickupCase);
			UsedLocations.Add(Pickup->GetActorLocation());
		}
	}
}

bool AHeistPrototypeGameModeBase::FindRandomPickupLocation(
	const FVector& Origin,
	FRandomStream& RandomStream,
	const TArray<FVector>& UsedLocations,
	FVector& OutLocation) const
{
	const float MinimumSpacingSquared = FMath::Square(MinimumPickupSpacing);
	for (int32 Attempt = 0; Attempt < LocationAttemptsPerPickup; ++Attempt)
	{
		const float Angle = RandomStream.FRandRange(0.0f, 2.0f * UE_PI);
		const float Radius = FMath::Sqrt(RandomStream.FRand()) * PickupFieldRadius;
		const FVector CandidateXY = Origin + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);

		const bool bTooClose = UsedLocations.ContainsByPredicate([&CandidateXY, MinimumSpacingSquared](const FVector& UsedLocation)
		{
			return FVector::DistSquared2D(CandidateXY, UsedLocation) < MinimumSpacingSquared;
		});
		if (bTooClose)
		{
			continue;
		}

		FHitResult GroundHit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InventoryPickupGroundTrace), false);
		const FVector TraceStart(CandidateXY.X, CandidateXY.Y, Origin.Z + 1500.0f);
		const FVector TraceEnd(CandidateXY.X, CandidateXY.Y, Origin.Z - 3000.0f);
		if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			continue;
		}

		OutLocation = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 45.0f);
		return true;
	}

	return false;
}
