#include "Bridge/TaraSimSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Entities/Supplement.h"
#include "Entities/Vehicle.h"
#include "Entities/Fence.h"
#include "Entities/Road.h"
#include "Entities/Hand.h"
#include "Systems/MusteringSystem.h"
#include "Systems/EventSystem.h"
#include "Systems/BreedingSystem.h"
#include "Systems/WildlifeSystem.h"
#include "Systems/ProgressionSystem.h"
#include "Systems/SensorSystem.h"
#include "Entities/PestPresence.h"
#include "Entities/InvasivePresence.h"
#include "Entities/WorkMachine.h"
#include "Entities/Sensor.h"

void UTaraSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!TryLoadFromDisk())
	{
		NewGame();
	}

	// Seed the season-change tracker so the FIRST DayEnded after load doesn't
	// fire a spurious "Wet → Wet" broadcast. Done after the Station exists.
	if (Station)
	{
		LastSeason = Station->GetClock().GetSeason();
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

void UTaraSimSubsystem::AdvanceRealTime(float DeltaSeconds)
{
	if (!Station || DeltaSeconds <= 0.0f) return;
	// AdvanceRealTime returns true when the clock has hit the sleep boundary
	// (Hour >= SleepHour). Per the AdvanceMinutes contract, the clock has
	// reset Hour/Minute to WakeHour/0 but has NOT bumped DayOfYear — that's
	// our job here. TickDay's DayStarted event emits with the (still-old) day
	// before Clock.TickDay() inside it bumps DayOfYear and runs all systems
	// for the new day.
	if (Station->GetClock().AdvanceRealTime(DeltaSeconds))
	{
		Station->TickDay();
	}
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

void UTaraSimSubsystem::ResolveBadWeatherDecision(const FString& Choice)
{
	if (!Station) return;
	Station->ResolveBadWeatherDecision(Choice);
}

int32 UTaraSimSubsystem::LogBird(const FString& SpeciesId)
{
	if (!Station) return 0;
	return Station->GetWildlifeSystem().LogBird(SpeciesId).Payout;
}

bool UTaraSimSubsystem::ShootPest(const FString& PaddockId, const FString& Species)
{
	if (!Station) return false;
	EPestSpecies S;
	if (!TryParsePestSpecies(Species, S)) return false;
	return Station->GetWildlifeSystem().ShootPest(PaddockId, S);
}

bool UTaraSimSubsystem::TreatInvasive(const FString& PaddockId, const FString& Species)
{
	if (!Station) return false;
	EInvasiveSpecies S;
	if (!TryParseInvasiveSpecies(Species, S)) return false;
	return Station->GetWildlifeSystem().TreatInvasive(PaddockId, S);
}

bool UTaraSimSubsystem::BuyProperty()
{
	if (!Station) return false;
	return Station->BuyProperty();
}

float UTaraSimSubsystem::EvaluateYearNow()
{
	if (!Station) return 0.0f;
	const int32 BirdLogCount = Station->GetWildlifeSystem().GetLoggedSpeciesCount();
	return Station->GetProgressionSystem().EvaluateYear(BirdLogCount).Score;
}

bool UTaraSimSubsystem::BuyWorkMachine(int32 WorkMachineType)
{
	if (!Station) return false;
	if (WorkMachineType < 0 || WorkMachineType > 6) return false;
	return Station->BuyWorkMachine((EWorkMachineType)WorkMachineType);
}

bool UTaraSimSubsystem::InstallSensor(const FString& Kind, const FString& LocationId)
{
	if (!Station) return false;
	ESensorKind K;
	if (!TryParseSensorKind(Kind, K)) return false;
	return Station->InstallSensor(K, LocationId);
}

bool UTaraSimSubsystem::SwapSensorBattery(const FString& SensorId)
{
	if (!Station) return false;
	return Station->SwapSensorBattery(SensorId);
}

bool UTaraSimSubsystem::GradeRoad(const FString& RoadId)
{
	if (!Station) return false;
	return Station->GradeRoad(RoadId);
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

float UTaraSimSubsystem::GetCurrentDroughtSeverity() const
{
	return Station ? Station->GetEventSystem().CurrentDroughtSeverity() : 0.0f;
}

bool UTaraSimSubsystem::IsFlooding() const
{
	return Station ? Station->GetEventSystem().IsFlooding() : false;
}

float UTaraSimSubsystem::GetGrassGrowthMultiplier() const
{
	return Station ? Station->GetEventSystem().GrassGrowthMultiplier() : 1.0f;
}

bool UTaraSimSubsystem::HasPendingBadWeatherDecision() const
{
	return Station ? !Station->GetEventSystem().GetPendingBadWeatherDecision().IsEmpty() : false;
}

int32 UTaraSimSubsystem::GetPregnantHead() const
{
	if (!Station) return 0;
	int32 Total = 0;
	for (const FPregnantRecord& R : Station->GetBreedingSystem().GetPregnant())
	{
		Total += R.Count;
	}
	return Total;
}

int32 UTaraSimSubsystem::GetLoggedSpeciesCount() const
{
	return Station ? Station->GetWildlifeSystem().GetLoggedSpeciesCount() : 0;
}

float UTaraSimSubsystem::GetPestPopulation(const FString& PaddockId, const FString& Species) const
{
	if (!Station) return 0.0f;
	EPestSpecies S;
	if (!TryParsePestSpecies(Species, S)) return 0.0f;
	for (const FPestEntry& E : Station->GetWildlifeSystem().GetPests())
	{
		if (E.PaddockId == PaddockId && E.Species == S) return E.Population;
	}
	return 0.0f;
}

float UTaraSimSubsystem::GetInvasiveCoverage(const FString& PaddockId, const FString& Species) const
{
	if (!Station) return 0.0f;
	EInvasiveSpecies S;
	if (!TryParseInvasiveSpecies(Species, S)) return 0.0f;
	for (const FInvasiveEntry& E : Station->GetWildlifeSystem().GetInvasives())
	{
		if (E.PaddockId == PaddockId && E.Species == S) return E.Coverage;
	}
	return 0.0f;
}

bool UTaraSimSubsystem::CanBuyProperty() const
{
	return Station ? Station->GetProgressionSystem().CanBuyProperty() : false;
}

int32 UTaraSimSubsystem::GetDaysInBankruptcy() const
{
	return Station ? Station->GetProgressionSystem().GetDaysInBankruptcy() : 0;
}

bool UTaraSimSubsystem::IsBankrupted() const
{
	return Station ? Station->GetProgressionSystem().IsBankrupted() : false;
}

bool UTaraSimSubsystem::IsWorkMachineOwned(int32 WorkMachineType) const
{
	if (!Station) return false;
	if (WorkMachineType < 0 || WorkMachineType > 6) return false;
	const FWorkMachine* M = Station->WorkMachineByType((EWorkMachineType)WorkMachineType);
	return M ? M->bOwned : false;
}

int32 UTaraSimSubsystem::GetSensorCount() const
{
	return Station ? Station->GetSensorSystem().GetSensors().Num() : 0;
}

FString UTaraSimSubsystem::GetSensorReading(const FString& SensorId) const
{
	if (!Station) return TEXT("—");
	const FSensor* S = Station->GetSensorSystem().SensorById(SensorId);
	return S ? S->LastReading : TEXT("—");
}

int32 UTaraSimSubsystem::GetSensorBatteryDays(const FString& SensorId) const
{
	if (!Station) return 0;
	const FSensor* S = Station->GetSensorSystem().SensorById(SensorId);
	return S ? S->BatteryDays : 0;
}

float UTaraSimSubsystem::GetCohortMusterTrainedness(int32 BirthYear) const
{
	if (!Station) return 0.0f;
	for (const FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear) return C.MusterTrainedness;
	}
	return 0.0f;
}

int32 UTaraSimSubsystem::GetCohortBreederStage(int32 BirthYear) const
{
	if (!Station) return 0;
	for (const FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear) return (int32)C.BreederStage;
	}
	return 0;
}

void UTaraSimSubsystem::SetCohortMusterTrainedness(int32 BirthYear, float Value)
{
	if (!Station) return;
	for (FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear)
		{
			C.MusterTrainedness = FMath::Clamp(Value, 0.0f, 100.0f);
			return;
		}
	}
}

