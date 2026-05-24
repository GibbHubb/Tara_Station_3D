#pragma once

#include "CoreMinimal.h"
#include "EventBus.h"
#include "SeasonClock.h"
#include "Entities/Paddock.h"
#include "Entities/Herd.h"
#include "Entities/Bore.h"
#include "Entities/Player.h"
#include "Entities/PriceTable.h"
#include "Entities/Hand.h"
#include "Entities/Vehicle.h"
#include "Entities/Fence.h"
#include "Entities/Road.h"
#include "Systems/ConditionSystem.h"
#include "Systems/WaterSystem.h"
#include "Systems/EconomySystem.h"
#include "Systems/MusteringSystem.h"
#include "Systems/EventSystem.h"
#include "Systems/BreedingSystem.h"

// Save-schema version. Bumps on any breaking sim shape change. Loading a save
// with a mismatched version is treated as "no save" (start fresh).
#define TARA_SIM_SAVE_SCHEMA_VERSION TEXT("tara-save-3d-v5-m5")

// Adjacency between paddocks — paddock id → list of adjacent paddock ids.
// Phase 0 carries this as explicit state (mirroring the 2D model). Phase 2
// in 3D will derive it from polygon edges sharing a fence-line, per the
// sim-port audit. Until then, declared adjacency stays the source of truth.
using FAdjacency = TMap<FString, TArray<FString>>;

// FStation — the sim orchestrator. Mirrors src/sim/Station.ts.
//
// Owns all entities + systems + the event bus. Runs the tickDay sequence:
// clock → herd-grass → water → economy → condition (current scope through M3).
// Future milestones extend: + infrastructure + event + breeding + wildlife
// + progression + sensor.
//
// The 3D layer's UTaraSimSubsystem (in TaraGame) owns one FStation via
// TUniquePtr. Actors / Pawns / Widgets reach the station through the
// subsystem; the station never knows about them.

class TARASIMCORE_API FStation
{
public:
	// Default construction sets up the M1 starter state: 3 paddocks + 1
	// 8-head cohort in Home Paddock + ringer player at $250 + default prices.
	// Equivalent to BootScene "New Game" in the 2D project.
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

	// M3 accessors.
	FPlayer& GetPlayer() { return Player; }
	const FPlayer& GetPlayer() const { return Player; }
	FPriceTable& GetPrices() { return Prices; }
	const FPriceTable& GetPrices() const { return Prices; }
	FEconomySystem& GetEconomySystem() { return *EconomySys; }

	// M4a accessors.
	TArray<FHand>& GetHands() { return Hands; }
	const TArray<FHand>& GetHands() const { return Hands; }
	TArray<FVehicle>& GetVehicles() { return Vehicles; }
	const TArray<FVehicle>& GetVehicles() const { return Vehicles; }
	TArray<FFence>& GetFences() { return Fences; }
	const TArray<FFence>& GetFences() const { return Fences; }
	TArray<FRoad>& GetRoads() { return Roads; }
	const TArray<FRoad>& GetRoads() const { return Roads; }
	FMusteringSystem& GetMusteringSystem() { return *MusterSys; }

	// M5 accessors.
	FEventSystem& GetEventSystem() { return *EventSys; }
	const FEventSystem& GetEventSystem() const { return *EventSys; }
	FBreedingSystem& GetBreedingSystem() { return *BreedingSys; }
	const FBreedingSystem& GetBreedingSystem() const { return *BreedingSys; }

	const FPaddock* PaddockById(const FString& Id) const;
	FPaddock* PaddockById(const FString& Id);
	const FVehicle* VehicleByType(EVehicleType Type) const;
	FVehicle* VehicleByType(EVehicleType Type);
	const FFence* FenceBetween(const FString& A, const FString& B) const;
	const FRoad* RoadBetween(const FString& A, const FString& B) const;

	// M2 — water access for a paddock, used by ConditionSystem + HUD.
	EWaterAccess GetWaterAccess(const FString& PaddockId) const;

	// Action surface for the bridge / Blueprints.
	struct FCheckBoreResult { bool bFixed = false; int32 DaysSpent = 0; };
	FCheckBoreResult CheckBore(const FString& BoreId);

	// M4a action surface — the 3D layer commits to a planned muster (it has
	// already chosen vehicle, hands, and computed days/cost spatially). The
	// sim accepts it, debits fuel cost, books the breakaway, and runs the
	// state-machine clock until completion. Returns false if invalid.
	bool StartMuster(
		const FString& ToPaddockId,
		EVehicleType VehicleType,
		const TArray<FString>& HandIds,
		int32 PlannedDays,
		int32 FuelCost,
		int32 BreakawayPending);

	// Purchase a vehicle — debits cash, marks owned. Returns false on
	// insufficient funds or already-owned.
	bool BuyVehicle(EVehicleType Type);

	// Hire / fire an AI station hand.
	bool HireHand(const FString& HandId);
	bool FireHand(const FString& HandId);

	// Repair a fence to full integrity. Returns false if unknown fence.
	bool RepairFence(const FString& FenceId);

	// M5 action surface — player chooses how to handle a drought decision.
	// Choice: "feed" / "agist" / "sell" / "hold".
	void ResolveBadWeatherDecision(const FString& Choice);

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
	FPlayer Player;
	FPriceTable Prices;

	// M4a state.
	TArray<FHand> Hands;
	TArray<FVehicle> Vehicles;
	TArray<FFence> Fences;
	TArray<FRoad> Roads;

	// Systems — initialised AFTER the data members above (constructor order
	// matters; systems hold references to Station + Bus). Water + Economy
	// come BEFORE Condition so Condition can read water access + active
	// supplements on its tick. Muster comes AFTER Economy so cancel-refund
	// can hit cash.
	TUniquePtr<FWaterSystem> WaterSys;
	TUniquePtr<FEconomySystem> EconomySys;
	TUniquePtr<FMusteringSystem> MusterSys;
	TUniquePtr<FEventSystem> EventSys;
	TUniquePtr<FBreedingSystem> BreedingSys;
	TUniquePtr<FConditionSystem> ConditionSys;

	// Default-seed helper — fills paddocks + herd + bores + adjacency to the
	// "New Game" state mirroring 2D BootScene.
	void SeedDefaults();

	// M4a — daily fence/road decay + broken-fence drift effects. Called from
	// TickDay between Economy and Condition.
	void TickInfrastructure();
};
