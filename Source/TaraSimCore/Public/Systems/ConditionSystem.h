#pragma once

#include "CoreMinimal.h"
#include "../SeasonClock.h"

class FStation;
class FEventBus;
class FHerd;

// Mirrors src/sim/systems/ConditionSystem.ts — the M1 system that gives the
// grass loop teeth. Asymmetric drift function: recovery slower than decline.
// Ports byte-for-byte from the 2D version's polish-pass numbers.

class TARASIMCORE_API FConditionSystem
{
public:
	// Tunable constants — port the 2D values. Tune per playtest-learnings.md.
	static constexpr float GrassThreshold = 1500.0f;    // ≥ → condition recovers
	static constexpr float GrassFloor = 600.0f;         // [floor, threshold) no change
	static constexpr float GrassCritical = 200.0f;      // [critical, floor) decline; below → starvation
	static constexpr float RecoveryPerDay = 0.5f;
	static constexpr float DeclinePerDay = 0.8f;
	static constexpr float StarvationPerDay = 2.0f;
	static constexpr float TransitDrainPerDay = 0.3f;

	// M2 — water access decay layer (applied AFTER grass-driven delta).
	static constexpr float WaterLowExtraDecay = 0.4f;
	static constexpr float WaterNoneExtraDecay = 1.5f;

	static constexpr float StarvationCondition = 20.0f;
	static constexpr int32 StarvationGraceDays = 3;
	static constexpr float DeathFractionPerDay = 0.05f;

	FConditionSystem(FEventBus& InBus, FStation& InStation);

	void TickDay();

private:
	FEventBus& Bus;
	FStation& Station;

	void ApplyDelta(FHerd& Herd, float Delta, const FString& HerdId, const FString& LocationName);
};