bool UTaraSimSubsystem::WeanCohort(int32 CalfCohortBirthYear)
{
	return Station ? Station->WeanCohort(CalfCohortBirthYear) : false;
}

bool UTaraSimSubsystem::IsCohortLactating(int32 BirthYear) const
{
	if (!Station) return false;
	for (const FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear) return C.bLactating;
	}
	return false;
}

int32 UTaraSimSubsystem::GetCohortBrandedDay(int32 BirthYear) const
{
	if (!Station) return 0;
	for (const FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear) return C.BrandedDay;
	}
	return 0;
}

bool UTaraSimSubsystem::IsCohortHorned(int32 BirthYear) const
{
	if (!Station) return false;
	for (const FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear) return C.bHorned;
	}
	return false;
}

bool UTaraSimSubsystem::RecordBrandingDay(int32 CohortBirthYear, int32 HeadProcessed, bool bDehorned)
{
	if (!Station) return false;
	bool bFound = false;
	for (FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.BirthYear == CohortBirthYear)
		{
			C.BrandedDay = Station->GetClock().DayOfYear;
			if (bDehorned) C.bHorned = false;
			bFound = true;
			break;
		}
	}
	if (!bFound) return false;

	// Player skill bump — 0.5 * processed / 10 per spec, clamped 0..100.
	const float SkillBump = 0.5f * (float)FMath::Max(0, HeadProcessed) / 10.0f;
	Station->GetPlayer().Skills.Animals = FMath::Min(100.0f,
		Station->GetPlayer().Skills.Animals + SkillBump);
	return true;
}

