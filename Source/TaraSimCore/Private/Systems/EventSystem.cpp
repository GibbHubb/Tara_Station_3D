#include "Systems/EventSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Bore.h"
#include "Entities/Fence.h"
#include "Entities/Vehicle.h"
#include "Entities/Paddock.h"
#include "Entities/PriceTable.h"
#include "SeasonClock.h"

// Daily probability constants — byte-for-byte from src/sim/systems/EventSystem.ts.
namespace
{
	struct FEventProb { float Wet = 0.0f; float Dry = 0.0f; };

	FEventProb BaseProb(EStationEventType Type)
	{
		switch (Type)
		{
		case EStationEventType::Drought:   return { 0.0000f, 0.0025f };
		case EStationEventType::Flood:     return { 0.0040f, 0.0000f };
		case EStationEventType::Fire:      return { 0.0000f, 0.0015f };
		case EStationEventType::Storm:     return { 0.0050f, 0.0010f };
		case EStationEventType::Breakdown: return { 0.0020f, 0.0030f };
		default: return { 0.0f, 0.0f };
		}
	}

	constexpr int32 RodeoDay = 120;
	constexpr int32 DraftDay = 220;
	constexpr int32 MaxConcurrentRandom = 2;
}

int32 FEventSystem::EventIdCounter = 1;

FEventSystem::FEventSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FEventSystem::TickDay()
{
	const int32 Day = Station.GetClock().DayOfYear;
	const ESeason Season = Station.GetClock().GetSeason();

	// Calendar events fire once per year on their fixed day.
	if (Day == RodeoDay)
	{
		FCalendarEventDuePayload P; P.Type = TEXT("rodeo");
		Bus.CalendarEventDue.Emit(P);
	}
	if (Day == DraftDay)
	{
		FCalendarEventDuePayload P; P.Type = TEXT("draft");
		Bus.CalendarEventDue.Emit(P);
	}

	// Random-event roll — at most MaxConcurrentRandom active.
	int32 ActiveRandomCount = 0;
	for (const FStationEvent& E : Active)
	{
		if (E.Kind == EStationEventKind::Random && !E.bResolved) ActiveRandomCount++;
	}
	if (ActiveRandomCount < MaxConcurrentRandom)
	{
		static const EStationEventType RollTypes[] = {
			EStationEventType::Drought,
			EStationEventType::Flood,
			EStationEventType::Fire,
			EStationEventType::Storm,
			EStationEventType::Breakdown,
		};
		for (EStationEventType Type : RollTypes)
		{
			// Skip if a non-resolved event of this type already exists.
			bool bExists = false;
			for (const FStationEvent& E : Active)
			{
				if (E.Type == Type && !E.bResolved) { bExists = true; break; }
			}
			if (bExists) continue;

			const FEventProb Prob = BaseProb(Type);
			const float P = (Season == ESeason::Wet) ? Prob.Wet : Prob.Dry;
			if (FMath::FRand() < P)
			{
				FireRandomEvent(Type, Day);
				break;  // one new event per day
			}
		}
	}

	// Tick down active events; apply ongoing effects; resolve on duration end.
	for (FStationEvent& E : Active)
	{
		if (E.bResolved) continue;
		ApplyOngoingEffect(E);
		E.DurationDays -= 1;
		if (E.DurationDays <= 0)
		{
			E.bResolved = true;
			if (E.Outcome.IsEmpty()) E.Outcome = TEXT("expired");

			FEventResolvedPayload P;
			P.Type = (int32)E.Type;
			P.EventId = E.Id;
			P.Outcome = E.Outcome;
			Bus.EventResolved.Emit(P);
		}
	}

	// Drought modulates the price curve (read by EconomySystem.SellCattle).
	Station.GetPrices().DroughtSeverity = CurrentDroughtSeverity();

	// Move resolved events to history.
	TArray<FStationEvent> StillActive;
	StillActive.Reserve(Active.Num());
	for (const FStationEvent& E : Active)
	{
		if (E.bResolved) History.Add(E);
		else StillActive.Add(E);
	}
	Active = MoveTemp(StillActive);
}

