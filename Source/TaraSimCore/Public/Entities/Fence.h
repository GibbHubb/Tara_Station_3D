#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Fence.ts. Fence between two adjacent paddocks.
// Integrity decays slowly; at 0, cattle drift between connected paddocks
// during day-tick. Repair via player action.
//
// Audit: 🟢 — entity carries forward unchanged. Phase 2 derives the fence line
// from the polygon edge between the two Landscape paddock regions; the entity
// still owns the integrity stat.

struct FFenceConfig
{
	FString Id;
	FString PaddockA;
	FString PaddockB;
	float Integrity = 75.0f;
};

class TARASIMCORE_API FFence
{
public:
	explicit FFence(const FFenceConfig& Config);

	static float ClampIntegrity(float V);
	bool Connects(const FString& A, const FString& B) const;

	FString SerializeJson() const;
	static FFence FromJson(const FString& Json);

public:
	FString Id;
	FString PaddockA;
	FString PaddockB;
	float Integrity = 75.0f;
};
