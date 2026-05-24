#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Hand.ts. An AI station hand hired for $X/day; brings
// skill that the (3D-spatial) muster reads when it picks how cleanly a mob
// flocks under pressure. The hand's skill is sim-side data; how the 3D layer
// USES it (radio orders during muster, hands visible as NPCs in the world) is
// a presentation concern.
//
// Audit: 🟢 — entity carries forward unchanged. The 2D MusteringSystem's
// computeMuster() is 🔴 (3D replaces it with spatial gameplay), but hand-as-
// hireable-skill-stat ports as-is.

struct FHandConfig
{
	FString Id;
	FString Name;
	int32 Skill = 50;          // 0..100
	int32 WagePerDay = 80;     // $
	bool bHired = false;
};

class TARASIMCORE_API FHand
{
public:
	explicit FHand(const FHandConfig& Config);

	FString SerializeJson() const;
	static FHand FromJson(const FString& Json);

public:
	FString Id;
	FString Name;
	int32 Skill = 50;
	int32 WagePerDay = 80;
	bool bHired = false;
};
