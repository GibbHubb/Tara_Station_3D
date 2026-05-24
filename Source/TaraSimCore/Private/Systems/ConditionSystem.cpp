#include "Systems/ConditionSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Paddock.h"
#include "Entities/Herd.h"
#include "Entities/CattleCohort.h"
#include "Systems/WaterSystem.h"
#include "Systems/EconomySystem.h"

FConditionSystem::FConditionSystem(FEventBus& InBus, FStation& InStation)
	: Bus(InBus), Station(InStation)
{
}

void FConditionSystem::TickDay()
{
	FHerd& Herd = Station.GetHerd();

	const FPaddock* Paddock = Station.PaddockById(Herd.CurrentPaddockId);
	if (!Paddock) return;

	// M3 — supplemental feeding lifts the *effective* grass before threshold checks.
	const float Lift = Station.GetEconomySystem().ActiveGrassFloorLift();
	const float RecoveryBoost = Station.GetEconomySystem().ActiveRecoveryBoost();
	const float Grass = Paddock->GrassKgPerHa + Lift;

	float Delta;
	if (Grass >= GrassThreshold)        Delta = RecoveryPerDay + RecoveryBoost;
	else if (Grass >= GrassFloor)       Delta = RecoveryBoost;
	else if (Grass >= GrassCritical)    Delta = -DeclinePerDay + RecoveryBoost;
	else                                Delta = -StarvationPerDay + RecoveryBoost;

	// M2 — water access on top of grass (locked from TS8: supplements do NOT
	// bypass dehydration — water-decay applied AFTER lifts).
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
