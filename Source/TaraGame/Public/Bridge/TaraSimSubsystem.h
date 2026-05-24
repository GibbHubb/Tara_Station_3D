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

	// Direct C++ access for performance-sensitive reads (e.g. per-frame Actor
	// updates). Blueprints should prefer the UFUNCTION getters above.
	FStation* GetStationMutable() { return Station.Get(); }
	const FStation* GetStation() const { return Station.Get(); }

	// UE-delegate re-broadcasts of selected sim events.
	FOnTaraDayEnded OnDayEnded;
	FOnTaraCattleDied OnCattleDied;
	FOnTaraHerdConditionChanged OnHerdConditionChanged;

private:
	TUniquePtr<FStation> Station;

	// Subscription ids so we can unsubscribe cleanly in Deinitialize.
	int32 SubIdDayEnded = -1;
	int32 SubIdCattleDied = -1;
	int32 SubIdHerdCondition = -1;

	void WireSimEventsToDelegates();
	void UnwireSimEvents();

	FString GetSaveFilePath() const;
	bool TryLoadFromDisk();
	void WriteSaveToDisk() const;
};
