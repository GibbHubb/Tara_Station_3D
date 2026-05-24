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

	// ---- Pacing (per CORE_LOOP §7 — TUNABLE; do not hard-commit) ----
	//
	// Real-world seconds the clock advances over one in-game minute. Starting
	// hypothesis 30 min/day (CORE_LOOP §7 range was 25-45 min/day; midpoint
	// keeps room to tune either way once playtest lands). One in-game minute
	// = 1800/(60*16) = 1.875 real seconds when WakeHour..SleepHour spans 16
	// awake hours. Sleep skips, so a "real" day is awake-time only.
	//
	// Lean LONGER/CALMER than Stardew. Set by feel in playtest.
	static constexpr float DefaultRealSecondsPerInGameDay = 1800.0f;

	// Active dial. Engine ticks call AdvanceRealTime(delta) each frame; the
	// clock converts that into in-game minutes using this value. Mutate at
	// runtime (e.g. settings menu) to retune without recompile.
	float RealSecondsPerInGameDay = DefaultRealSecondsPerInGameDay;

	// How many real seconds equal one in-game minute, given the current dial.
	// Awake span = SleepHour - WakeHour = 16 hours by default; intra-day
	// minutes count only that awake span (sleep jumps to next WakeHour).
	float RealSecondsPerInGameMinute() const
	{
		const int32 AwakeMinutes = FMath::Max(1, (SleepHour - WakeHour) * 60);
		return RealSecondsPerInGameDay / (float)AwakeMinutes;
	}

	FSeasonClock() = default;

	// One-day tick. Year/day++; resets hour to WakeHour.
	void TickDay();

	// Intra-day advance. Returns true if the day rolled (sleep / midnight).
	bool AdvanceMinutes(int32 Minutes);

	// Real-time advance — engine calls this each frame with the delta seconds.
	// Accumulates fractional in-game minutes; advances whole minutes when
	// enough has built up. Returns true if a day rolled during this advance.
	// The fractional remainder carries to the next call; not serialised
	// (precision-loss across a save is invisible at the per-frame level).
	bool AdvanceRealTime(float DeltaSeconds);

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

private:
	// Fractional carry for AdvanceRealTime — accumulates < 1 minute of real
	// time across frames. Reset on save/load (lossy fraction of a minute is
	// inconsequential).
	float RealTimeAccumulatorSeconds = 0.0f;
};
