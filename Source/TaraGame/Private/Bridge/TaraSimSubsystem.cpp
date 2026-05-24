#include "Bridge/TaraSimSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Entities/Supplement.h"
#include "Entities/Vehicle.h"
#include "Entities/Fence.h"
#include "Entities/Road.h"
#include "Entities/Hand.h"
#include "Systems/MusteringSystem.h"

void UTaraSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!TryLoadFromDisk())
	{
		NewGame();
	}

	WireSimEventsToDelegates();

	UE_LOG(LogTemp, Display, TEXT("[TaraSim] Initialized. Year %d Day %d %s — herd %d head"),
		Station->GetClock().Year, Station->GetClock().DayOfYear,
		*Station->GetClock().FormatTime(), Station->GetHerd().GetSize());
}

void UTaraSimSubsystem::Deinitialize()
{
	UnwireSimEvents();
	WriteSaveToDisk();
	Station.Reset();
	Super::Deinitialize();
}

void UTaraSimSubsystem::TickDay()
{
	if (!Station) return;
	Station->TickDay();
	// Autosave on every day-tick — mirrors 2D Save-on-DayEnded.
	// The DayEnded event handler also writes; this is belt-and-braces during Phase 0.
}

void UTaraSimSubsystem::SaveNow()
{
	WriteSaveToDisk();
}

void UTaraSimSubsystem::NewGame()
{
	UnwireSimEvents();
	Station = MakeUnique<FStation>();
	WireSimEventsToDelegates();
}

bool UTaraSimSubsystem::CheckBore(const FString& BoreId)
{
	if (!Station) return false;
	const auto R = Station->CheckBore(BoreId);
	return R.bFixed;
}

bool UTaraSimSubsystem::BuySupplement(int32 SupplementKind)
{
	if (!Station) return false;
	if (SupplementKind < 0 || SupplementKind > 2) return false;
	return Station->GetEconomySystem().BuySupplement((ESupplementKind)SupplementKind);
}

int32 UTaraSimSubsystem::SellCattle(int32 Count)
{
	if (!Station) return 0;
	const auto R = Station->GetEconomySystem().SellCattle(Count, Station->GetClock().GetSeason());
	return R.bSuccess ? R.Amount : 0;
}

bool UTaraSimSubsystem::StartMuster(
	const FString& ToPaddockId,
	int32 VehicleType,
	const TArray<FString>& HandIds,
	int32 PlannedDays,
	int32 FuelCost,
	int32 BreakawayPending)
{
	if (!Station) return false;
	if (VehicleType < 0 || VehicleType > 5) return false;
	return Station->StartMuster(
		ToPaddockId,
		(EVehicleType)VehicleType,
		HandIds,
		PlannedDays,
		FuelCost,
		BreakawayPending);
}

bool UTaraSimSubsystem::CancelMuster()
{
	if (!Station) return false;
	const auto R = Station->GetMusteringSystem().CancelMuster();
	return R.bWasInTransit;
}

bool UTaraSimSubsystem::BuyVehicle(int32 VehicleType)
{
	if (!Station) return false;
	if (VehicleType < 0 || VehicleType > 5) return false;
	return Station->BuyVehicle((EVehicleType)VehicleType);
}

bool UTaraSimSubsystem::HireHand(const FString& HandId)
{
	if (!Station) return false;
	return Station->HireHand(HandId);
}

bool UTaraSimSubsystem::FireHand(const FString& HandId)
{
	if (!Station) return false;
	return Station->FireHand(HandId);
}

bool UTaraSimSubsystem::RepairFence(const FString& FenceId)
{
	if (!Station) return false;
	return Station->RepairFence(FenceId);
}

int32 UTaraSimSubsystem::GetYear() const
{
	return Station ? Station->GetClock().Year : 0;
}

int32 UTaraSimSubsystem::GetDayOfYear() const
{
	return Station ? Station->GetClock().DayOfYear : 0;
}

FString UTaraSimSubsystem::GetFormattedTime() const
{
	return Station ? Station->GetClock().FormatTime() : TEXT("--:--");
}

int32 UTaraSimSubsystem::GetHerdSize() const
{
	return Station ? Station->GetHerd().GetSize() : 0;
}

