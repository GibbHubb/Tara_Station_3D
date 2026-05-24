#include "Station.h"

FStation::FStation()
{
	SeedDefaults();

	// Systems — construct in dependency order. Water FIRST (Condition reads
	// water access on its tick); refresh access immediately so the very first
	// TickDay has a valid map.
	WaterSys = MakeUnique<FWaterSystem>(Bus, *this);
	WaterSys->RecomputeAccess();
	ConditionSys = MakeUnique<FConditionSystem>(Bus, *this);
}

void FStation::SeedDefaults()
{
	// Default M1 starter — three paddocks, one 8-head cohort in Home.
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

	// M2 seed — adjacency A-B-C linear chain.
	Adjacency.Add(TEXT("A"), { TEXT("B") });
	Adjacency.Add(TEXT("B"), { TEXT("A"), TEXT("C") });
	Adjacency.Add(TEXT("C"), { TEXT("B") });

	// M2 seed — 2 bores: bore-1 serves Home only (single-paddock topology);
	// bore-2 serves River + Back (shared 2-paddock topology). Player meets
	// both topologies on day one — when bore-2 fails, both paddocks lose
	// water at the same time, teaching the design lesson.
	FBoreConfig Bore1Cfg;
	Bore1Cfg.Id = TEXT("bore-1");
	Bore1Cfg.ServesPaddockIds.Add(TEXT("A"));
	Bore1Cfg.Condition = 88.0f;
	Bores.Add(FBore(Bore1Cfg));

	FBoreConfig Bore2Cfg;
	Bore2Cfg.Id = TEXT("bore-2");
	Bore2Cfg.ServesPaddockIds.Add(TEXT("B"));
	Bore2Cfg.ServesPaddockIds.Add(TEXT("C"));
	Bore2Cfg.Condition = 82.0f;
	Bores.Add(FBore(Bore2Cfg));
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

	// Order: Water → Condition (Condition reads water-access cached by Water).
	WaterSys->TickDay();
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

EWaterAccess FStation::GetWaterAccess(const FString& PaddockId) const
{
	return WaterSys->GetAccess(PaddockId);
}

FStation::FCheckBoreResult FStation::CheckBore(const FString& BoreId)
{
	const auto R = WaterSys->CheckBore(BoreId, Clock.DayOfYear);
	FCheckBoreResult Out;
	Out.bFixed = R.bFixed;
	Out.DaysSpent = R.DaysSpent;
	return Out;
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

	FString BoresJson = TEXT("[");
	for (int32 I = 0; I < Bores.Num(); I++)
	{
		if (I > 0) BoresJson += TEXT(",");
		BoresJson += Bores[I].SerializeJson();
	}
	BoresJson += TEXT("]");

	// Adjacency: {"A":["B"],"B":["A","C"],"C":["B"]}
	FString AdjJson = TEXT("{");
	bool bFirstKey = true;
	for (const auto& Pair : Adjacency)
	{
		if (!bFirstKey) AdjJson += TEXT(",");
		bFirstKey = false;
		AdjJson += FString::Printf(TEXT("\"%s\":["), *Pair.Key);
		for (int32 I = 0; I < Pair.Value.Num(); I++)
		{
			if (I > 0) AdjJson += TEXT(",");
			AdjJson += FString::Printf(TEXT("\"%s\""), *Pair.Value[I]);
		}
		AdjJson += TEXT("]");
	}
	AdjJson += TEXT("}");

	return FString::Printf(
		TEXT("{\"schemaVersion\":\"%s\",\"clock\":%s,\"paddocks\":%s,\"herd\":%s,\"adjacency\":%s,\"bores\":%s}"),
		TARA_SIM_SAVE_SCHEMA_VERSION,
		*Clock.SerializeJson(),
		*PaddocksJson,
		*Herd.SerializeJson(),
		*AdjJson,
		*BoresJson);
}

namespace
{
	// Helper — pull the JSON object substring starting at the first '{' after `Token`.
	FString ExtractObject(const FString& Json, const FString& Token)
	{
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return FString();
		int32 Start = Idx + Token.Len();
		while (Start < Json.Len() && Json[Start] != '{') Start++;
		if (Start >= Json.Len()) return FString();
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
		return Json.Mid(Start, End - Start);
	}
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
		// Schema mismatch — discard. Subsystem starts a fresh game.
		return nullptr;
	}

	auto Station = MakeUnique<FStation>();
	// Wipe defaults — we're about to overwrite them from the snapshot.
	Station->Paddocks.Empty();
	Station->Bores.Empty();
	Station->Adjacency.Empty();

	// Clock.
	const FString ClockJson = ExtractObject(Json, TEXT("\"clock\":"));
	if (!ClockJson.IsEmpty()) Station->Clock = FSeasonClock::FromJson(ClockJson);

	// Herd.
	const FString HerdJson = ExtractObject(Json, TEXT("\"herd\":"));
	if (!HerdJson.IsEmpty()) Station->Herd = FHerd::FromJson(HerdJson);

	// Paddocks array.
	{
		const FString PdkToken = TEXT("\"paddocks\":[");
		const int32 PdkIdx = Json.Find(PdkToken);
		if (PdkIdx != INDEX_NONE)
		{
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
						Station->Paddocks.Add(FPaddock::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
						ObjStart = INDEX_NONE;
					}
				}
				else if (Ch == ']' && Depth == 0)
				{
					break;
				}
			}
		}
	}

	// Bores array.
	{
		const FString BorToken = TEXT("\"bores\":[");
		const int32 BorIdx = Json.Find(BorToken);
		if (BorIdx != INDEX_NONE)
		{
			int32 Depth = 0;
			int32 ObjStart = INDEX_NONE;
			for (int32 P = BorIdx + BorToken.Len(); P < Json.Len(); P++)
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
						Station->Bores.Add(FBore::FromJson(Json.Mid(ObjStart, P - ObjStart + 1)));
						ObjStart = INDEX_NONE;
					}
				}
				else if (Ch == ']' && Depth == 0)
				{
					break;
				}
			}
		}
	}

	// Adjacency: {"A":["B"],"B":["A","C"],"C":["B"]}
	{
		const FString AdjToken = TEXT("\"adjacency\":{");
		const int32 AdjIdx = Json.Find(AdjToken);
		if (AdjIdx != INDEX_NONE)
		{
			int32 P = AdjIdx + AdjToken.Len();
			while (P < Json.Len())
			{
				if (Json[P] == '}') break;
				if (Json[P] != '"') { P++; continue; }

				const int32 KeyStart = P + 1;
				const int32 KeyEnd = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, KeyStart);
				if (KeyEnd == INDEX_NONE) break;
				const FString Key = Json.Mid(KeyStart, KeyEnd - KeyStart);
				P = KeyEnd + 1;
				while (P < Json.Len() && Json[P] != '[') P++;
				if (P >= Json.Len()) break;
				P++;
				TArray<FString> Values;
				while (P < Json.Len() && Json[P] != ']')
				{
					if (Json[P] == '"')
					{
						const int32 VS = P + 1;
						const int32 VE = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, VS);
						if (VE == INDEX_NONE) { P = Json.Len(); break; }
						Values.Add(Json.Mid(VS, VE - VS));
						P = VE + 1;
					}
					else
					{
						P++;
					}
				}
				Station->Adjacency.Add(Key, Values);
				P++;
			}
		}
	}

	// Reset water access cache for the restored topology.
	if (Station->WaterSys) Station->WaterSys->RecomputeAccess();

	return Station;
}
