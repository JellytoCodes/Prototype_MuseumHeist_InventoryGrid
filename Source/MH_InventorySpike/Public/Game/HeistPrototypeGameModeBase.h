#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeistPrototypeGameModeBase.generated.h"

UCLASS()
class MH_INVENTORYSPIKE_API AHeistPrototypeGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHeistPrototypeGameModeBase();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnRandomPickupField();
	bool FindRandomPickupLocation(const FVector& Origin, FRandomStream& RandomStream, const TArray<FVector>& UsedLocations, FVector& OutLocation) const;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field")
	bool bSpawnRandomPickupField = true;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field", meta = (ClampMin = "3", UIMin = "3"))
	int32 RandomPickupCount = 30;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field", meta = (ClampMin = "200.0"))
	float PickupFieldRadius = 1400.0f;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field", meta = (ClampMin = "50.0"))
	float MinimumPickupSpacing = 160.0f;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field", meta = (ClampMin = "0"))
	int32 MinimumPerPickupCase = 4;

	UPROPERTY(EditAnywhere, Category = "Inventory Spike|Pickup Field", meta = (ClampMin = "1"))
	int32 LocationAttemptsPerPickup = 16;
};
