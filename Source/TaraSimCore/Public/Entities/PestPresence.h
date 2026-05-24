#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/PestPresence.ts. Per-pest population number that
// grows over time unless controlled (M6 shoot interaction). When the herd is
// in the same paddock, calves are at risk per day.

enum class EPestSpecies : uint8
{
	Dingo    = 0,
	FeralCat = 1,
	WildPig  = 2,
};

struct FPestEntry
{
	FString PaddockId;
	EPestSpecies Species = EPestSpecies::Dingo;
	float Population = 0.0f;

	FString SerializeJson() const;
	static FPestEntry FromJson(const FString& Json);
};

struct FPestImpact
{
	float CalfKillProbPerDay = 0.0f;
	const TCHAR* Description = TEXT("");
};

TARASIMCORE_API FPestImpact GetPestImpact(EPestSpecies Species);
TARASIMCORE_API FString PestSpeciesToString(EPestSpecies Species);
TARASIMCORE_API bool TryParsePestSpecies(const FString& Name, EPestSpecies& OutSpecies);