float UTaraSimSubsystem::GetHerdConditionMean() const
{
	return Station ? Station->GetHerd().GetConditionMean() : 0.0f;
}

float UTaraSimSubsystem::GetPaddockGrassKgPerHa(const FString& PaddockId) const
{
	if (!Station) return 0.0f;
	const FPaddock* P = Station->PaddockById(PaddockId);
	return P ? P->GrassKgPerHa : 0.0f;
}

int32 UTaraSimSubsystem::GetPaddockWaterAccess(const FString& PaddockId) const
{
	if (!Station) return 0;
	return (int32)Station->GetWaterAccess(PaddockId);
}

int32 UTaraSimSubsystem::GetPlayerCash() const
{
	return Station ? Station->GetPlayer().Cash : 0;
}

int32 UTaraSimSubsystem::GetPlayerRole() const
{
	return Station ? (int32)Station->GetPlayer().Role : 0;
}

int32 UTaraSimSubsystem::GetCurrentCattlePrice() const
{
	if (!Station) return 0;
	return Station->GetPrices().CurrentCattlePrice(Station->GetClock().GetSeason());
}

bool UTaraSimSubsystem::IsHerdInTransit() const
{
	return Station ? Station->GetHerd().IsInTransit() : false;
}

int32 UTaraSimSubsystem::GetMusterDaysRemaining() const
{
	if (!Station) return 0;
	return Station->GetHerd().GetMuster().DaysRemaining;
}

FString UTaraSimSubsystem::GetMusterDestinationPaddock() const
{
	if (!Station) return FString();
	return Station->GetHerd().GetMuster().ToPaddockId;
}

bool UTaraSimSubsystem::IsVehicleOwned(int32 VehicleType) const
{
	if (!Station) return false;
	if (VehicleType < 0 || VehicleType > 5) return false;
	const FVehicle* V = Station->VehicleByType((EVehicleType)VehicleType);
	return V ? V->bOwned : false;
}

float UTaraSimSubsystem::GetVehicleCondition(int32 VehicleType) const
{
	if (!Station) return 0.0f;
	if (VehicleType < 0 || VehicleType > 5) return 0.0f;
	const FVehicle* V = Station->VehicleByType((EVehicleType)VehicleType);
	return V ? V->Condition : 0.0f;
}

float UTaraSimSubsystem::GetFenceIntegrity(const FString& FenceId) const
{
	if (!Station) return 0.0f;
	for (const FFence& F : Station->GetFences())
	{
		if (F.Id == FenceId) return F.Integrity;
	}
	return 0.0f;
}

float UTaraSimSubsystem::GetRoadGradeQuality(const FString& RoadId) const
{
	if (!Station) return 0.0f;
	for (const FRoad& R : Station->GetRoads())
	{
		if (R.Id == RoadId) return R.GradeQuality;
	}
	return 0.0f;
}

bool UTaraSimSubsystem::IsHandHired(const FString& HandId) const
{
	if (!Station) return false;
	for (const FHand& H : Station->GetHands())
	{
		if (H.Id == HandId) return H.bHired;
	}
	return false;
}

