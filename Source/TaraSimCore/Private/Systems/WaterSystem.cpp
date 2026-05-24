#include "Systems/WaterSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Paddock.h"
#include "Entities/Bore.h"

FWaterSystem::FWaterSystem(FEventBus& InBus, FStation& InStation)
	: Bus(InBus), Station(InStation)
{
}

void FWaterSystem::TickDay()
{
	for (FBore& Bore : Station.GetBores())
	{
		if (Bore.Status == EBoreStatus::Failed) continue;

		Bore.Condition = FBore::ClampCondition(Bore.Condition - DailyConditionDecay);

		if (Bore.Condition < FailureThreshold)
		{
			const float Prob = FailureDailyProbAtZero * (1.0f - Bore.Condition / FailureThreshold);
			if (FMath::FRand() < Prob)
			{
				Bore.Status = EBoreStatus::Failed;
				FBoreFailedPayload P;
				P.BoreId = Bore.Id;
				P.PaddockIds = Bore.ServesPaddockIds;
				Bus.BoreFailed.Emit(P);
			}
		}
	}
	RecomputeAccess();
}

void FWaterSystem::RecomputeAccess()
{
	for (const FPaddock& Paddock : Station.GetPaddocks())
	{
		// Find serving bores.
		TArray<const FBore*> Serving;
		for (const FBore& B : Station.GetBores())
		{
			if (B.ServesPaddockIds.Contains(Paddock.Id))
			{
				Serving.Add(&B);
			}
		}

		// Working subset.
		TArray<const FBore*> Working;
		for (const FBore* B : Serving)
		{
			if (B->Status == EBoreStatus::Working) Working.Add(B);
		}

		EWaterAccess Next;
		if (Working.Num() == 0)
		{
			Next = EWaterAccess::None;
		}
		else
		{
			bool bAnyAboveLow = false;
			for (const FBore* B : Working)
			{
				if (B->Condition > LowCondition) { bAnyAboveLow = true; break; }
			}
			Next = bAnyAboveLow ? EWaterAccess::Full : EWaterAccess::Low;
		}

		const EWaterAccess Prev = AccessByPaddock.FindRef(Paddock.Id);
		AccessByPaddock.Add(Paddock.Id, Next);

		if (Prev != Next)
		{
			if (Next == EWaterAccess::None)
			{
				FWaterAccessLostPayload P;
				P.PaddockId = Paddock.Id;
				Bus.WaterAccessLost.Emit(P);
			}
			else if (Prev == EWaterAccess::None)
			{
				FWaterAccessRestoredPayload P;
				P.PaddockId = Paddock.Id;
				Bus.WaterAccessRestored.Emit(P);
			}
		}
	}
}

EWaterAccess FWaterSystem::GetAccess(const FString& PaddockId) const
{
	const EWaterAccess* P = AccessByPaddock.Find(PaddockId);
	return P ? *P : EWaterAccess::Full;
}

FWaterSystem::FCheckBoreResult FWaterSystem::CheckBore(const FString& BoreId, int32 CurrentDayOfYear)
{
	FCheckBoreResult Result;

	FBore* Bore = nullptr;
	for (FBore& B : Station.GetBores())
	{
		if (B.Id == BoreId) { Bore = &B; break; }
	}
	if (!Bore) return Result;

	Bore->LastCheckedDay = CurrentDayOfYear;

	if (Bore->Status == EBoreStatus::Failed)
	{
		if (FMath::FRand() < CheckBoreBaseSuccess)
		{
			Bore->Status = EBoreStatus::Working;
			Bore->Condition = FBore::ClampCondition(FMath::Max(60.0f, Bore->Condition));
			FBoreRepairedPayload P;
			P.BoreId = BoreId;
			Bus.BoreRepaired.Emit(P);
			RecomputeAccess();
			Result.bFixed = true;
			Result.DaysSpent = CheckBoreDayCost;
			return Result;
		}
		Result.DaysSpent = CheckBoreDayCost;
		return Result;
	}

	// Servicing a working bore — small condition boost.
	Bore->Condition = FBore::ClampCondition(Bore->Condition + 10.0f);
	Result.bFixed = true;
	Result.DaysSpent = CheckBoreDayCost;
	return Result;
}
