#pragma once

#include "CoreMinimal.h"

class FStation;

// Pure-C++ sim smoke test — runs against a `FStation` (or a fresh-constructed
// one), ticks N days, asserts a set of invariants the sim must preserve, and
// returns a structured result. Callable from the bridge subsystem's exec
// command `Tara.RunSmokeTest <days>` (see UTaraSimSubsystem::RunSmokeTest).
//
// Catches regressions when Phase 5+ work touches existing sim systems —
// runs over a full simulated year + verifies that:
//   - Cohort counts never go negative.
//   - Save/load round-trips preserve state byte-for-equivalent.
//   - The clock advances correctly (no day skipped, no year skipped).
//   - SeedDefaults state is sane (≥1 paddock, ≥1 cohort, ≥1 bore).
//   - bLactating dam flag toggles correctly through breeding cycle.
//   - SplitCohort + WeanCohort preserve head-count totals.
//   - EventSystem fires events in plausible ranges (no infinite-loop emit).
//
// Stateless — does not mutate any existing Station. Builds its own fresh
// FStation for testing.

struct FSimSmokeTestResult
{
	bool bPassed = true;
	int32 DaysTicked = 0;
	int32 Year = 0;
	int32 HerdSizeStart = 0;
	int32 HerdSizeEnd = 0;
	int32 PaddockCount = 0;
	int32 EventsFiredCount = 0;          // total EventStarted from EventSystem
	int32 CalfBornCount = 0;             // total CalfBorn payloads
	int32 CattleDiedCount = 0;
	bool bSaveRoundTripOk = false;
	TArray<FString> Failures;            // empty if bPassed
	TArray<FString> Notes;               // informational lines for the log
};

class TARAGAME_API FSimSmokeTest
{
public:
	// Runs the smoke test on a fresh FStation for the given number of days
	// (1..3650 reasonable; ticks beyond ~10 years risk cohort accumulation
	// pushing memory but not invariant failure).
	static FSimSmokeTestResult RunOnFreshStation(int32 Days);

	// Pretty-prints the result to UE_LOG at LogTemp Display (pass) or Warning (fail).
	static void LogResult(const FSimSmokeTestResult& Result);
};