void UTaraSimSubsystem::WireSimEventsToDelegates()
{
	if (!Station) return;
	FEventBus& Bus = Station->GetBus();

	SubIdDayEnded = Bus.DayEnded.Subscribe([this](const FDayEndedPayload& P)
	{
		OnDayEnded.Broadcast(P);
		// Autosave on every DayEnded.
		WriteSaveToDisk();
	});

	SubIdCattleDied = Bus.CattleDied.Subscribe([this](const FCattleDiedPayload& P)
	{
		OnCattleDied.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] CattleDied: %d head — %s"), P.Count, *P.Cause);
	});

	SubIdHerdCondition = Bus.HerdConditionChanged.Subscribe([this](const FHerdConditionChangedPayload& P)
	{
		OnHerdConditionChanged.Broadcast(P);
	});

	// M2 — water events.
	SubIdWaterLost = Bus.WaterAccessLost.Subscribe([this](const FWaterAccessLostPayload& P)
	{
		OnWaterAccessLost.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] WaterAccessLost: paddock %s"), *P.PaddockId);
	});

	SubIdWaterRestored = Bus.WaterAccessRestored.Subscribe([this](const FWaterAccessRestoredPayload& P)
	{
		OnWaterAccessRestored.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] WaterAccessRestored: paddock %s"), *P.PaddockId);
	});

	SubIdBoreFailed = Bus.BoreFailed.Subscribe([this](const FBoreFailedPayload& P)
	{
		OnBoreFailed.Broadcast(P);
		const FString Joined = FString::Join(P.PaddockIds, TEXT("+"));
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] BoreFailed: %s (paddocks: %s)"), *P.BoreId, *Joined);
	});

	SubIdBoreRepaired = Bus.BoreRepaired.Subscribe([this](const FBoreRepairedPayload& P)
	{
		OnBoreRepaired.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] BoreRepaired: %s"), *P.BoreId);
	});

	// M3 — economy events.
	SubIdMoneyChanged = Bus.MoneyChanged.Subscribe([this](const FMoneyChangedPayload& P)
	{
		OnMoneyChanged.Broadcast(P);
	});

	SubIdCattleSold = Bus.CattleSold.Subscribe([this](const FCattleSoldPayload& P)
	{
		OnCattleSold.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] CattleSold: %d head @ $%d = $%d"),
			P.Count, P.PricePerHead, P.Total);
	});

	SubIdSupplementOrdered = Bus.SupplementOrdered.Subscribe([this](const FSupplementOrderedPayload& P)
	{
		OnSupplementOrdered.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] SupplementOrdered: %s $%d (arrives %dd)"),
			*P.Kind, P.Cost, P.ArrivesInDays);
	});

	SubIdYearEnded = Bus.YearEnded.Subscribe([this](const FYearEndedPayload& P)
	{
		OnYearEnded.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] YearEnded: %d"), P.Year);
	});

	// M4a — muster + infrastructure events.
	SubIdMusterStarted = Bus.MusterStarted.Subscribe([this](const FMusterStartedPayload& P)
	{
		OnMusterStarted.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] MusterStarted: %s -> %s (%dd, $%d fuel)"),
			*P.FromPaddockId, *P.ToPaddockId, P.PlannedDays, P.FuelCost);
	});

	SubIdMusterCompleted = Bus.MusterCompleted.Subscribe([this](const FMusterCompletedPayload& P)
	{
		OnMusterCompleted.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] MusterCompleted: arrived %s (%d breakaway)"),
			*P.ToPaddockId, P.BreakawayHead);
	});

	SubIdMusterCancelled = Bus.MusterCancelled.Subscribe([this](const FMusterCancelledPayload& P)
	{
		OnMusterCancelled.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] MusterCancelled: %s after %dd ($%d refund)"),
			*P.ToPaddockId, P.DaysSpent, P.Refund);
	});

	SubIdBreakaway = Bus.Breakaway.Subscribe([this](const FBreakawayPayload& P)
	{
		OnBreakaway.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] Breakaway: %d head at %s"), P.Count, *P.PaddockId);
	});

	SubIdFenceBroken = Bus.FenceBroken.Subscribe([this](const FFenceBrokenPayload& P)
	{
		OnFenceBroken.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] FenceBroken: %s (%s<->%s)"),
			*P.FenceId, *P.PaddockA, *P.PaddockB);
	});

	SubIdCattleDrifted = Bus.CattleDrifted.Subscribe([this](const FCattleDriftedPayload& P)
	{
		OnCattleDrifted.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] CattleDrifted: %d head %s -> %s"),
			P.Count, *P.FromPaddockId, *P.ToPaddockId);
	});

	SubIdVehiclePurchased = Bus.VehiclePurchased.Subscribe([this](const FVehiclePurchasedPayload& P)
	{
		OnVehiclePurchased.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] VehiclePurchased: type=%d cost=$%d"),
			P.VehicleType, P.Cost);
	});

	SubIdHandHired = Bus.HandHired.Subscribe([this](const FHandHiredPayload& P)
	{
		OnHandHired.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] HandHired: %s"), *P.HandId);
	});

	SubIdHandFired = Bus.HandFired.Subscribe([this](const FHandFiredPayload& P)
	{
		OnHandFired.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] HandFired: %s"), *P.HandId);
	});
}