float UTaraSimSubsystem::GetSecondsPerInGameDay() const
{
	return Station ? Station->GetClock().RealSecondsPerInGameDay
		: FSeasonClock::DefaultRealSecondsPerInGameDay;
}

void UTaraSimSubsystem::SetSecondsPerInGameDay(float Seconds)
{
	if (!Station) return;
	// Clamp to a sane range — 60s (very fast) .. 7200s (2h/day). The
	// CORE_LOOP §7 starting-hypothesis band is 1500..2700 (25-45 min).
	Station->GetClock().RealSecondsPerInGameDay = FMath::Clamp(Seconds, 60.0f, 7200.0f);
}

int32 UTaraSimSubsystem::GetYearsToOwnerHypothesis() const
{
	return FProgressionSystem::DefaultYearsToOwnerHypothesis;
}

void UTaraSimSubsystem::WireSimEventsToDelegates()
{
	if (!Station) return;
	FEventBus& Bus = Station->GetBus();

	SubIdDayEnded = Bus.DayEnded.Subscribe([this](const FDayEndedPayload& P)
	{
		OnDayEnded.Broadcast(P);

		// Synthesise OnSeasonChanged here — see TaraSimSubsystem.h comment
		// near the FOnTaraSeasonChanged declaration. Cheaper than adding a
		// real FEventBus event for what is fundamentally a UI concern.
		if (Station)
		{
			const ESeason NewSeason = Station->GetClock().GetSeason();
			if (NewSeason != LastSeason)
			{
				const ESeason From = LastSeason;
				LastSeason = NewSeason;
				OnSeasonChanged.Broadcast(From, NewSeason);
				UE_LOG(LogTemp, Display, TEXT("[TaraSim] SeasonChanged: %s -> %s"),
					From == ESeason::Wet ? TEXT("Wet") : TEXT("Dry"),
					NewSeason == ESeason::Wet ? TEXT("Wet") : TEXT("Dry"));
			}
		}

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

	// M5 — events + breeding.
	SubIdEventStarted = Bus.EventStarted.Subscribe([this](const FEventStartedPayload& P)
	{
		OnEventStarted.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] EventStarted: type=%d severity=%.2f duration=%dd id=%s"),
			P.Type, P.Severity, P.DurationDays, *P.EventId);
	});

	SubIdEventResolved = Bus.EventResolved.Subscribe([this](const FEventResolvedPayload& P)
	{
		OnEventResolved.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] EventResolved: type=%d id=%s outcome=%s"),
			P.Type, *P.EventId, *P.Outcome);
	});

	SubIdBadWeatherDecisionRequired = Bus.BadWeatherDecisionRequired.Subscribe(
		[this](const FBadWeatherDecisionRequiredPayload& P)
	{
		OnBadWeatherDecisionRequired.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] BadWeatherDecisionRequired: %s"), *P.EventId);
	});

	SubIdBadWeatherDecisionMade = Bus.BadWeatherDecisionMade.Subscribe(
		[this](const FBadWeatherDecisionMadePayload& P)
	{
		OnBadWeatherDecisionMade.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] BadWeatherDecisionMade: %s -> %s"),
			*P.EventId, *P.Choice);
	});

	SubIdCalendarEventDue = Bus.CalendarEventDue.Subscribe([this](const FCalendarEventDuePayload& P)
	{
		OnCalendarEventDue.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] CalendarEventDue: %s"), *P.Type);
	});

	SubIdBreedingWindowOpened = Bus.BreedingWindowOpened.Subscribe(
		[this](const FBreedingWindowOpenedPayload& P)
	{
		OnBreedingWindowOpened.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] BreedingWindowOpened: cohort birthYear %d"),
			P.CohortBirthYear);
	});

	SubIdBreedingConceived = Bus.BreedingConceived.Subscribe(
		[this](const FBreedingConceivedPayload& P)
	{
		OnBreedingConceived.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] BreedingConceived: %d head pregnant (dam cohort %d)"),
			P.PregnantCount, P.CohortBirthYear);
	});

	SubIdCalfBorn = Bus.CalfBorn.Subscribe([this](const FCalfBornPayload& P)
	{
		OnCalfBorn.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] CalfBorn: %d calves (cohort %d)"),
			P.CalfCount, P.CohortBirthYear);
	});

	// M6 — wildlife + invasives.
	SubIdBirdSighted = Bus.BirdSighted.Subscribe([this](const FBirdSightedPayload& P)
	{
		OnBirdSighted.Broadcast(P);
	});

	SubIdBirdLogged = Bus.BirdLogged.Subscribe([this](const FBirdLoggedPayload& P)
	{
		OnBirdLogged.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] BirdLogged: %s first=%s payout=$%d"),
			*P.SpeciesId, P.bFirst ? TEXT("yes") : TEXT("no"), P.Payout);
	});

	SubIdPestShot = Bus.PestShot.Subscribe([this](const FPestShotPayload& P)
	{
		OnPestShot.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] PestShot: %s (paddock %s)"),
			*PestSpeciesToString((EPestSpecies)P.Species), *P.PaddockId);
	});

	SubIdInvasiveTreated = Bus.InvasiveTreated.Subscribe([this](const FInvasiveTreatedPayload& P)
	{
		OnInvasiveTreated.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] InvasiveTreated: %s @ %s coverage=%.1f"),
			*InvasiveSpeciesToString((EInvasiveSpecies)P.Species), *P.PaddockId, P.NewCoverage);
	});

	SubIdInvasiveSpread = Bus.InvasiveSpread.Subscribe([this](const FInvasiveSpreadPayload& P)
	{
		OnInvasiveSpread.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] InvasiveSpread: %s -> %s (%s)"),
			*P.FromPaddockId, *P.ToPaddockId,
			*InvasiveSpeciesToString((EInvasiveSpecies)P.Species));
	});

	// M7 — progression.
	SubIdRoleChanged = Bus.RoleChanged.Subscribe([this](const FRoleChangedPayload& P)
	{
		OnRoleChanged.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] RoleChanged: %d -> %d"), P.From, P.To);
	});

	SubIdYearEvaluated = Bus.YearEvaluated.Subscribe([this](const FYearEvaluatedPayload& P)
	{
		OnYearEvaluated.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] YearEvaluated: year %d score %.1f newRole=%d promoted=%s fired=%s"),
			P.Year, P.Score, P.NewRole,
			P.bPromoted ? TEXT("yes") : TEXT("no"),
			P.bFired ? TEXT("yes") : TEXT("no"));
	});

	SubIdPropertyPurchased = Bus.PropertyPurchased.Subscribe([this](const FPropertyPurchasedPayload& P)
	{
		OnPropertyPurchased.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] PropertyPurchased: $%d"), P.Cost);
	});

	SubIdBankruptcyDeclared = Bus.BankruptcyDeclared.Subscribe([this](const FBankruptcyDeclaredPayload& P)
	{
		OnBankruptcyDeclared.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] BankruptcyDeclared: year %d after %d days in debt"),
			P.Year, P.DaysInDebt);
	});

	// M8 — machinery + sensors + infrastructure repair.
	SubIdWorkMachinePurchased = Bus.WorkMachinePurchased.Subscribe([this](const FWorkMachinePurchasedPayload& P)
	{
		OnWorkMachinePurchased.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] WorkMachinePurchased: type=%d cost=$%d"),
			P.Type, P.Cost);
	});

	SubIdSensorInstalled = Bus.SensorInstalled.Subscribe([this](const FSensorInstalledPayload& P)
	{
		OnSensorInstalled.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] SensorInstalled: %s (kind=%d) @ %s"),
			*P.SensorId, P.Kind, *P.LocationId);
	});

	SubIdSensorBatterySwapped = Bus.SensorBatterySwapped.Subscribe([this](const FSensorBatterySwappedPayload& P)
	{
		OnSensorBatterySwapped.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] SensorBatterySwapped: %s"), *P.SensorId);
	});

	SubIdFenceRepaired = Bus.FenceRepaired.Subscribe([this](const FFenceRepairedPayload& P)
	{
		OnFenceRepaired.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] FenceRepaired: %s"), *P.FenceId);
	});

	SubIdCohortWeaned = Bus.CohortWeaned.Subscribe([this](const FCohortWeanedPayload& P)
	{
		OnCohortWeaned.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] CohortWeaned: calf cohort %d (dam %d)"),
			P.CalfCohortBirthYear, P.DamCohortBirthYear);
	});

	SubIdCohortSplit = Bus.CohortSplit.Subscribe([this](const FCohortSplitPayload& P)
	{
		OnCohortSplit.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] CohortSplit: source %d -> %d sub-cohorts"),
			P.SourceCohortBirthYear, P.NumSplits);
	});

	SubIdRoadGraded = Bus.RoadGraded.Subscribe([this](const FRoadGradedPayload& P)
	{
		OnRoadGraded.Broadcast(P);
		UE_LOG(LogTemp, Display, TEXT("[TaraSim] RoadGraded: %s"), *P.RoadId);
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
	if (SubIdEventStarted != -1)    { Bus.EventStarted.Unsubscribe(SubIdEventStarted); SubIdEventStarted = -1; }
	if (SubIdEventResolved != -1)   { Bus.EventResolved.Unsubscribe(SubIdEventResolved); SubIdEventResolved = -1; }
	if (SubIdBadWeatherDecisionRequired != -1) { Bus.BadWeatherDecisionRequired.Unsubscribe(SubIdBadWeatherDecisionRequired); SubIdBadWeatherDecisionRequired = -1; }
	if (SubIdBadWeatherDecisionMade != -1)     { Bus.BadWeatherDecisionMade.Unsubscribe(SubIdBadWeatherDecisionMade); SubIdBadWeatherDecisionMade = -1; }
	if (SubIdCalendarEventDue != -1){ Bus.CalendarEventDue.Unsubscribe(SubIdCalendarEventDue); SubIdCalendarEventDue = -1; }
	if (SubIdBreedingWindowOpened != -1) { Bus.BreedingWindowOpened.Unsubscribe(SubIdBreedingWindowOpened); SubIdBreedingWindowOpened = -1; }
	if (SubIdBreedingConceived != -1)    { Bus.BreedingConceived.Unsubscribe(SubIdBreedingConceived); SubIdBreedingConceived = -1; }
	if (SubIdCalfBorn != -1)        { Bus.CalfBorn.Unsubscribe(SubIdCalfBorn); SubIdCalfBorn = -1; }
	if (SubIdBirdSighted != -1)     { Bus.BirdSighted.Unsubscribe(SubIdBirdSighted); SubIdBirdSighted = -1; }
	if (SubIdBirdLogged != -1)      { Bus.BirdLogged.Unsubscribe(SubIdBirdLogged); SubIdBirdLogged = -1; }
	if (SubIdPestShot != -1)        { Bus.PestShot.Unsubscribe(SubIdPestShot); SubIdPestShot = -1; }
	if (SubIdInvasiveTreated != -1) { Bus.InvasiveTreated.Unsubscribe(SubIdInvasiveTreated); SubIdInvasiveTreated = -1; }
	if (SubIdInvasiveSpread != -1)  { Bus.InvasiveSpread.Unsubscribe(SubIdInvasiveSpread); SubIdInvasiveSpread = -1; }
	if (SubIdRoleChanged != -1)     { Bus.RoleChanged.Unsubscribe(SubIdRoleChanged); SubIdRoleChanged = -1; }
	if (SubIdYearEvaluated != -1)   { Bus.YearEvaluated.Unsubscribe(SubIdYearEvaluated); SubIdYearEvaluated = -1; }
	if (SubIdPropertyPurchased != -1){ Bus.PropertyPurchased.Unsubscribe(SubIdPropertyPurchased); SubIdPropertyPurchased = -1; }
	if (SubIdBankruptcyDeclared != -1){ Bus.BankruptcyDeclared.Unsubscribe(SubIdBankruptcyDeclared); SubIdBankruptcyDeclared = -1; }
	if (SubIdWorkMachinePurchased != -1){ Bus.WorkMachinePurchased.Unsubscribe(SubIdWorkMachinePurchased); SubIdWorkMachinePurchased = -1; }
	if (SubIdSensorInstalled != -1) { Bus.SensorInstalled.Unsubscribe(SubIdSensorInstalled); SubIdSensorInstalled = -1; }
	if (SubIdSensorBatterySwapped != -1){ Bus.SensorBatterySwapped.Unsubscribe(SubIdSensorBatterySwapped); SubIdSensorBatterySwapped = -1; }
	if (SubIdFenceRepaired != -1)   { Bus.FenceRepaired.Unsubscribe(SubIdFenceRepaired); SubIdFenceRepaired = -1; }
	if (SubIdRoadGraded != -1)      { Bus.RoadGraded.Unsubscribe(SubIdRoadGraded); SubIdRoadGraded = -1; }
	if (SubIdCohortWeaned != -1)    { Bus.CohortWeaned.Unsubscribe(SubIdCohortWeaned); SubIdCohortWeaned = -1; }
	if (SubIdCohortSplit != -1)     { Bus.CohortSplit.Unsubscribe(SubIdCohortSplit); SubIdCohortSplit = -1; }
}

FString UTaraSimSubsystem::GetSaveFilePath() const
{
	// Schema-versioned filename mirrors the 2D project's localStorage key pattern.
	// Bump on every milestone; mismatched older files discard cleanly via
	// FStation::FromJson returning nullptr on version mismatch.
	return FPaths::ProjectSavedDir() / TEXT("tara-save-3d-v9-phase5-prereqs.json");
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
