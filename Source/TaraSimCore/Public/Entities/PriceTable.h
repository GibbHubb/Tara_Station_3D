#pragma once

#include "CoreMinimal.h"
#include "../SeasonClock.h"

// Mirrors src/sim/entities/PriceTable.ts.
//
// Market price model — seasonal modifier shipped now; drought modifier (set by
// M5 EventSystem at Phase 5+) starts at 0 so the data path exists and lights
// up when the events port lands. Same approach as the 2D project.

class TARASIMCORE_API FPriceTable
{
public:
	FPriceTable() = default;

	// 2D polish-pass defaults — port byte-for-byte.
	int32 BasePerHead = 900;
	int32 BasePerLickDay = 8;
	int32 BasePerMolassesDay = 16;
	int32 BasePerFeedDay = 35;
	int32 BasePerFuelMuster = 25;

	float DroughtSeverity = 0.0f;   // 0..1; lit by M5 EventSystem at Phase 5+.

	// Dry = 0.85x baseline; Wet = 1.0x — prices breathe with the season even
	// without M5 drought driving them.
	float SeasonalModifier(ESeason Season) const;

	// Drought modifier — starts at 1.0, falls toward ~0.45x at peak drought.
	float DroughtModifier() const;

	int32 CurrentCattlePrice(ESeason Season) const;

	FString SerializeJson() const;
	static FPriceTable FromJson(const FString& Json);
};