void UTaraSimSubsystem::UnwireSimEvents()
{
	if (!Station) return;
	FEventBus& Bus = Station->GetBus();
	if (SubIdDayEnded != -1)      { Bus.DayEnded.Unsubscribe(SubIdDayEnded); SubIdDayEnded = -1; }
	if (SubIdCattleDied != -1)    { Bus.CattleDied.Unsubscribe(SubIdCattleDied); SubIdCattleDied = -1; }
	if (SubIdHerdCondition != -1) { Bus.HerdConditionChanged.Unsubscribe(SubIdHerdCondition); SubIdHerdCondition = -1; }
	if (SubIdWaterLost != -1)     { Bus.WaterAccessLost.Unsubscribe(SubIdWaterLost); SubIdWaterLost = -1; }
	if (SubIdWaterRestored != -1) { Bus.WaterAccessRestored.Unsubscribe(SubIdWaterRestored); SubIdWaterRestored = -1; }
	if (SubIdBoreFailed != -1)    { Bus.BoreFailed.Unsubscribe(SubIdBoreFailed); SubIdBoreFailed = -1; }
	if (SubIdBoreRepaired != -1)  { Bus.BoreRepaired.Unsubscribe(SubIdBoreRepaired); SubIdBoreRepaired = -1; }
	if (SubIdMoneyChanged != -1)  { Bus.MoneyChanged.Unsubscribe(SubIdMoneyChanged); SubIdMoneyChanged = -1; }
	if (SubIdCattleSold != -1)    { Bus.CattleSold.Unsubscribe(SubIdCattleSold); SubIdCattleSold = -1; }
	if (SubIdSupplementOrdered != -1) { Bus.SupplementOrdered.Unsubscribe(SubIdSupplementOrdered); SubIdSupplementOrdered = -1; }
	if (SubIdYearEnded != -1)     { Bus.YearEnded.Unsubscribe(SubIdYearEnded); SubIdYearEnded = -1; }
	if (SubIdMusterStarted != -1)   { Bus.MusterStarted.Unsubscribe(SubIdMusterStarted); SubIdMusterStarted = -1; }
	if (SubIdMusterCompleted != -1) { Bus.MusterCompleted.Unsubscribe(SubIdMusterCompleted); SubIdMusterCompleted = -1; }
	if (SubIdMusterCancelled != -1) { Bus.MusterCancelled.Unsubscribe(SubIdMusterCancelled); SubIdMusterCancelled = -1; }
	if (SubIdBreakaway != -1)       { Bus.Breakaway.Unsubscribe(SubIdBreakaway); SubIdBreakaway = -1; }
	if (SubIdFenceBroken != -1)     { Bus.FenceBroken.Unsubscribe(SubIdFenceBroken); SubIdFenceBroken = -1; }
	if (SubIdCattleDrifted != -1)   { Bus.CattleDrifted.Unsubscribe(SubIdCattleDrifted); SubIdCattleDrifted = -1; }
	if (SubIdVehiclePurchased != -1){ Bus.VehiclePurchased.Unsubscribe(SubIdVehiclePurchased); SubIdVehiclePurchased = -1; }
	if (SubIdHandHired != -1)       { Bus.HandHired.Unsubscribe(SubIdHandHired); SubIdHandHired = -1; }
	if (SubIdHandFired != -1)       { Bus.HandFired.Unsubscribe(SubIdHandFired); SubIdHandFired = -1; }
}

FString UTaraSimSubsystem::GetSaveFilePath() const
{
	// Schema-versioned filename mirrors the 2D project's localStorage key pattern.
	// Bump on every milestone; mismatched older files discard cleanly via
	// FStation::FromJson returning nullptr on version mismatch.
	return FPaths::ProjectSavedDir() / TEXT("tara-save-3d-v4-m4a.json");
}

bool UTaraSimSubsystem::TryLoadFromDisk()
{
	const FString Path = GetSaveFilePath();
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		return false;
	}

	TUniquePtr<FStation> Loaded = FStation::FromJson(Json);
	if (!Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] Save schema mismatch or parse failure — discarding."));
		return false;
	}

	Station = MoveTemp(Loaded);
	return true;
}

void UTaraSimSubsystem::WriteSaveToDisk() const
{
	if (!Station) return;
	const FString Path = GetSaveFilePath();
	const FString Json = Station->SerializeJson();
	if (!FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] Save failed: %s"), *Path);
	}
}
