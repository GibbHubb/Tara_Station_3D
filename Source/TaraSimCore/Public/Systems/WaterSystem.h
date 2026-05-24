#pragma once

#include "CoreMinimal.h"
#include "../Entities/Bore.h"

class FStation;
class FEventBus;

enum class EWaterAccess : uint8
{
	Full,
	Low,
	None,
};

// Mirrors src/sim/systems/WaterSystem.ts. Daily bore-condition decay + failure
// rolls + waterAccess derivation per paddock from the union of serving bores.
// Emits BoreFailed / BoreRepaired / WaterAccessLost / WaterAccessRestored via
// FEventBus so ConditionSystem (and the bridge) react automatically.

class TARASIMCORE_API FWaterSystem
{
public:
	static constexpr float DailyConditionDecay = 0.3f;
	static constexpr float FailureThreshold = 50.0f;
	static constexpr float FailureDailyProbAtZero = 0.20f;
	static constexpr float LowCondition = 30.0f;
	static constexpr int32 CheckBoreDayCost = 1;
	static constexpr float CheckBoreBaseSuccess = 0.80f;

	FWaterSystem(FEventBus& InBus, FStation& InStation);

	void TickDay();

	// Manually recompute access (after entities are loaded / on demand).
	void RecomputeAccess();

	EWaterAccess GetAccess(const FString& PaddockId) const;

	struct FCheckBoreResult { bool bFixed = false; int32 DaysSpent = 0; };
	FCheckBoreResult CheckBore(const FString& BoreId, int32 CurrentDayOfYear);

private:
	FEventBus& Bus;
	FStation& Station;
	TMap<FString, EWaterAccess> AccessByPaddock;
};
