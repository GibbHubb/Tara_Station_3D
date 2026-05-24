#include "Systems/BreedingSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/CattleCohort.h"
#include "Entities/Herd.h"

FBreedingSystem::FBreedingSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FBreedingSystem::TickDay()
{
	const int32 Year = Station.GetClock().Year;
	const int32 Day = Station.GetClock().DayOfYear;

	// Open the joining window once per year.
	if (Day == PreConceptionOpenDay && WindowOpenForYear != Year)
	{
		WindowOpenForYear = Year;
		WindowDaysRemaining = PreConceptionDurationDays;
		bWindowSuccess = false;

		int32 BreedersBirthYear = -1;
		for (const FCattleCohort& C : Station.GetHerd().Cohorts)
		{
			if (C.State == ECohortState::Breeder || C.State == ECohortState::Heifer)
			{
				BreedersBirthYear = C.BirthYear;
				break;
			}
		}
		FBreedingWindowOpenedPayload P;
		P.CohortBirthYear = BreedersBirthYear;
		Bus.BreedingWindowOpened.Emit(P);
	}

	// While window is open, check condition; on close, attempt conception.
	if (WindowDaysRemaining > 0)
	{
		WindowDaysRemaining -= 1;
		if (Station.GetHerd().GetConditionMean() >= MinConditionForConception)
		{
			bWindowSuccess = true;
		}
		if (WindowDaysRemaining == 0 && bWindowSuccess)
		{
			AttemptConception(Year);
		}
	}

	// Tick gestation; spawn calves on completion.
	TArray<int32> CalvingIdx;
	for (int32 I = 0; I < Pregnant.Num(); I++)
	{
		Pregnant[I].DaysSinceConception += 1;
		if (Pregnant[I].DaysSinceConception >= GestationTotalDays)
		{
			CalvingIdx.Add(I);
		}
	}
	// Process calving in reverse so removals don't shift indices.
	CalvingIdx.Sort([](const int32& A, const int32& B) { return A > B; });
	for (int32 Idx : CalvingIdx)
	{
		Calf(Pregnant[Idx]);
		Pregnant.RemoveAt(Idx);
	}
}

void FBreedingSystem::AttemptConception(int32 CurrentYear)
{
	TArray<const FCattleCohort*> Breeders;
	for (const FCattleCohort& C : Station.GetHerd().Cohorts)
	{
		if ((C.State == ECohortState::Breeder || C.State == ECohortState::Heifer) && C.Count > 0)
		{
			Breeders.Add(&C);
		}
	}
	if (Breeders.Num() == 0) return;

	int32 BreederHead = 0;
	for (const FCattleCohort* C : Breeders)
	{
		BreederHead += FMath::FloorToInt(C->Count * C->SexRatio);
	}
	const int32 Conceptions = FMath::FloorToInt(BreederHead * ConceptionRate);
	if (Conceptions <= 0) return;

	FPregnantRecord R;
	R.DamCohortBirthYear = Breeders[0]->BirthYear;
	R.ConceivedDay = Station.GetClock().DayOfYear;
	R.ConceivedYear = CurrentYear;
	R.Count = Conceptions;
	R.DaysSinceConception = 0;
	Pregnant.Add(R);

	FBreedingConceivedPayload P;
	P.CohortBirthYear = R.DamCohortBirthYear;
	P.PregnantCount = Conceptions;
	Bus.BreedingConceived.Emit(P);
}

