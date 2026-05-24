#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Supplement.ts. Active supplemental-feed effect on
// the herd. Lifts the effective grass-floor for the herd (cattle behave like
// they're on better country) for N days. Costs money + has delivery delay.

enum class ESupplementKind : uint8
{
	Lick,
	Molasses,
	Feed,
};

struct FSupplementEffect
{
	ESupplementKind Kind;
	int32 DaysRemaining = 0;       // ticks down after delivery
	int32 DaysUntilArrival = 0;    // 0 = delivered, ticking each day until then
	float GrassFloorLift = 0.0f;   // kg/ha added to effective floor while active
	float ConditionRecoveryBoost = 0.0f;  // extra per-day recovery while active
	int32 TotalDays = 0;
};

// Per-kind profiles (mirrors SUPPLEMENT_PROFILES from the 2D version).
struct TARASIMCORE_API FSupplementProfile
{
	static float GrassFloorLift(ESupplementKind Kind);
	static float ConditionRecoveryBoost(ESupplementKind Kind);
	static int32 DefaultDurationDays(ESupplementKind Kind);
	static int32 DefaultDeliveryDays(ESupplementKind Kind);
};
