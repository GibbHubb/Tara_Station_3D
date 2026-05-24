#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/WorkMachine.ts. The M8 work-machine fleet —
// non-movement vehicles that unlock or accelerate specific station actions.
// Per CORE_LOOP §1, these are the wet-season build-mode toys: tractor /
// front-end loader / forklift / digger / grader / flat-bed trailer /
// molasses truck. Distinct from FVehicle (M4a movement types).

enum class EWorkMachineType : uint8
{
	Tractor          = 0,
	FrontEndLoader   = 1,
	Forklift         = 2,
	Digger           = 3,
	Grader           = 4,
	FlatBedTrailer   = 5,
	MolassesTruck    = 6,
};

struct FWorkMachineStats
{
	int32 PurchasePrice = 0;
	int32 FuelCostPerUse = 0;
	const TCHAR* Description = TEXT("");
	const TCHAR* Capability = TEXT("");
};

class TARASIMCORE_API FWorkMachine
{
public:
	explicit FWorkMachine(EWorkMachineType InType, bool bInOwned = false);

	static FWorkMachineStats GetStatsFor(EWorkMachineType Type);
	static TArray<EWorkMachineType> AllTypes();

	FString SerializeJson() const;
	static FWorkMachine FromJson(const FString& Json);

public:
	EWorkMachineType Type = EWorkMachineType::Tractor;
	FWorkMachineStats Stats;
	float Condition = 100.0f;
	float Fuel = 100.0f;
	bool bOwned = false;
};
