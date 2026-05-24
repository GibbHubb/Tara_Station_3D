#pragma once

#include "CoreMinimal.h"

class FStation;
class FEventBus;

// MusteringSystem — **stub form** for M4a.
//
// The 2D project's `MusteringSystem` (src/sim/systems/MusteringSystem.ts) owned
// the canonical outcome math: `computeMuster(plan)` returned days / fuel cost /
// expected breakaway / skill contribution from a static plan. The sim-port
// audit marked that calc 🔴 — in 3D, mustering is a *spatial* activity that
// the world decides. There's no static days/breakaway number; the player
// drives the herd themselves, and how they perform is read by the 3D AI.
//
// What this stub keeps from the 2D system:
//   • OnMusterCompleted — small mustering-skill bump on the Player. The player
//     learns by doing, regardless of how the muster came about (2D plan or 3D
//     spatial gameplay).
//   • CancelMuster — clears the muster, refunds half the booked fuel via
//     EconomySystem, emits `MusterCancelled`. The 3D layer calls this if the
//     player abandons mid-transit (drives home without finishing).
//   • Day-tick advances the muster state machine on the Herd. On completion,
//     fires `MusterCompleted` (and `Breakaway` if any breakaway head landed).
//
// The "compute outcome" calc — days, expected breakaway, skill/handling/road
// math — is owned by the 3D layer (presentation). The 3D bridge fills in the
// FMusterState fields when calling StartMuster, and the sim runs the clock on
// what was committed.

class TARASIMCORE_API FMusteringSystem
{
public:
	FMusteringSystem(FStation& InStation, FEventBus& InBus);

	// Day-tick — advances the muster state machine, fires completion events.
	void TickDay();

	// Cancel an in-flight muster. Returns whether anything was cancelled +
	// the refund applied. The 3D layer calls this when the player drives back
	// to the homestead without finishing the muster.
	struct FCancelResult
	{
		bool bWasInTransit = false;
		int32 DaysSpent = 0;
		int32 FuelRefund = 0;
	};
	FCancelResult CancelMuster();

	// Called by the 3D layer when a muster completes spatially without going
	// through the day-tick state machine (e.g. instant completion in a tight
	// paddock). Drives the same skill-bump + event side-effects.
	void OnMusterCompleted();

private:
	FStation& Station;
	FEventBus& Bus;

	// Per-day skill gain when a muster completes. Mirrors 2D constant.
	static constexpr float MusteringSkillBumpPerCompletion = 0.4f;

	// Fuel-refund fraction on cancel. Mirrors 2D's 0.5.
	static constexpr float FuelRefundFractionOnCancel = 0.5f;
};
