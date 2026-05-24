#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/InvasivePresence.ts. Invasive plant presence per
// paddock. Spreads if unchecked; lowers effective grass quality (we
// approximate by reducing the paddock's effective intake area).

enum class EInvasiveSpecies : uint8
{
	Parkinsonia   = 0,
	RubberVine    = 1,
	PricklyAcacia = 2,
};

struct FInvasiveEntry
{
	FString PaddockId;
	EInvasiveSpecies Species = EInvasiveSpecies::Parkinsonia;
	float Coverage = 0.0f;  // 0..100 %

	FString SerializeJson() const;
	static FInvasiveEntry FromJson(const FString& Json);
};

TARASIMCORE_API FString InvasiveSpeciesToString(EInvasiveSpecies Species);
TARASIMCORE_API bool TryParseInvasiveSpecies(const FString& Name, EInvasiveSpecies& OutSpecies);
