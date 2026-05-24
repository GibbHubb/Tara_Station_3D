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

// M5 — events + breeding surfaced for HUD modal + paddock visual states.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraEventStarted, const FEventStartedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraEventResolved, const FEventResolvedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBadWeatherDecisionRequired, const FBadWeatherDecisionRequiredPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBadWeatherDecisionMade, const FBadWeatherDecisionMadePayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraCalendarEventDue, const FCalendarEventDuePayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBreedingWindowOpened, const FBreedingWindowOpenedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBreedingConceived, const FBreedingConceivedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraCalfBorn, const FCalfBornPayload&);

// M6 — wildlife + invasives surfaced for traversal encounters + paddock overlays.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBirdSighted, const FBirdSightedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBirdLogged, const FBirdLoggedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraPestShot, const FPestShotPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraInvasiveTreated, const FInvasiveTreatedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraInvasiveSpread, const FInvasiveSpreadPayload&);

// M7 — progression surfaced for year-end widget + role-change UI.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraRoleChanged, const FRoleChangedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraYearEvaluated, const FYearEvaluatedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraPropertyPurchased, const FPropertyPurchasedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraBankruptcyDeclared, const FBankruptcyDeclaredPayload&);

// M8 — machinery + sensors + infrastructure repair events.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraWorkMachinePurchased, const FWorkMachinePurchasedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraSensorInstalled, const FSensorInstalledPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraSensorBatterySwapped, const FSensorBatterySwappedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraFenceRepaired, const FFenceRepairedPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTaraRoadGraded, const FRoadGradedPayload&);

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

	// Real-time advance — engine ticks call this each frame (typically from a
	// GameMode Tick or the player Pawn). Converts real seconds into in-game
	// minutes via FSeasonClock::AdvanceRealTime. Triggers Station::TickDay()
	// internally when a full in-game day rolls (i.e. when 22:00 hits).
	UFUNCTION(BlueprintCallable, Category = "Tara|Pacing")
	void AdvanceRealTime(float DeltaSeconds);

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

	// M5 — player resolves a pending drought decision modal.
	// Choice: "feed" / "agist" / "sell" / "hold".
	UFUNCTION(BlueprintCallable, Category = "Tara")
	void ResolveBadWeatherDecision(const FString& Choice);

	// M6 — wildlife actions. PestSpecies / InvasiveSpecies passed as strings
	// matching 2D ids ("dingo" / "feral-cat" / "wild-pig" / "parkinsonia" /
	// "rubber-vine" / "prickly-acacia"). LogBird returns payout (0 if not the
	// first log of this species).
	UFUNCTION(BlueprintCallable, Category = "Tara")
	int32 LogBird(const FString& SpeciesId);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool ShootPest(const FString& PaddockId, const FString& Species);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool TreatInvasive(const FString& PaddockId, const FString& Species);

	// M7 — purchase your own property (Manager → Owner). Returns false on
	// insufficient funds or wrong role.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool BuyProperty();

	// Trigger a year-end evaluation now (year-end UI normally calls this on
	// the YearEnded event consumption). Returns score 0..100.
	UFUNCTION(BlueprintCallable, Category = "Tara")
	float EvaluateYearNow();

	// M8 — capital-development actions (the wet-season build mode per
	// CORE_LOOP §1). WorkMachineType: 0=Tractor / 1=FrontEndLoader /
	// 2=Forklift / 3=Digger / 4=Grader / 5=FlatBedTrailer / 6=MolassesTruck.
	// SensorKind passed as string ("rain-gauge" / "tank-flow" / "bore-pressure").
	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool BuyWorkMachine(int32 WorkMachineType);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool InstallSensor(const FString& Kind, const FString& LocationId);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool SwapSensorBattery(const FString& SensorId);

	UFUNCTION(BlueprintCallable, Category = "Tara")
	bool GradeRoad(const FString& RoadId);

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

	// M5 — drought + flood readouts for the HUD + paddock material.
	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetCurrentDroughtSeverity() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsFlooding() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetGrassGrowthMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	bool HasPendingBadWeatherDecision() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetPregnantHead() const;

	// M6 — wildlife readouts.
	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetLoggedSpeciesCount() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetPestPopulation(const FString& PaddockId, const FString& Species) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	float GetInvasiveCoverage(const FString& PaddockId, const FString& Species) const;

	// M7 — progression readouts.
	UFUNCTION(BlueprintPure, Category = "Tara")
	bool CanBuyProperty() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetDaysInBankruptcy() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsBankrupted() const;

	// M8 — work-machine + sensor readouts.
	UFUNCTION(BlueprintPure, Category = "Tara")
	bool IsWorkMachineOwned(int32 WorkMachineType) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetSensorCount() const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	FString GetSensorReading(const FString& SensorId) const;

	UFUNCTION(BlueprintPure, Category = "Tara")
	int32 GetSensorBatteryDays(const FString& SensorId) const;

	// Pacing dials (CORE_LOOP §7 — TUNABLE, do not hard-commit). Settings UI
	// can mutate via SetSecondsPerInGameDay.
	UFUNCTION(BlueprintPure, Category = "Tara|Pacing")
	float GetSecondsPerInGameDay() const;

	UFUNCTION(BlueprintCallable, Category = "Tara|Pacing")
	void SetSecondsPerInGameDay(float Seconds);

	UFUNCTION(BlueprintPure, Category = "Tara|Pacing")
	int32 GetYearsToOwnerHypothesis() const;

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
	FOnTaraEventStarted OnEventStarted;
	FOnTaraEventResolved OnEventResolved;
	FOnTaraBadWeatherDecisionRequired OnBadWeatherDecisionRequired;
	FOnTaraBadWeatherDecisionMade OnBadWeatherDecisionMade;
	FOnTaraCalendarEventDue OnCalendarEventDue;
	FOnTaraBreedingWindowOpened OnBreedingWindowOpened;
	FOnTaraBreedingConceived OnBreedingConceived;
	FOnTaraCalfBorn OnCalfBorn;
	FOnTaraBirdSighted OnBirdSighted;
	FOnTaraBirdLogged OnBirdLogged;
	FOnTaraPestShot OnPestShot;
	FOnTaraInvasiveTreated OnInvasiveTreated;
	FOnTaraInvasiveSpread OnInvasiveSpread;
	FOnTaraRoleChanged OnRoleChanged;
	FOnTaraYearEvaluated OnYearEvaluated;
	FOnTaraPropertyPurchased OnPropertyPurchased;
	FOnTaraBankruptcyDeclared OnBankruptcyDeclared;
	FOnTaraWorkMachinePurchased OnWorkMachinePurchased;
	FOnTaraSensorInstalled OnSensorInstalled;
	FOnTaraSensorBatterySwapped OnSensorBatterySwapped;
	FOnTaraFenceRepaired OnFenceRepaired;
	FOnTaraRoadGraded OnRoadGraded;

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
	int32 SubIdEventStarted = -1;
	int32 SubIdEventResolved = -1;
	int32 SubIdBadWeatherDecisionRequired = -1;
	int32 SubIdBadWeatherDecisionMade = -1;
	int32 SubIdCalendarEventDue = -1;
	int32 SubIdBreedingWindowOpened = -1;
	int32 SubIdBreedingConceived = -1;
	int32 SubIdCalfBorn = -1;
	int32 SubIdBirdSighted = -1;
	int32 SubIdBirdLogged = -1;
	int32 SubIdPestShot = -1;
	int32 SubIdInvasiveTreated = -1;
	int32 SubIdInvasiveSpread = -1;
	int32 SubIdRoleChanged = -1;
	int32 SubIdYearEvaluated = -1;
	int32 SubIdPropertyPurchased = -1;
	int32 SubIdBankruptcyDeclared = -1;
	int32 SubIdWorkMachinePurchased = -1;
	int32 SubIdSensorInstalled = -1;
	int32 SubIdSensorBatterySwapped = -1;
	int32 SubIdFenceRepaired = -1;
	int32 SubIdRoadGraded = -1;

	void WireSimEventsToDelegates();
	void UnwireSimEvents();

	FString GetSaveFilePath() const;
	bool TryLoadFromDisk();
	void WriteSaveToDisk() const;
};
