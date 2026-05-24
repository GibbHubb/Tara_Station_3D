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

// ---- M6 — wildlife + invasives ----

struct FBirdSightedPayload
{
	FString SpeciesId;
	FString Name;
};

struct FBirdLoggedPayload
{
	FString SpeciesId;
	int32 Payout = 0;
	bool bFirst = false;
};

struct FPestShotPayload
{
	FString PaddockId;
	int32 Species = 0;  // EPestSpecies as int
};

struct FInvasiveTreatedPayload
{
	FString PaddockId;
	int32 Species = 0;  // EInvasiveSpecies as int
	float NewCoverage = 0.0f;
};

struct FInvasiveSpreadPayload
{
	FString FromPaddockId;
	FString ToPaddockId;
	int32 Species = 0;
};

// ---- M7 — progression ----

struct FRoleChangedPayload
{
	int32 From = 0;   // EPlayerRole as int
	int32 To = 0;
};

struct FYearEvaluatedPayload
{
	int32 Year = 0;
	float Score = 0.0f;
	int32 NewRole = 0;
	bool bPromoted = false;
	bool bFired = false;
};

struct FPropertyPurchasedPayload
{
	int32 Cost = 0;
};

struct FBankruptcyDeclaredPayload
{
	int32 Year = 0;
	int32 DaysInDebt = 0;
};

// ---- M8 — machinery + sensors ----

struct FWorkMachinePurchasedPayload
{
	int32 Type = 0;   // EWorkMachineType as int
	int32 Cost = 0;
};

struct FSensorInstalledPayload
{
	FString SensorId;
	int32 Kind = 0;   // ESensorKind as int
	FString LocationId;
};

struct FSensorBatterySwappedPayload
{
	FString SensorId;
};

struct FFenceRepairedPayload
{
	FString FenceId;
};

struct FRoadGradedPayload
{
	FString RoadId;
};

// ---- Phase 5+ cohort lifecycle (TS-3D-WEANER-SCHOOL + TS-3D-PREG-TEST) ----

struct FCohortWeanedPayload
{
	int32 CalfCohortBirthYear = 0;
	int32 DamCohortBirthYear = 0;   // -1 if dam not found
};

struct FCohortSplitPayload
{
	int32 SourceCohortBirthYear = 0;
	int32 NumSplits = 0;            // how many new sub-cohorts were created
};

// Later milestones extend this file. Keep it the only place payload shapes are
// declared — that way the EventBus.h template just refers to types from here.
