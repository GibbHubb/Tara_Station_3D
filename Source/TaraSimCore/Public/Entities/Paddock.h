#pragma once

#include "CoreMinimal.h"
#include "../SeasonClock.h"

// Mirrors src/sim/Paddock.ts from the 2D project.
//
// In 2D: an abstract slot with an areaHa + grassKgPerHa.
// In 3D: same sim shape; spatial bounds (a polygon on the Landscape) are owned
// by APaddockActor in TaraGame, which holds a pointer to this FPaddock.
//
// The grass-simulation math (intake cap + growth × season × droughtMultiplier)
// is identical to the 2D version's polish-pass calibration.

struct FPaddockConfig
{
	FString Id;
	FString Name;
	float AreaHa = 180.0f;
	float StartingGrassKgPerHa = 2500.0f;
};

class TARASIMCORE_API FPaddock
{
public:
	static constexpr float MaxGrassKgPerHa = 3500.0f;

	// Tuning constants — Phase 0 mirror of 2D polish-pass calibration. Carry
	// adjustments from playtest-learnings.md here.
	static constexpr float DailyIntakeKgPerHead = 500.0f;
	static constexpr float MaxIntakeKgPerHa = 50.0f;
	static constexpr float WetGrowthKgPerHa = 28.0f;
	static constexpr float DryGrowthKgPerHa = 6.0f;

	explicit FPaddock(const FPaddockConfig& Config);

	// Daily grass simulation. `GrowthMultiplier` is provided by EventSystem
	// (drought halves growth at peak); defaults to 1.0 when no event is active.
	void SimulateDay(ESeason Season, int32 HerdInPaddock, float GrowthMultiplier = 1.0f);

	float GrassRatio() const;

	FString SerializeJson() const;
	static FPaddock FromJson(const FString& Json);

public:
	FString Id;
	FString Name;
	float AreaHa = 180.0f;
	float GrassKgPerHa = 2500.0f;
};
