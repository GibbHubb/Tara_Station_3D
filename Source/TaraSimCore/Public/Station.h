#pragma once

#include "CoreMinimal.h"
#include "EventBus.h"
#include "SeasonClock.h"
#include "Entities/Paddock.h"
#include "Entities/Herd.h"
#include "Systems/ConditionSystem.h"

// Save-schema version. Bumps on any breaking sim shape change. Loading a save
// with a mismatched version is treated as "no save" (start fresh).
#define TARA_SIM_SAVE_SCHEMA_VERSION TEXT("tara-save-3d-v1-phase0")

// FStation — the sim orchestrator. Mirrors src/sim/Station.ts.
//
// Owns all entities + systems + the event bus. Runs the tickDay sequence:
// (clock → herd → grass) — for Phase 0 only condition wires up. Later phases
// add Water → Economy → Mustering → Infrastructure → Event → Breeding →
// Wildlife → Progression → Sensor.
//
// The 3D layer's UTaraSimSubsystem (in TaraGame) owns one FStation via
// TUniquePtr. Actors / Pawns / Widgets reach the station through the
// subsystem; the station never knows about them.

class TARASIMCORE_API FStation
{
public:
	// Default construction sets up the M1 starter state: 3 paddocks + 1
	// 8-head cohort in Home Paddock. Equivalent to BootScene "New Game"
	// in the 2D project.
	FStation();

	// Day-tick — the heart of the sim.
	void TickDay();

	// Read accessors used by the bridge subsystem + the systems.
	FEventBus& GetBus() { return Bus; }
	FSeasonClock& GetClock() { return Clock; }
	const FSeasonClock& GetClock() const { return Clock; }
	FHerd& GetHerd() { return Herd; }
	const FHerd& GetHerd() const { return Herd; }
	TArray<FPaddock>& GetPaddocks() { return Paddocks; }
	const TArray<FPaddock>& GetPaddocks() const { return Paddocks; }

	const FPaddock* PaddockById(const FString& Id) const;
	FPaddock* PaddockById(const FString& Id);

	// Serialisation — JSON snapshot, schema-versioned. See SaveManager pattern
	// in the 2D project's src/sim/SaveManager.ts.
	FString SerializeJson() const;
	static TUniquePtr<FStation> FromJson(const FString& Json);

private:
	FSeasonClock Clock;
	FEventBus Bus;

	TArray<FPaddock> Paddocks;
	FHerd Herd;

	// Systems — initialised AFTER the data members above (constructor order
	// matters; systems hold references to Station + Bus).
	TUniquePtr<FConditionSystem> ConditionSys;
};
