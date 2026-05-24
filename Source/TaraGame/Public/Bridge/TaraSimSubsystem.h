#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Station.h"           // from TaraSimCore — pure C++; no UCLASS
#include "TaraSimSubsystem.generated.h"

// UE delegates that mirror selected FEventBus events. Bridge subscribes to the
// sim's pure C++ bus once, then re-broadcasts here so Blueprints / Widgets /
// Actors can bind without ever touching FEventBus directly.

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraDayEnded, const FDayEndedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraCattleDied, const FCattleDiedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraHerdConditionChanged, const FHerdConditionChangedPayload&);

// M2 — water events surfaced through the bridge for the HUD + paddock actors.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraWaterAccessLost, const FWaterAccessLostPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraWaterAccessRestored, const FWaterAccessRestoredPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBoreFailed, const FBoreFailedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBoreRepaired, const FBoreRepairedPayload&);

// M3 — economy events surfaced for HUD + year-end widget.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraMoneyChanged, const FMoneyChangedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraCattleSold, const FCattleSoldPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraSupplementOrdered, const FSupplementOrderedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraYearEnded, const FYearEndedPayload&);

// M4a — muster + infrastructure events surfaced for HUD + paddock/fence actors.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraMusterStarted, const FMusterStartedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraMusterCompleted, const FMusterCompletedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraMusterCancelled, const FMusterCancelledPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBreakaway, const FBreakawayPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraFenceBroken, const FFenceBrokenPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraCattleDrifted, const FCattleDriftedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraVehiclePurchased, const FVehiclePurchasedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraHandHired, const FHandHiredPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraHandFired, const FHandFiredPayload&);

/**
 * UTaraSimSubsystem — the bridge between the engine-agnostic FStation (pure C++
 * sim from TaraSimCore) and the UE5 runtime. There is ONE Station per game
 * instance, owned here as a TUniquePtr.
 *
 * Actors, Pawns, Widgets all read sim state through this subsystem and bind
 * to its UE delegates for reactions. The sim itself never knows the bridge
 * exists.
 *
 * Save/load lives here: on every DayEnded the subsystem writes Station.SerializeJson()
 * to FPaths::ProjectSavedDir() / "tara-save-3d-vN.json"; on game start it tries
 * to read it back.
 */
UCLASS()
class TARAGAME_API UTaraSimSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem interface.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface.

	// Player-facing actions exposed to Blueprints. Currently Phase 0 = TickDay.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	void TickDay();

	UFUNCTION(BlueprintCallable, Category = "Tara")
	void SaveNow();

	UFUNCTION(BlueprintCallable, Category = "Tara")
	void NewGame();

	// M2 — bore action surface.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool CheckBore(const FString& BoreId);

	// M3 — economy actions.
	// SupplementKind: 0 = Lick, 1 = Molasses, 2 = Feed.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool BuySupplement(int32 SupplementKind);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	int32 SellCattle(int32 Count);

	// M4a — muster + infrastructure actions.
	// VehicleType: 0=Foot, 1=Horse, 2=TwoWheeler, 3=FourWheeler, 4=Buggy, 5=Chopper.
	// PlannedDays / FuelCost / BreakawayPending are decided by the 3D layer
	// (spatial gameplay computes them); the sim runs the clock + applies them
	// at completion.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool StartMuster(
		const FString& ToPaddockId,
		int32 VehicleType,
		const TArray<FString>& HandIds,
		int32 PlannedDays,
		int32 FuelCost,
		int32 BreakawayPending);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool CancelMuster();

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool BuyVehicle(int32 VehicleType);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool HireHand(const FString& HandId);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool FireHand(const FString& HandId);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool RepairFence(const FString& FenceId);

	// Read-only views for Blueprints / Widgets. These mirror common HUD reads
	// so the UMG layer doesn't need direct access to the C++ Station.
	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetYear() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetDayOfYear() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	FString GetFormattedTime() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetHerdSize() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetHerdConditionMean() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetPaddockGrassKgPerHa(const FString& PaddockId) const;

	// M2 — water access readout as int (0=Full / 1=Low / 2=None) for Blueprint.
	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetPaddockWaterAccess(const FString& PaddockId) const;

	// M3 — economy readouts.
	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetPlayerCash() const;

	// 0 = Ringer / 1 = Manager / 2 = Owner.
	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetPlayerRole() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetCurrentCattlePrice() const;

	// M4a readouts.
	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsHerdInTransit() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetMusterDaysRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	FString GetMusterDestinationPaddock() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsVehicleOwned(int32 VehicleType) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetVehicleCondition(int32 VehicleType) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetFenceIntegrity(const FString& FenceId) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetRoadGradeQuality(const FString& RoadId) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsHandHired(const FString& HandId) const;

	// Direct C++ access for performance-sensitive reads (e.g. per-frame Actor
	// updates). Blueprints should prefer the UFUNCTION getters above.
	FStation* GetStationMutable() { return Station.Get(); }
	const FStation* GetStation() const { return Station.Get(); }

	// UE-delegate re-broadcasts of selected sim events.
	FOnTaraDayEnded OnDayEnded;
	FOnTaraCattleDied OnCattleDied;
	FOnTaraHerdConditionChanged OnHerdConditionChanged;
	FOnTaraWaterAccessLost OnWaterAccessLost;
	FOnTaraWaterAccessRestored OnWaterAccessRestored;
	FOnTaraBoreFailed OnBoreFailed;
	FOnTaraBoreRepaired OnBoreRepaired;
	FOnTaraMoneyChanged OnMoneyChanged;
	FOnTaraCattleSold OnCattleSold;
	FOnTaraSupplementOrdered OnSupplementOrdered;
	FOnTaraYearEnded OnYearEnded;
	FOnTaraMusterStarted OnMusterStarted;
	FOnTaraMusterCompleted OnMusterCompleted;
	FOnTaraMusterCancelled OnMusterCancelled;
	FOnTaraBreakaway OnBreakaway;
	FOnTaraFenceBroken OnFenceBroken;
	FOnTaraCattleDrifted OnCattleDrifted;
	FOnTaraVehiclePurchased OnVehiclePurchased;
	FOnTaraHandHired OnHandHired;
	FOnTaraHandFired OnHandFired;

private:
	TUniquePtr<FStation> Station;

	// Subscription ids so we can unsubscribe cleanly in Deinitialize.
	int32 SubIdDayEnded = -1;
	int32 SubIdCattleDied = -1;
	int32 SubIdHerdCondition = -1;
	int32 SubIdWaterLost = -1;
	int32 SubIdWaterRestored = -1;
	int32 SubIdBoreFailed = -1;
	int32 SubIdBoreRepaired = -1;
	int32 SubIdMoneyChanged = -1;
	int32 SubIdCattleSold = -1;
	int32 SubIdSupplementOrdered = -1;
	int32 SubIdYearEnded = -1;
	int32 SubIdMusterStarted = -1;
	int32 SubIdMusterCompleted = -1;
	int32 SubIdMusterCancelled = -1;
	int32 SubIdBreakaway = -1;
	int32 SubIdFenceBroken = -1;
	int32 SubIdCattleDrifted = -1;
	int32 SubIdVehiclePurchased = -1;
	int32 SubIdHandHired = -1;
	int32 SubIdHandFired = -1;

	void WireSimEventsToDelegates();
	void UnwireSimEvents();

	FString GetSaveFilePath() const;
	bool TryLoadFromDisk();
	void WriteSaveToDisk() const;
};
