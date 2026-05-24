#include "Systems/ConditionSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Paddock.h"
#include "Entities/Herd.h"
#include "Entities/CattleCohort.h"
#include "Systems/WaterSystem.h"

FConditionSystem::FConditionSystem(FEventBus& InBus, FStation& InStation)
	: Bus(InBus), Station(InStation)
{
}

void FConditionSystem::TickDay()
{
	FHerd& Herd = Station.GetHerd();

	const FPaddock* Paddock = Station.PaddockById(Herd.CurrentPaddockId);
	if (!Paddock) return;

	const float Grass = Paddock->GrassKgPerHa;
	float Delta;
	if (Grass >= GrassThreshold)        Delta = RecoveryPerDay;
	else if (Grass >= GrassFloor)       Delta = 0.0f;
	else if (Grass >= GrassCritical)    Delta = -DeclinePerDay;
	else                                Delta = -StarvationPerDay;

	// M2 — water access on top of grass (locked from TS8: supplements do NOT
	// bypass dehydration when M3 lands).
	const EWaterAccess Water = Station.GetWaterAccess(Paddock->Id);
	if (Water == EWaterAccess::None) Delta -= WaterNoneExtraDecay;
	else if (Water == EWaterAccess::Low) Delta -= WaterLowExtraDecay;

	ApplyDelta(Herd, Delta, Paddock->Id, Paddock->Name);
}

void FConditionSystem::ApplyDelta(FHerd& Herd, float Delta, const FString& HerdId, const FString& LocationName)
{
	for (FCattleCohort& Cohort : Herd.Cohorts)
	{
		if (Cohort.Count <= 0) continue;

		const float Prev = Cohort.ConditionMean;
		Cohort.ConditionMean = FCattleCohort::ClampCondition(Prev + Delta);

		if (Cohort.ConditionMean < StarvationCondition)
		{
			Cohort.ConsecutiveStarvationDays += 1;
		}
		else
		{
			Cohort.ConsecutiveStarvationDays = 0;
		}

		if (Cohort.ConsecutiveStarvationDays >= StarvationGraceDays)
		{
			const int32 Lost = FMath::Max(1, (int32)FMath::FloorToInt32(Cohort.Count * DeathFractionPerDay));
			const int32 Actual = FMath::Min(Lost, Cohort.Count);
			Cohort.Count -= Actual;

			FCattleDiedPayload P;
			P.HerdId = HerdId;
			P.CohortBirthYear = Cohort.BirthYear;
			P.Count = Actual;
			P.Cause = FString::Printf(TEXT("starvation (%s)"), *LocationName);
			Bus.CattleDied.Emit(P);

			if (Cohort.Count == 0)
			{
				Cohort.ConsecutiveStarvationDays = 0;
			}
		}

		if (Delta != 0.0f && Cohort.ConditionMean != Prev)
		{
			FHerdConditionChangedPayload P;
			P.HerdId = HerdId;
			P.CohortBirthYear = Cohort.BirthYear;
			P.Delta = Cohort.ConditionMean - Prev;
			P.Mean = Cohort.ConditionMean;
			Bus.HerdConditionChanged.Emit(P);
		}
	}
}
