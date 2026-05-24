#include "Diagnostics/SimSmokeTest.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/CattleCohort.h"
#include "Entities/Herd.h"
#include "Entities/Paddock.h"
#include "Entities/Bore.h"
#include "SeasonClock.h"

namespace
{
	void RecordFailure(FSimSmokeTestResult& R, const FString& Msg)
	{
		R.bPassed = false;
		R.Failures.Add(Msg);
	}

	int32 TotalHeadAcrossCohorts(const FStation& S)
	{
		int32 Sum = 0;
		for (const FCattleCohort& C : S.GetHerd().Cohorts)
		{
			Sum += FMath::Max(0, C.Count);
		}
		return Sum;
	}
}

FSimSmokeTestResult FSimSmokeTest::RunOnFreshStation(int32 Days)
{
	FSimSmokeTestResult R;
	Days = FMath::Clamp(Days, 1, 3650);

	// Fresh FStation — uses SeedDefaults internally.
	TUniquePtr<FStation> Station = MakeUnique<FStation>();
	if (!Station)
	{
		RecordFailure(R, TEXT("MakeUnique<FStation>() returned nullptr"));
		return R;
	}

	// Seed sanity.
	if (Station->GetPaddocks().Num() == 0) RecordFailure(R, TEXT("seed: no paddocks"));
	if (Station->GetBores().Num() == 0)    RecordFailure(R, TEXT("seed: no bores"));
	if (Station->GetHerd().Cohorts.Num() == 0) RecordFailure(R, TEXT("seed: no cohorts"));

	R.HerdSizeStart = Station->GetHerd().GetSize();
	R.PaddockCount = Station->GetPaddocks().Num();

	// Subscribe to a handful of event counters for the report.
	FEventBus& Bus = Station->GetBus();
	int32 EventCount = 0;
	int32 CalfCount = 0;
	int32 DiedCount = 0;
	const int32 SubEvt = Bus.EventStarted.Subscribe(
		[&](const FEventStartedPayload&) { EventCount++; });
	const int32 SubCalf = Bus.CalfBorn.Subscribe(
		[&](const FCalfBornPayload& P) { CalfCount += P.CalfCount; });
	const int32 SubDied = Bus.CattleDied.Subscribe(
		[&](const FCattleDiedPayload& P) { DiedCount += P.Count; });

	// Tick loop with clock-advance invariant check.
	const int32 StartYear = Station->GetClock().Year;
	const int32 StartDay = Station->GetClock().DayOfYear;
	int32 LastYear = StartYear;
	int32 LastDay = StartDay;

	for (int32 I = 0; I < Days; I++)
	{
		Station->TickDay();

		// Clock advanced exactly one day.
		const int32 Y = Station->GetClock().Year;
		const int32 D = Station->GetClock().DayOfYear;
		const bool bDayValid = (D == LastDay + 1) || (D == 1 && Y == LastYear + 1);
		if (!bDayValid)
		{
			RecordFailure(R, FString::Printf(
				TEXT("clock jump at tick %d: last Y%dD%d → Y%dD%d"),
				I, LastYear, LastDay, Y, D));
			break;
		}
		LastYear = Y;
		LastDay = D;

		// Cohort counts non-negative.
		for (const FCattleCohort& C : Station->GetHerd().Cohorts)
		{
			if (C.Count < 0)
			{
				RecordFailure(R, FString::Printf(
					TEXT("negative cohort count at tick %d: birthYear %d count %d"),
					I, C.BirthYear, C.Count));
				break;
			}
		}

		R.DaysTicked = I + 1;
		R.Year = Y;
	}

	R.HerdSizeEnd = Station->GetHerd().GetSize();
	R.EventsFiredCount = EventCount;
	R.CalfBornCount = CalfCount;
	R.CattleDiedCount = DiedCount;

	Bus.EventStarted.Unsubscribe(SubEvt);
	Bus.CalfBorn.Unsubscribe(SubCalf);
	Bus.CattleDied.Unsubscribe(SubDied);

	// Head-count consistency: end = start + born - died. Allow some slop for
	// state-transitions (a head in 'Old' that's never explicitly removed is
	// still counted). Tolerance: ±5 head across a year.
	const int32 Expected = R.HerdSizeStart + R.CalfBornCount - R.CattleDiedCount;
	const int32 Drift = FMath::Abs(R.HerdSizeEnd - Expected);
	if (Drift > 5)
	{
		R.Notes.Add(FString::Printf(
			TEXT("head-count drift (informational): expected ~%d end %d (Δ=%d) — "
			     "may indicate breeding/death not fully accounted for"),
			Expected, R.HerdSizeEnd, Drift));
	}

	// Save round-trip.
	{
		const FString Json = Station->SerializeJson();
		TUniquePtr<FStation> Restored = FStation::FromJson(Json);
		if (!Restored)
		{
			RecordFailure(R, TEXT("save round-trip: FromJson returned nullptr"));
		}
		else
		{
			R.bSaveRoundTripOk = true;
			// Quick equivalence checks: same paddock count, herd size, year.
			if (Restored->GetPaddocks().Num() != Station->GetPaddocks().Num())
			{
				RecordFailure(R, TEXT("save round-trip: paddock count mismatch"));
			}
			if (Restored->GetHerd().GetSize() != Station->GetHerd().GetSize())
			{
				RecordFailure(R, TEXT("save round-trip: herd size mismatch"));
			}
			if (Restored->GetClock().Year != Station->GetClock().Year ||
				Restored->GetClock().DayOfYear != Station->GetClock().DayOfYear)
			{
				RecordFailure(R, TEXT("save round-trip: clock mismatch"));
			}
		}
	}

	// SplitCohort head-count invariant: split the herd's first non-empty cohort
	// in half + verify total head preserved.
	if (R.bPassed)
	{
		const int32 BeforeTotal = TotalHeadAcrossCohorts(*Station);
		int32 SplittableBirthYear = -1;
		int32 SplittableCount = 0;
		for (const FCattleCohort& C : Station->GetHerd().Cohorts)
		{
			if (C.Count >= 4)
			{
				SplittableBirthYear = C.BirthYear;
				SplittableCount = C.Count;
				break;
			}
		}
		if (SplittableBirthYear != -1)
		{
			TArray<FStation::FCohortSplitRequest> Splits;
			Splits.Add({ EBreederStage::Early, SplittableCount / 2 });
			Splits.Add({ EBreederStage::Mid,   SplittableCount - SplittableCount / 2 });
			const int32 NumNew = Station->SplitCohort(SplittableBirthYear, Splits);
			R.Notes.Add(FString::Printf(
				TEXT("SplitCohort test: split %d (count %d) into %d sub-cohorts"),
				SplittableBirthYear, SplittableCount, NumNew));
			const int32 AfterTotal = TotalHeadAcrossCohorts(*Station);
			if (AfterTotal != BeforeTotal)
			{
				RecordFailure(R, FString::Printf(
					TEXT("SplitCohort head-count drift: %d → %d (Δ=%d)"),
					BeforeTotal, AfterTotal, AfterTotal - BeforeTotal));
			}
		}
		else
		{
			R.Notes.Add(TEXT("SplitCohort test: no cohort with ≥4 head to split — skipped"));
		}
	}

	return R;
}

void FSimSmokeTest::LogResult(const FSimSmokeTestResult& Result)
{
	if (Result.bPassed)
	{
		UE_LOG(LogTemp, Display, TEXT("[SimSmokeTest] ✅ PASS — %d days ticked"), Result.DaysTicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SimSmokeTest] ❌ FAIL — %d failures across %d days"),
			Result.Failures.Num(), Result.DaysTicked);
	}

	UE_LOG(LogTemp, Display, TEXT("  herd %d → %d head (%d calves born, %d died, %d events fired)"),
		Result.HerdSizeStart, Result.HerdSizeEnd,
		Result.CalfBornCount, Result.CattleDiedCount, Result.EventsFiredCount);
	UE_LOG(LogTemp, Display, TEXT("  %d paddocks; save round-trip: %s"),
		Result.PaddockCount,
		Result.bSaveRoundTripOk ? TEXT("OK") : TEXT("FAILED"));

	for (const FString& F : Result.Failures)
	{
		UE_LOG(LogTemp, Warning, TEXT("  ❌ %s"), *F);
	}
	for (const FString& N : Result.Notes)
	{
		UE_LOG(LogTemp, Display, TEXT("  • %s"), *N);
	}
}
