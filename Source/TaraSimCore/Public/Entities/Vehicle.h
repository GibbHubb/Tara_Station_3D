#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Vehicle.ts — the 6 movement types from M4a. Work
// machines (tractor, grader, etc.) arrive in M8 as a separate entity.
//
// Per CORE_LOOP §7: quad + motorcycle are everyday workhorses; horses are
// also used; the chopper is the manager/owner-tier expensive option (75k
// purchase + 180/muster fuel); foot is the floor.

enum class EVehicleType : uint8
{
	Foot       = 0,
	Horse      = 1,
	TwoWheeler = 2,
	FourWheeler = 3,
	Buggy      = 4,
	Chopper    = 5,
};

struct FVehicleStats
{
	float SpeedFactor = 1.0f;       // multiplied into base muster days (lower = faster)
	float TerrainPenalty = 0.0f;    // added to days if road quality is poor (wheeled only)
	int32 FuelCostPerMuster = 0;    // $ deducted on each muster
	int32 CattleHandling = 50;      // 0..100 — affects breakaway risk (higher = better)
	int32 PurchasePrice = 0;        // $ to acquire (0 = always available — foot, horse)
};

class TARASIMCORE_API FVehicle
{
public:
	explicit FVehicle(EVehicleType InType, bool bInOwned = false);

	// Static profile table mirroring the 2D PROFILES const.
	static FVehicleStats GetStatsFor(EVehicleType Type);

	// True if the type is a wheeled vehicle (2-wheeler, 4-wheeler, buggy) —
	// these suffer the road-grade terrain penalty.
	static bool IsWheeled(EVehicleType Type);

	// All 6 vehicle types, in display order.
	static TArray<EVehicleType> AllTypes();

	FString SerializeJson() const;
	static FVehicle FromJson(const FString& Json);

public:
	EVehicleType Type = EVehicleType::Foot;
	FVehicleStats Stats;
	float Condition = 100.0f;   // 0..100; degrades with use; affects breakdown risk
	float Fuel = 100.0f;        // 0..100; refilled at homestead
	bool bOwned = false;
};
