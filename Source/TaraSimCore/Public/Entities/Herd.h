#pragma once

#include "CoreMinimal.h"
#include "CattleCohort.h"

// Mirrors src/sim/Herd.ts. In Phase 0 we ship the cohorts + currentPaddockId
// fields. The 3D-shaped change (per sim-port-audit.md): `CurrentPaddockId` is
// a hint, not authoritative — in a continuous 3D world, which paddock the
// herd is in is *derived* from cattle Actor positions. Phase 2 reconciles
// this; Phase 0 just keeps the field so M1 sim math has somewhere to read.
//
// M4a (2026-05-24) adds a minimal **muster state machine** — the same
// idle/transit pattern from the 2D Herd.ts. The 2D version's `computeMuster`
// outcome math is 🔴 (replaced by 3D spatial gameplay), but tracking *that a
// muster is in flight* still matters: ConditionSystem/HUD/save-game all need
// to know whether the mob is in transit and where it's heading. The 3D layer
// decides spatially HOW the muster goes; the sim records WHAT happened.

enum class EMusterStatus : uint8
{
	Idle    = 0,
	Transit = 1,
};

// Active muster — populated only when Status == Transit. The 3D layer fills in
// the transit fields when it begins a muster (player commits to a destination
// paddock with a chosen vehicle + hands); the sim ticks DaysRemaining down to
// zero and then flips back to Idle, applying any BreakawayPending.
struct FMusterState
{
	EMusterStatus Status = EMusterStatus::Idle;
	FString ToPaddockId;            // destination
	int32 DaysRemaining = 0;
	int32 TotalDays = 0;
	int32 VehicleType = 0;          // EVehicleType as int (avoids cross-include)
	TArray<FString> HandIds;        // hands committed to this muster
	int32 FuelCost = 0;             // booked at StartMuster time
	int32 BreakawayPending = 0;     // applied on completion
};

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

	// M4a — muster state.
	bool IsInTransit() const { return Muster.Status == EMusterStatus::Transit; }
	const FMusterState& GetMuster() const { return Muster; }
	FMusterState& GetMusterMutable() { return Muster; }

	// Begin a muster. Mirrors src/sim/Herd.ts startMuster — refuses if already
	// in transit, destination matches current paddock, days <= 0, or herd is
	// empty. Returns true if accepted.
	bool StartMuster(
		const FString& ToPaddockId,
		int32 Days,
		int32 VehicleType,
		const TArray<FString>& HandIds,
		int32 FuelCost,
		int32 BreakawayPending);

	// Cancel an active muster — clears Muster, returns the days elapsed. Caller
	// (MusteringSystem) handles fuel refund + event emission.
	struct FCancelResult { bool bWasInTransit = false; int32 DaysElapsed = 0; };
	FCancelResult CancelMuster();

	// Tick one day of an active muster. When DaysRemaining hits 0, the herd
	// teleports to ToPaddockId (sim-side) and returns true so the caller can
	// apply BreakawayPending + emit MusterCompleted. The 3D layer is
	// responsible for the visible cattle motion across that window.
	struct FMusterTickResult
	{
		bool bCompletedThisTick = false;
		FString CompletedToPaddockId;
		int32 BreakawayApplied = 0;
	};
	FMusterTickResult TickMusterDay();

	FString SerializeJson() const;
	static FHerd FromJson(const FString& Json);

public:
	TArray<FCattleCohort> Cohorts;
	FString CurrentPaddockId;
	// Notables (named cattle — bulls, prize cows) come in Phase 5+ per the audit.

private:
	FMusterState Muster;
};
