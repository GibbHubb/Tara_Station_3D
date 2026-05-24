#pragma once

#include "CoreMinimal.h"

enum class ESeason : uint8
{
	Wet,
	Dry,
};

// Sim-layer clock — tracks year, day-of-year, and intra-day hour/minute.
// Mirrors src/sim/SeasonClock.ts. The directional sun in TaraGame reads from
// this clock; the sim doesn't know or care that there's a sun light.

class TARASIMCORE_API FSeasonClock
{
public:
	static constexpr int32 WakeHour = 6;
	static constexpr int32 SleepHour = 22;
	static constexpr int32 DaysPerYear = 365;

	FSeasonClock() = default;

	// One-day tick. Year/day++; resets hour to WakeHour.
	void TickDay();

	// Intra-day advance. Returns true if the day rolled (sleep / midnight).
	bool AdvanceMinutes(int32 Minutes);

	ESeason GetSeason() const;
	FString FormatTime() const; // "HH:MM"

	// JSON-friendly serialisation (manual; deliberately no FArchive).
	FString SerializeJson() const;
	static FSeasonClock FromJson(const FString& Json);

public:
	int32 Year = 1;
	int32 DayOfYear = 1;
	int32 Hour = WakeHour;
	int32 Minute = 0;
};
