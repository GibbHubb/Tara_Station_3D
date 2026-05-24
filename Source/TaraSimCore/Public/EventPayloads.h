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

// Later milestones extend this file. Keep it the only place payload shapes are
// declared — that way the EventBus.h template just refers to types from here.
