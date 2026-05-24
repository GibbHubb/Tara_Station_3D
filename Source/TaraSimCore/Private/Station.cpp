#include "Station.h"

FStation::FStation()
{
	// Default M1 starter — three paddocks, one 8-head cohort in Home.
	// Mirrors BootScene "New Game" defaults from the 2D project.
	FPaddockConfig HomeCfg;
	HomeCfg.Id = TEXT("A");
	HomeCfg.Name = TEXT("Home Paddock");
	HomeCfg.AreaHa = 200.0f;
	HomeCfg.StartingGrassKgPerHa = 2500.0f;
	Paddocks.Add(FPaddock(HomeCfg));

	FPaddockConfig RiverCfg;
	RiverCfg.Id = TEXT("B");
	RiverCfg.Name = TEXT("River Paddock");
	RiverCfg.AreaHa = 180.0f;
	RiverCfg.StartingGrassKgPerHa = 1800.0f;
	Paddocks.Add(FPaddock(RiverCfg));

	FPaddockConfig BackCfg;
	BackCfg.Id = TEXT("C");
	BackCfg.Name = TEXT("Back Paddock");
	BackCfg.AreaHa = 250.0f;
	BackCfg.StartingGrassKgPerHa = 2900.0f;
	Paddocks.Add(FPaddock(BackCfg));

	FCattleCohortConfig CohortCfg;
	CohortCfg.BirthYear = 0;
	CohortCfg.Count = 8;
	Herd = FHerd(FCattleCohort(CohortCfg), TEXT("A"));

	// Systems — construct in dependency order.
	ConditionSys = MakeUnique<FConditionSystem>(Bus, *this);
}

void FStation::TickDay()
{
	{
		FDayStartedPayload P;
		P.Day = Clock.DayOfYear;
		P.Year = Clock.Year;
		Bus.DayStarted.Emit(P);
	}

	Clock.TickDay();

	for (FPaddock& P : Paddocks)
	{
		const int32 HerdHere = Herd.HerdInPaddock(P.Id);
		P.SimulateDay(Clock.GetSeason(), HerdHere, 1.0f);
	}

	ConditionSys->TickDay();

	{
		FDayEndedPayload P;
		P.Day = Clock.DayOfYear;
		P.Year = Clock.Year;
		Bus.DayEnded.Emit(P);
	}
}

const FPaddock* FStation::PaddockById(const FString& Id) const
{
	for (const FPaddock& P : Paddocks)
	{
		if (P.Id == Id) return &P;
	}
	return nullptr;
}

FPaddock* FStation::PaddockById(const FString& Id)
{
	for (FPaddock& P : Paddocks)
	{
		if (P.Id == Id) return &P;
	}
	return nullptr;
}

FString FStation::SerializeJson() const
{
	FString PaddocksJson = TEXT("[");
	for (int32 I = 0; I < Paddocks.Num(); I++)
	{
		if (I > 0) PaddocksJson += TEXT(",");
		PaddocksJson += Paddocks[I].SerializeJson();
	}
	PaddocksJson += TEXT("]");

	return FString::Printf(
		TEXT("{\"schemaVersion\":\"%s\",\"clock\":%s,\"paddocks\":%s,\"herd\":%s}"),
		TARA_SIM_SAVE_SCHEMA_VERSION,
		*Clock.SerializeJson(),
		*PaddocksJson,
		*Herd.SerializeJson());
}

TUniquePtr<FStation> FStation::FromJson(const FString& Json)
{
	// Schema check first.
	const FString SchemaToken = TEXT("\"schemaVersion\":\"");
	const int32 SchemaIdx = Json.Find(SchemaToken);
	if (SchemaIdx == INDEX_NONE) return nullptr;
	const int32 SchemaStart = SchemaIdx + SchemaToken.Len();
	const int32 SchemaEnd = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, SchemaStart);
	if (SchemaEnd == INDEX_NONE) return nullptr;
	const FString SchemaVersion = Json.Mid(SchemaStart, SchemaEnd - SchemaStart);
	if (SchemaVersion != TARA_SIM_SAVE_SCHEMA_VERSION)
	{
		// Schema mismatch — caller treats as no save.
		return nullptr;
	}

	auto Station = MakeUnique<FStation>();

	// Pull clock substring.
	const FString ClockToken = TEXT("\"clock\":");
	const int32 ClockIdx = Json.Find(ClockToken);
	if (ClockIdx != INDEX_NONE)
	{
		int32 Start = ClockIdx + ClockToken.Len();
		int32 Depth = 0;
		int32 End = Start;
		for (; End < Json.Len(); End++)
		{
			if (Json[End] == '{') Depth++;
			else if (Json[End] == '}')
			{
				Depth--;
				if (Depth == 0) { End++; break; }
			}
		}
		Station->Clock = FSeasonClock::FromJson(Json.Mid(Start, End - Start));
	}

	// Herd.
	const FString HerdToken = TEXT("\"herd\":");
	const int32 HerdIdx = Json.Find(HerdToken);
	if (HerdIdx != INDEX_NONE)
	{
		int32 Start = HerdIdx + HerdToken.Len();
		int32 Depth = 0;
		int32 End = Start;
		for (; End < Json.Len(); End++)
		{
			if (Json[End] == '{') Depth++;
			else if (Json[End] == '}')
			{
				Depth--;
				if (Depth == 0) { End++; break; }
			}
		}
		Station->Herd = FHerd::FromJson(Json.Mid(Start, End - Start));
	}

	// Paddocks. Same ad-hoc parser as Herd's cohorts.
	const FString PdkToken = TEXT("\"paddocks\":[");
	const int32 PdkIdx = Json.Find(PdkToken);
	if (PdkIdx != INDEX_NONE)
	{
		Station->Paddocks.Empty();
		int32 Depth = 0;
		int32 ObjStart = INDEX_NONE;
		for (int32 P = PdkIdx + PdkToken.Len(); P < Json.Len(); P++)
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
					const FString ObjJson = Json.Mid(ObjStart, P - ObjStart + 1);
					Station->Paddocks.Add(FPaddock::FromJson(ObjJson));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0)
			{
				break;
			}
		}
	}

	return Station;
}