void FEventSystem::FireRandomEvent(EStationEventType Type, int32 Day)
{
	FStationEventConfig Cfg;
	Cfg.Id = FString::Printf(TEXT("evt-%d"), EventIdCounter++);
	Cfg.Type = Type;
	Cfg.Kind = EStationEventKind::Random;
	Cfg.Severity = FMath::Min(1.0f, 0.3f + FMath::FRand() * 0.6f);
	Cfg.StartDay = Day;

	switch (Type)
	{
	case EStationEventType::Drought:   Cfg.DurationDays = 30 + FMath::RandRange(0, 59); break;
	case EStationEventType::Flood:     Cfg.DurationDays = 4 + FMath::RandRange(0, 5); break;
	case EStationEventType::Fire:      Cfg.DurationDays = 1 + FMath::RandRange(0, 2); break;
	case EStationEventType::Storm:     Cfg.DurationDays = 2 + FMath::RandRange(0, 3); break;
	case EStationEventType::Breakdown: Cfg.DurationDays = 1; break;
	default: Cfg.DurationDays = 1; break;
	}

	FStationEvent E(Cfg);
	Active.Add(E);

	FEventStartedPayload P;
	P.Type = (int32)Type;
	P.Severity = Cfg.Severity;
	P.DurationDays = Cfg.DurationDays;
	P.EventId = Cfg.Id;
	Bus.EventStarted.Emit(P);

	// First-impact reactions — mirror 2D side-effects.
	if (Type == EStationEventType::Drought)
	{
		PendingBadWeatherDecision = Cfg.Id;
		FBadWeatherDecisionRequiredPayload Q; Q.EventId = Cfg.Id;
		Bus.BadWeatherDecisionRequired.Emit(Q);
	}
	else if (Type == EStationEventType::Flood)
	{
		// Damage a random bore + a random fence.
		if (Station.GetBores().Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, Station.GetBores().Num() - 1);
			FBore& B = Station.GetBores()[Idx];
			if (B.Status == EBoreStatus::Working)
			{
				B.Condition = FBore::ClampCondition(B.Condition - 30.0f * Cfg.Severity);
			}
		}
		if (Station.GetFences().Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, Station.GetFences().Num() - 1);
			FFence& F = Station.GetFences()[Idx];
			F.Integrity = FFence::ClampIntegrity(F.Integrity - 50.0f * Cfg.Severity);
		}
	}
	else if (Type == EStationEventType::Fire)
	{
		// Pull grass down in a random paddock.
		if (Station.GetPaddocks().Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, Station.GetPaddocks().Num() - 1);
			FPaddock& Pa = Station.GetPaddocks()[Idx];
			Pa.GrassKgPerHa = FMath::Max(0.0f, Pa.GrassKgPerHa - 1500.0f * Cfg.Severity);
		}
	}
	else if (Type == EStationEventType::Breakdown)
	{
		// Damage a random owned non-foot vehicle.
		TArray<FVehicle*> OwnedVehicles;
		for (FVehicle& V : Station.GetVehicles())
		{
			if (V.bOwned && V.Type != EVehicleType::Foot) OwnedVehicles.Add(&V);
		}
		if (OwnedVehicles.Num() > 0)
		{
			const int32 Idx = FMath::RandRange(0, OwnedVehicles.Num() - 1);
			OwnedVehicles[Idx]->Condition = FMath::Max(0.0f, OwnedVehicles[Idx]->Condition - 40.0f * Cfg.Severity);
		}
	}
}