void FBreedingSystem::Calf(const FPregnantRecord& Record)
{
	const int32 Survivors = FMath::FloorToInt(Record.Count * (1.0f - CalvingLossRate));
	if (Survivors <= 0) return;

	const int32 BirthYear = Station.GetClock().Year;
	FCattleCohort* CalfCohort = nullptr;
	for (FCattleCohort& C : Station.GetHerd().Cohorts)
	{
		if (C.BirthYear == BirthYear && C.State == ECohortState::Unweaned)
		{
			CalfCohort = &C;
			break;
		}
	}
	if (!CalfCohort)
	{
		FCattleCohortConfig Cfg;
		Cfg.BirthYear = BirthYear;
		Cfg.Count = Survivors;
		Cfg.State = ECohortState::Unweaned;
		Cfg.ConditionMean = 75.0f;
		// Phase 5+ prereq — bHorned roll: ~30% of new Unweaned cohorts will
		// need dehorning at branding. Mixed-breed default per CORE_LOOP §2.
		Cfg.bHorned = (FMath::FRand() < 0.30f);
		// Fresh weaners haven't been schooled yet — start at 0, rises via
		// the weaner-school sub-scene (TS-3D-WEANER-SCHOOL).
		Cfg.MusterTrainedness = 0.0f;
		Station.GetHerd().Cohorts.Add(FCattleCohort(Cfg));
	}
	else
	{
		CalfCohort->Count += Survivors;
	}

	// Flip dam cohort lactating — body-score-protection won't engage until
	// the player weans (Station::WeanCohort). ConditionSystem applies the
	// -0.2/day extra drift while this flag is set.
	for (FCattleCohort& C : Station.GetHerd().Cohorts)
	{
		if (C.BirthYear == Record.DamCohortBirthYear &&
			(C.State == ECohortState::Breeder || C.State == ECohortState::Heifer))
		{
			C.bLactating = true;
			break;
		}
	}

	FCalfBornPayload P;
	P.CohortBirthYear = BirthYear;
	P.CalfCount = Survivors;
	Bus.CalfBorn.Emit(P);
}

FString FBreedingSystem::SerializeJson() const
{
	FString PregJson = TEXT("[");
	for (int32 I = 0; I < Pregnant.Num(); I++)
	{
		if (I > 0) PregJson += TEXT(",");
		PregJson += FString::Printf(
			TEXT("{\"damCohortBirthYear\":%d,\"conceivedDay\":%d,\"conceivedYear\":%d,\"count\":%d,\"daysSinceConception\":%d}"),
			Pregnant[I].DamCohortBirthYear, Pregnant[I].ConceivedDay,
			Pregnant[I].ConceivedYear, Pregnant[I].Count, Pregnant[I].DaysSinceConception);
	}
	PregJson += TEXT("]");

	return FString::Printf(
		TEXT("{\"windowOpenForYear\":%d,\"windowDaysRemaining\":%d,\"windowSuccess\":%s,\"pregnant\":%s}"),
		WindowOpenForYear, WindowDaysRemaining,
		bWindowSuccess ? TEXT("true") : TEXT("false"),
		*PregJson);
}

void FBreedingSystem::LoadFromJson(const FString& Json)
{
	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		int32 P = Idx + Token.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		Out = FCString::Atoi(*Json.Mid(Start, P - Start));
	};

	ReadInt(TEXT("windowOpenForYear"), WindowOpenForYear);
	ReadInt(TEXT("windowDaysRemaining"), WindowDaysRemaining);

	const int32 WSIdx = Json.Find(TEXT("\"windowSuccess\":"));
	if (WSIdx != INDEX_NONE)
	{
		bWindowSuccess = Json.Mid(WSIdx + 16, 4).StartsWith(TEXT("true"));
	}

	Pregnant.Empty();
	const FString PTok = TEXT("\"pregnant\":[");
	const int32 PIdx = Json.Find(PTok);
	if (PIdx == INDEX_NONE) return;
	int32 Depth = 0;
	int32 ObjStart = INDEX_NONE;
	for (int32 P = PIdx + PTok.Len(); P < Json.Len(); P++)
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
				FPregnantRecord R;
				auto ReadIntIn = [&](const TCHAR* Key, int32& Out)
				{
					const FString Tok = FString::Printf(TEXT("\"%s\":"), Key);
					const int32 II = ObjJson.Find(Tok);
					if (II == INDEX_NONE) return;
					int32 Q = II + Tok.Len();
					int32 S = Q;
					while (Q < ObjJson.Len() && (FChar::IsDigit(ObjJson[Q]) || ObjJson[Q] == '-')) Q++;
					Out = FCString::Atoi(*ObjJson.Mid(S, Q - S));
				};
				ReadIntIn(TEXT("damCohortBirthYear"), R.DamCohortBirthYear);
				ReadIntIn(TEXT("conceivedDay"), R.ConceivedDay);
				ReadIntIn(TEXT("conceivedYear"), R.ConceivedYear);
				ReadIntIn(TEXT("count"), R.Count);
				ReadIntIn(TEXT("daysSinceConception"), R.DaysSinceConception);
				Pregnant.Add(R);
				ObjStart = INDEX_NONE;
			}
		}
		else if (Ch == ']' && Depth == 0) break;
	}
}
