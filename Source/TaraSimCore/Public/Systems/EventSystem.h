#pragma once

#include "CoreMinimal.h"
#include "../Entities/StationEvent.h"

class FStation;
class FEventBus;

// Mirrors src/sim/systems/EventSystem.ts. Daily probability roll for random
// events (drought / flood / fire / storm / breakdown) modulated by season;
// calendar events fire on fixed days (rodeo day 120, draft day 220).
//
// Per CORE_LOOP §7: a typical Tara wet is generally consistent with occasional
// bad years — the BASE_PROB constants in the .cpp are tuned to match (drought
// roll ~0.25%/day in the dry caps annual bad-wet occurrence in the
// "~15-25%/year" range Max wanted). Adjust these in playtest, not in the
// design doc.

class TARASIMCORE_API FEventSystem
{
public:
	FEventSystem(FStation& InStation, FEventBus& InBus);

	void TickDay();

	// Reads called by other systems + the bridge.
	float CurrentDroughtSeverity() const;
	bool IsFlooding() const;
	float GrassGrowthMultiplier() const;

	const TArray<FStationEvent>& GetActive() const { return Active; }
	const TArray<FStationEvent>& GetHistory() const { return History; }
	const FString& GetPendingBadWeatherDecision() const { return PendingBadWeatherDecision; }

	// Player decision on a drought "feed/agist/sell/hold" choice. Mirrors 2D.
	void ResolveBadWeatherDecision(const FString& Choice);

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

	static int32 GetEventIdCounter() { return EventIdCounter; }
	static void SetEventIdCounter(int32 V) { EventIdCounter = FMath::Max(V, 1); }

private:
	FStation& Station;
	FEventBus& Bus;

	TArray<FStationEvent> Active;
	TArray<FStationEvent> History;
	FString PendingBadWeatherDecision;

	static int32 EventIdCounter;

	void FireRandomEvent(EStationEventType Type, int32 Day);
	void ApplyOngoingEffect(FStationEvent& E);
};
