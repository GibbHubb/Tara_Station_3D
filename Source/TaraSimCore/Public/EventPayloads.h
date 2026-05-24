#pragma once

#include "CoreMinimal.h"

// All sim-layer event payloads. Plain structs — no UStruct, no UObject.
// Mirrors the EventMap from src/sim/EventBus.ts in the 2D project, one struct
// per event type. Adding a new event = adding a struct here + adding a TEvent
// member to FEventBus.

struct FDayStartedPayload
{
	int32 Day;
	int32 Year;
};

struct FDayEndedPayload
{
	int32 Day;
	int32 Year;
};

struct FHerdConditionChangedPayload
{
	FString HerdId;
	int32 CohortBirthYear;
	float Delta;
	float Mean;
};

struct FCattleDiedPayload
{
	FString HerdId;
	int32 CohortBirthYear;
	int32 Count;
	FString Cause;
};

// ---- M2 — water ----

struct FWaterAccessLostPayload
{
	FString PaddockId;
};

struct FWaterAccessRestoredPayload
{
	FString PaddockId;
};

struct FBoreFailedPayload
{
	FString BoreId;
	TArray<FString> PaddockIds;
};

struct FBoreRepairedPayload
{
	FString BoreId;
};

// ---- M3 — economy ----

struct FMoneyChangedPayload
{
	int32 Delta;
	FString Reason;
	int32 NewCash;
};

struct FCattleSoldPayload
{
	int32 Count;
	int32 PricePerHead;
	int32 Total;
};

struct FSupplementOrderedPayload
{
	FString Kind;
	int32 Cost;
	int32 ArrivesInDays;
};

struct FYearEndedPayload
{
	int32 Year;
};

// ---- M4a — mustering, vehicles, fences ----

struct FMusterStartedPayload
{
	FString FromPaddockId;
	FString ToPaddockId;
	int32 VehicleType = 0;     // EVehicleType as int (avoids cross-include)
	TArray<FString> HandIds;
	int32 PlannedDays = 0;
	int32 FuelCost = 0;
	int32 BreakawayPending = 0;
};

struct FMusterCompletedPayload
{
	FString ToPaddockId;
	int32 VehicleType = 0;
	TArray<FString> HandIds;
	int32 BreakawayHead = 0;   // actual breakaway head removed from cohorts
};

struct FMusterCancelledPayload
{
	FString ToPaddockId;
	int32 DaysSpent = 0;
	int32 Refund = 0;          // $ fuel-refund applied to player cash
};

struct FBreakawayPayload
{
	int32 Count = 0;
	FString PaddockId;         // paddock at the moment of breakaway (typically destination)
};

struct FFenceBrokenPayload
{
	FString FenceId;
	FString PaddockA;
	FString PaddockB;
};

struct FCattleDriftedPayload
{
	FString FromPaddockId;
	FString ToPaddockId;
	int32 Count = 0;
};

struct FVehiclePurchasedPayload
{
	int32 VehicleType = 0;
	int32 Cost = 0;
};

struct FHandHiredPayload
{
	FString HandId;
};

struct FHandFiredPayload
{
	FString HandId;
};

// ---- M5 — events + breeding ----

struct FEventStartedPayload
{
	int32 Type = 0;            // EStationEventType as int
	float Severity = 0.0f;
	int32 DurationDays = 0;
	FString EventId;
};

struct FEventResolvedPayload
{
	int32 Type = 0;
	FString EventId;
	FString Outcome;
};

struct FBadWeatherDecisionRequiredPayload
{
	FString EventId;
};

struct FBadWeatherDecisionMadePayload
{
	FString EventId;
	FString Choice;   // "feed" / "agist" / "sell" / "hold"
};

struct FCalendarEventDuePayload
{
	FString Type;     // "rodeo" / "draft"
};

struct FBreedingWindowOpenedPayload
{
	int32 CohortBirthYear = -1;
};

struct FBreedingConceivedPayload
{
	int32 CohortBirthYear = -1;
	int32 PregnantCount = 0;
};

struct FCalfBornPayload
{
	int32 CohortBirthYear = 0;
	int32 CalfCount = 0;
};

// Later milestones extend this file. Keep it the only place payload shapes are
// declared — that way the EventBus.h template just refers to types from here.
