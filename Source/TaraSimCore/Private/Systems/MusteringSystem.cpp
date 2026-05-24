#include "Systems/MusteringSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Player.h"
#include "Entities/Herd.h"
#include "Systems/EconomySystem.h"

FMusteringSystem::FMusteringSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FMusteringSystem::TickDay()
{
	FHerd& Herd = Station.GetHerd();
	if (!Herd.IsInTransit()) return;

	const FMusterState SnapshotBeforeTick = Herd.GetMuster();
	const FHerd::FMusterTickResult R = Herd.TickMusterDay();

	if (R.bCompletedThisTick)
	{
		OnMusterCompleted();

		FMusterCompletedPayload Payload;
		Payload.ToPaddockId = R.CompletedToPaddockId;
		Payload.VehicleType = SnapshotBeforeTick.VehicleType;
		Payload.HandIds = SnapshotBeforeTick.HandIds;
		Payload.BreakawayHead = R.BreakawayApplied;
		Bus.MusterCompleted.Emit(Payload);

		if (R.BreakawayApplied > 0)
		{
			FBreakawayPayload BP;
			BP.Count = R.BreakawayApplied;
			BP.PaddockId = R.CompletedToPaddockId;
			Bus.Breakaway.Emit(BP);
		}
	}
}

FMusteringSystem::FCancelResult FMusteringSystem::CancelMuster()
{
	FCancelResult R;

	FHerd& Herd = Station.GetHerd();
	if (!Herd.IsInTransit()) return R;

	const FMusterState Snapshot = Herd.GetMuster();
	const FHerd::FCancelResult Cancel = Herd.CancelMuster();
	if (!Cancel.bWasInTransit) return R;

	R.bWasInTransit = true;
	R.DaysSpent = Cancel.DaysElapsed;
	R.FuelRefund = FMath::RoundToInt(Snapshot.FuelCost * FuelRefundFractionOnCancel);

	if (R.FuelRefund > 0)
	{
		Station.GetEconomySystem().AddCash(R.FuelRefund, TEXT("muster cancelled — fuel refund"));
	}

	FMusterCancelledPayload Payload;
	Payload.ToPaddockId = Snapshot.ToPaddockId;
	Payload.DaysSpent = R.DaysSpent;
	Payload.Refund = R.FuelRefund;
	Bus.MusterCancelled.Emit(Payload);

	return R;
}

void FMusteringSystem::OnMusterCompleted()
{
	FPlayer& Player = Station.GetPlayer();
	Player.Skills.Mustering = FMath::Min(100.0f, Player.Skills.Mustering + MusteringSkillBumpPerCompletion);
}