void FEventSystem::ApplyOngoingEffect(FStationEvent& E)
{
	if (E.Type == EStationEventType::Storm)
	{
		// Storm slowly damages roads.
		for (FRoad& R : Station.GetRoads())
		{
			R.GradeQuality = FMath::Max(0.0f, R.GradeQuality - 0.4f * E.Severity);
		}
	}
	// Drought ongoing: handled via prices.DroughtSeverity + grass-growth multiplier.
	// Fire / Flood: first-impact only (already applied).
	// Breakdown: 1-day duration; first-impact only.
}

float FEventSystem::CurrentDroughtSeverity() const
{
	float Sum = 0.0f;
	for (const FStationEvent& E : Active)
	{
		if (E.Type == EStationEventType::Drought && !E.bResolved) Sum += E.Severity;
	}
	return FMath::Min(1.0f, Sum);
}

bool FEventSystem::IsFlooding() const
{
	for (const FStationEvent& E : Active)
	{
		if (E.Type == EStationEventType::Flood && !E.bResolved) return true;
	}
	return false;
}

float FEventSystem::GrassGrowthMultiplier() const
{
	const float D = CurrentDroughtSeverity();
	return FMath::Max(0.0f, 1.0f - D * 0.7f);
}

void FEventSystem::ResolveBadWeatherDecision(const FString& Choice)
{
	if (PendingBadWeatherDecision.IsEmpty()) return;
	for (FStationEvent& E : Active)
	{
		if (E.Id == PendingBadWeatherDecision)
		{
			E.Outcome = Choice;
			FBadWeatherDecisionMadePayload P;
			P.EventId = E.Id;
			P.Choice = Choice;
			Bus.BadWeatherDecisionMade.Emit(P);
			break;
		}
	}
	PendingBadWeatherDecision.Empty();
}

FString FEventSystem::SerializeJson() const
{
	auto SerArr = [](const TArray<FStationEvent>& Arr) -> FString
	{
		FString S = TEXT("[");
		for (int32 I = 0; I < Arr.Num(); I++)
		{
			if (I > 0) S += TEXT(",");
			S += Arr[I].SerializeJson();
		}
		S += TEXT("]");
		return S;
	};
	return FString::Printf(
		TEXT("{\"active\":%s,\"history\":%s,\"pendingBadWeatherDecision\":\"%s\",\"eventIdCounter\":%d}"),
		*SerArr(Active), *SerArr(History), *PendingBadWeatherDecision, EventIdCounter);
}

void FEventSystem::LoadFromJson(const FString& Json)
{
	Active.Empty();
	History.Empty();
	PendingBadWeatherDecision.Empty();

	auto ParseArr = [&Json](const FString& ArrayToken, TArray<FStationEvent>& Out)
	{
		const int32 ArrIdx = Json.Find(ArrayToken);
		if (ArrIdx == INDEX_NONE) return;
		int32 Depth = 0;
		int32 ObjStart = INDEX_NONE;
		for (int32 P = ArrIdx + ArrayToken.Len(); P < Json.Len(); P++)
		{
			const TCHAR Ch = Json[P];
			if (Ch == '{')
			{
				if (Depth == 0) ObjStart = P;
				Depth++;
			}
			else if (Ch == '}')
			{
				Depth--;
				if (Depth == 0 && ObjStart != INDEX_NONE)
				{
					Out.Add(FStationEvent::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0) break;
		}
	};
	ParseArr(TEXT("\"active\":["), Active);
	ParseArr(TEXT("\"history\":["), History);

	const FString Tok = TEXT("\"pendingBadWeatherDecision\":\"");
	const int32 PIdx = Json.Find(Tok);
	if (PIdx != INDEX_NONE)
	{
		const int32 Start = PIdx + Tok.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) PendingBadWeatherDecision = Json.Mid(Start, End - Start);
	}

	const FString CTok = TEXT("\"eventIdCounter\":");
	const int32 CIdx = Json.Find(CTok);
	if (CIdx != INDEX_NONE)
	{
		int32 P = CIdx + CTok.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		EventIdCounter = FMath::Max(FCString::Atoi(*Json.Mid(Start, P - Start)), 1);
	}
}
