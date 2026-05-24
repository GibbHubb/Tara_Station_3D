#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/StationEvent.ts. Random + calendar events.
// Distinct entity name (StationEvent vs Event) avoids clash with engine types.
//
// Audit: 🟢 — entity ports unchanged. The 3D layer adds *visualisation* per
// type (drought bleaches grass, flood fills creeks, fire is a particle, storm
// darkens sky) but the underlying event records are the same.

enum class EStationEventType : uint8
{
	Drought   = 0,
	Flood     = 1,
	Fire      = 2,
	Storm     = 3,
	Breakdown = 4,
	Rodeo     = 5,    // calendar event
	Draft     = 6,    // calendar event
};

enum class EStationEventKind : uint8
{
	Random   = 0,
	Calendar = 1,
};

struct FStationEventConfig
{
	FString Id;
	EStationEventType Type = EStationEventType::Drought;
	EStationEventKind Kind = EStationEventKind::Random;
	float Severity = 0.5f;       // 0..1
	int32 StartDay = 0;
	int32 DurationDays = 1;
	TArray<FString> PaddockIds;
};

class TARASIMCORE_API FStationEvent
{
public:
	FStationEvent() = default;
	explicit FStationEvent(const FStationEventConfig& Config);

	FString SerializeJson() const;
	static FStationEvent FromJson(const FString& Json);

public:
	FString Id;
	EStationEventType Type = EStationEventType::Drought;
	EStationEventKind Kind = EStationEventKind::Random;
	float Severity = 0.5f;
	int32 StartDay = 0;
	int32 DurationDays = 1;
	TArray<FString> PaddockIds;
	bool bResolved = false;
	FString Outcome;  // empty = unresolved; "expired" / "feed" / "agist" / "sell" / "hold"
};
