#include "Systems/SensorSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Paddock.h"
#include "Entities/Bore.h"
#include "SeasonClock.h"

FSensorSystem::FSensorSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FSensorSystem::TickDay()
{
	for (FSensor& S : Sensors)
	{
		if (!S.bInstalled) continue;
		S.BatteryDays = FMath::Max(0, S.BatteryDays - 1);
		if (S.BatteryDays == 0)
		{
			S.LastReading = TEXT("battery flat");
			continue;
		}

		switch (S.Kind)
		{
		case ESensorKind::RainGauge:
		{
			const ESeason Season = Station.GetClock().GetSeason();
			const float Mm = (Season == ESeason::Wet)
				? FMath::FRand() * 12.0f
				: FMath::FRand() * 1.5f;
			S.LastReading = FString::Printf(TEXT("%.1f mm"), Mm);
			break;
		}
		case ESensorKind::TankFlow:
		{
			const int32 Pct = FMath::RoundToInt(70.0f + FMath::FRand() * 30.0f);
			S.LastReading = FString::Printf(TEXT("%d%% full"), Pct);
			break;
		}
		case ESensorKind::BorePressure:
		{
			const FBore* Bore = nullptr;
			for (const FBore& B : Station.GetBores())
			{
				if (B.Id == S.LocationId) { Bore = &B; break; }
			}
			if (Bore)
			{
				FString Tail = (Bore->Status == EBoreStatus::Failed) ? TEXT(" (FAILED)") : TEXT("");
				if (Bore->Condition < 30.0f && Bore->Status == EBoreStatus::Working)
				{
					Tail = TEXT(" low");
				}
				S.LastReading = FString::Printf(TEXT("cond %d%%%s"),
					FMath::RoundToInt(Bore->Condition), *Tail);
			}
			break;
		}
		}
	}
}

bool FSensorSystem::InstallSensor(ESensorKind Kind, const FString& LocationId)
{
	FSensorConfig Cfg;
	Cfg.Id = FString::Printf(TEXT("sensor-%s-%s-%d"),
		*SensorKindToString(Kind), *LocationId, InstallCounter++);
	Cfg.Kind = Kind;
	Cfg.LocationId = LocationId;
	Cfg.bInstalled = true;
	Cfg.BatteryDays = DefaultBatteryDays;
	Sensors.Add(FSensor(Cfg));

	FSensorInstalledPayload P;
	P.SensorId = Cfg.Id;
	P.Kind = (int32)Kind;
	P.LocationId = LocationId;
	Bus.SensorInstalled.Emit(P);
	return true;
}

bool FSensorSystem::SwapBattery(const FString& SensorId)
{
	for (FSensor& S : Sensors)
	{
		if (S.Id == SensorId)
		{
			S.BatteryDays = DefaultBatteryDays;
			FSensorBatterySwappedPayload P;
			P.SensorId = SensorId;
			Bus.SensorBatterySwapped.Emit(P);
			return true;
		}
	}
	return false;
}

const FSensor* FSensorSystem::SensorById(const FString& Id) const
{
	for (const FSensor& S : Sensors)
	{
		if (S.Id == Id) return &S;
	}
	return nullptr;
}

FString FSensorSystem::SerializeJson() const
{
	FString SensorsJson = TEXT("[");
	for (int32 I = 0; I < Sensors.Num(); I++)
	{
		if (I > 0) SensorsJson += TEXT(",");
		SensorsJson += Sensors[I].SerializeJson();
	}
	SensorsJson += TEXT("]");
	return FString::Printf(
		TEXT("{\"sensors\":%s,\"installCounter\":%d}"),
		*SensorsJson, InstallCounter);
}

void FSensorSystem::LoadFromJson(const FString& Json)
{
	Sensors.Empty();
	const FString Tok = TEXT("\"sensors\":[");
	const int32 ArrIdx = Json.Find(Tok);
	if (ArrIdx != INDEX_NONE)
	{
		int32 Depth = 0;
		int32 ObjStart = INDEX_NONE;
		for (int32 P = ArrIdx + Tok.Len(); P < Json.Len(); P++)
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
					Sensors.Add(FSensor::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0) break;
		}
	}

	const FString CTok = TEXT("\"installCounter\":");
	const int32 CIdx = Json.Find(CTok);
	if (CIdx != INDEX_NONE)
	{
		int32 P = CIdx + CTok.Len();
		int32 S = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		InstallCounter = FMath::Max(1, FCString::Atoi(*Json.Mid(S, P - S)));
	}
}
