#pragma once

#include "CoreMinimal.h"
#include "CattleCohort.h"

// Mirrors src/sim/Herd.ts. In Phase 0 we ship the cohorts + currentPaddockId
// fields. The 3D-shaped change (per sim-port-audit.md): `CurrentPaddockId` is
// a hint, not authoritative — in a continuous 3D world, which paddock the
// herd is in is *derived* from cattle Actor positions. Phase 2 reconciles
// this; Phase 0 just keeps the field so M1 sim math has somewhere to read.

class TARASIMCORE_API FHerd
{
public:
	FHerd() = default;
	explicit FHerd(const FCattleCohort& StartingCohort, const FString& StartPaddockId);

	int32 GetSize() const;
	float GetConditionMean() const;
	int32 HerdInPaddock(const FString& PaddockId) const;

	// Returns the count actually removed (capped at the existing total).
	int32 ApplyBreakaway(int32 RequestedCount);

	FString SerializeJson() const;
	static FHerd FromJson(const FString& Json);

public:
	TArray<FCattleCohort> Cohorts;
	FString CurrentPaddockId;
	// Notables (named cattle — bulls, prize cows) come in Phase 5+ per the audit.
	// Muster state is intentionally not modelled here in Phase 0; mustering in
	// 3D is a spatial activity, not a transit-state field.
};
