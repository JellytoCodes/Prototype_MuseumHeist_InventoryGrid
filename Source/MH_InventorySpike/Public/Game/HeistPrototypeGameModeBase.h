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
	UPROPERTY(EditAnywhere, Category = "Inventory Spike")
	bool bSpawnPickupTestCases = true;
};
