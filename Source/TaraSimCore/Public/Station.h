#pragma once

#include "CoreMinimal.h"
#include "EventBus.h"
#include "SeasonClock.h"
#include "Entities/Paddock.h"
#include "Entities/Herd.h"
#include "Entities/Bore.h"
#include "Systems/ConditionSystem.h"
#include "Systems/WaterSystem.h"

// Save-schema version. Bumps on any breaking sim shape change. Loading a save
// with a mismatched version is treated as "no save" (start fresh).
#define TARA_SIM_SAVE_SCHEMA_VERSION TEXT("tara-save-3d-v2-m2")

// Adjacency between paddocks — paddock id → list of adjacent paddock ids.
// Phase 0 carries this as explicit state (mirroring the 2D model). Phase 2
// in 3D will derive it from polygon edges sharing a fence-line, per the
// sim-port audit. Until then, declared adjacency stays the source of truth.
using FAdjacency = TMap<FString, TArray<FString>>;

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
	TArray<FBore>& GetBores() { return Bores; }
	const TArray<FBore>& GetBores() const { return Bores; }
	const FAdjacency& GetAdjacency() const { return Adjacency; }
	FWaterSystem& GetWaterSystem() { return *WaterSys; }

	const FPaddock* PaddockById(const FString& Id) const;
	FPaddock* PaddockById(const FString& Id);

	// M2 — water access for a paddock, used by ConditionSystem + HUD.
	EWaterAccess GetWaterAccess(const FString& PaddockId) const;

	// Action surface for the bridge / Blueprints.
	struct FCheckBoreResult { bool bFixed = false; int32 DaysSpent = 0; };
	FCheckBoreResult CheckBore(const FString& BoreId);

	// Serialisation — JSON snapshot, schema-versioned. See SaveManager pattern
	// in the 2D project's src/sim/SaveManager.ts.
	FString SerializeJson() const;
	static TUniquePtr<FStation> FromJson(const FString& Json);

private:
	FSeasonClock Clock;
	FEventBus Bus;

	TArray<FPaddock> Paddocks;
	FHerd Herd;
	TArray<FBore> Bores;
	FAdjacency Adjacency;

	// Systems — initialised AFTER the data members above (constructor order
	// matters; systems hold references to Station + Bus). Water comes BEFORE
	// Condition so Condition can query water-access on its tick.
	TUniquePtr<FWaterSystem> WaterSys;
	TUniquePtr<FConditionSystem> ConditionSys;

	// Default-seed helper — fills paddocks + herd + bores + adjacency to the
	// "New Game" state mirroring 2D BootScene.
	void SeedDefaults();
};
