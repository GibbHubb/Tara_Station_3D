#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeanerCornerVolume.generated.h"

class UBoxComponent;
class UTaraSimSubsystem;

// TS-3D-WEANER-SCHOOL plan §6. A volume placed inside a paddock; while a
// Weaner-state cohort's herd sits in this paddock at day-tick time, the
// cohort's MusterTrainedness rises by `TrainednessRiseDailyAmount` (default
// 1.0, capped at 100).
//
// Stateless — listens for OnDayEnded each tick and inspects current sim state.
// Place one instance per active schooling paddock.

UCLASS(Blueprintable)
class TARAGAME_API AWeanerCornerVolume : public AActor
{
	GENERATED_BODY()

public:
	AWeanerCornerVolume();

	// The paddock id this volume is associated with. Must match a paddock id
	// from Station::SeedDefaults (e.g. "Holding"). Schooling only credits if
	// the herd's currentPaddockId equals this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Weaner")
	FString PaddockId;

	// Per-day Trainedness rise. Plan default 1.0/day → ~100 days to cap.
	// Tune per playtest (CORE_LOOP §7 noted both pace + this kind of rate are
	// TUNABLE).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Weaner")
	float TrainednessRiseDailyAmount = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> Volume = nullptr;

private:
	int32 SubIdDayEnded = -1;

	void HandleDayEnded();
	UTaraSimSubsystem* GetSubsystem() const;
};
