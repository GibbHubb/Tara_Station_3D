#include "Systems/WildlifeSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/CattleCohort.h"
#include "Entities/Herd.h"
#include "Entities/Player.h"
#include "Entities/Paddock.h"
#include "Systems/EconomySystem.h"

FWildlifeSystem::FWildlifeSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FWildlifeSystem::InitDefaults()
{
	if (Pests.Num() == 0 && Station.GetPaddocks().Num() > 0)
	{
		// Seed one dingo in River Paddock (B) so the player learns the loop.
		FString RiverId = TEXT("B");
		const FPaddock* River = Station.PaddockById(RiverId);
		if (!River) RiverId = Station.GetPaddocks()[0].Id;
		FPestEntry E;
		E.PaddockId = RiverId;
		E.Species = EPestSpecies::Dingo;
		E.Population = 3.0f;
		Pests.Add(E);
	}
	if (Invasives.Num() == 0 && Station.GetPaddocks().Num() > 0)
	{
		// Seed parkinsonia at low coverage in Back Paddock (C).
		FString BackId = TEXT("C");
		const FPaddock* Back = Station.PaddockById(BackId);
		if (!Back) BackId = Station.GetPaddocks()[0].Id;
		FInvasiveEntry E;
		E.PaddockId = BackId;
		E.Species = EInvasiveSpecies::Parkinsonia;
		E.Coverage = 8.0f;
		Invasives.Add(E);
	}
}

void FWildlifeSystem::TickDay()
{
	// Pest population growth + calf-kill rolls when herd shares the paddock.
	for (FPestEntry& P : Pests)
	{
		P.Population *= (1.0f + PestGrowthPerDay);
		const bool bHere = (Station.GetHerd().CurrentPaddockId == P.PaddockId);
		if (!bHere) continue;

		const FPestImpact Impact = GetPestImpact(P.Species);
		const float Prob = Impact.CalfKillProbPerDay * P.Population;
		if (FMath::FRand() < Prob)
		{
			FCattleCohort* Calves = nullptr;
			for (FCattleCohort& C : Station.GetHerd().Cohorts)
			{
				if (C.State == ECohortState::Unweaned && C.Count > 0)
				{
					Calves = &C;
					break;
				}
			}
			if (Calves)
			{
				Calves->Count -= 1;
				FCattleDiedPayload Payload;
				Payload.HerdId = P.PaddockId;
				Payload.CohortBirthYear = Calves->BirthYear;
				Payload.Count = 1;
				Payload.Cause = FString::Printf(TEXT("%s attack"), *PestSpeciesToString(P.Species));
				Bus.CattleDied.Emit(Payload);
			}
		}
	}

	// Invasive spread + occasional jump to adjacent paddock when coverage > 80.
	const int32 CountBeforeJump = Invasives.Num();
	for (int32 I = 0; I < CountBeforeJump; I++)
	{
		FInvasiveEntry& Inv = Invasives[I];
		Inv.Coverage = FMath::Min(100.0f, Inv.Coverage + InvasiveSpreadPerDay);

		if (Inv.Coverage > 80.0f && FMath::FRand() < 0.01f)
		{
			const TArray<FString>* AdjList = Station.GetAdjacency().Find(Inv.PaddockId);
			if (AdjList && AdjList->Num() > 0)
			{
				const FString TargetId = (*AdjList)[FMath::RandRange(0, AdjList->Num() - 1)];
				bool bExists = false;
				for (const FInvasiveEntry& E : Invasives)
				{
					if (E.PaddockId == TargetId && E.Species == Inv.Species) { bExists = true; break; }
				}
				if (!bExists)
				{
					FInvasiveEntry New;
					New.PaddockId = TargetId;
					New.Species = Inv.Species;
					New.Coverage = 5.0f;
					Invasives.Add(New);

					FInvasiveSpreadPayload Payload;
					Payload.FromPaddockId = Inv.PaddockId;
					Payload.ToPaddockId = TargetId;
					Payload.Species = (int32)Inv.Species;
					Bus.InvasiveSpread.Emit(Payload);
				}
			}
		}
	}
}

const FSpeciesDef* FWildlifeSystem::RollBirdSighting(ESpeciesHabitat Habitat) const
{
	TArray<const FSpeciesDef*> Weighted;
	for (const FSpeciesDef& S : FSpeciesRegistry::Birds())
	{
		if (S.Habitat != Habitat && S.Habitat != ESpeciesHabitat::Any) continue;
		const int32 Weight = (S.Rarity == ESpeciesRarity::Common) ? 6
			: (S.Rarity == ESpeciesRarity::Uncommon) ? 2 : 1;
		for (int32 I = 0; I < Weight; I++) Weighted.Add(&S);
	}
	if (Weighted.Num() == 0) return nullptr;
	return Weighted[FMath::RandRange(0, Weighted.Num() - 1)];
}

FWildlifeSystem::FLogResult FWildlifeSystem::LogBird(const FString& SpeciesId)
{
	FLogResult R;
	const FSpeciesDef* Sp = FSpeciesRegistry::SpeciesById(SpeciesId);
	if (!Sp || Sp->Group != ESpeciesGroup::Bird) return R;

	const bool bFirst = !LoggedSpeciesIds.Contains(SpeciesId);
	if (bFirst)
	{
		LoggedSpeciesIds.Add(SpeciesId);
		R.bFirst = true;
		R.Payout = Sp->ResearcherPayout;
	}

	if (R.Payout > 0)
	{
		const FString Reason = FString::Printf(TEXT("bird researcher: %s"), *Sp->Name);
		Station.GetEconomySystem().AddCash(R.Payout, Reason);
	}

	FBirdLoggedPayload Payload;
	Payload.SpeciesId = SpeciesId;
	Payload.Payout = R.Payout;
	Payload.bFirst = R.bFirst;
	Bus.BirdLogged.Emit(Payload);

	return R;
}

bool FWildlifeSystem::ShootPest(const FString& PaddockId, EPestSpecies Species)
{
	const float Skill = Station.GetPlayer().Skills.Shooting;
	// 0.4 base + skill/200 bonus — mirrors 2D.
	if (FMath::FRand() > 0.4f + Skill / 200.0f) return false;

	int32 FoundIdx = -1;
	for (int32 I = 0; I < Pests.Num(); I++)
	{
		if (Pests[I].PaddockId == PaddockId && Pests[I].Species == Species)
		{
			FoundIdx = I;
			break;
		}
	}
	if (FoundIdx == -1) return false;

	Pests[FoundIdx].Population = FMath::Max(0.0f, Pests[FoundIdx].Population - 1.0f);
	if (Pests[FoundIdx].Population <= 0.0f)
	{
		Pests.RemoveAt(FoundIdx);
	}

	// Small skill bump.
	Station.GetPlayer().Skills.Shooting = FMath::Min(100.0f, Skill + 0.4f);

	FPestShotPayload Payload;
	Payload.PaddockId = PaddockId;
	Payload.Species = (int32)Species;
	Bus.PestShot.Emit(Payload);

	return true;
}

bool FWildlifeSystem::TreatInvasive(const FString& PaddockId, EInvasiveSpecies Species)
{
	int32 FoundIdx = -1;
	for (int32 I = 0; I < Invasives.Num(); I++)
	{
		if (Invasives[I].PaddockId == PaddockId && Invasives[I].Species == Species)
		{
			FoundIdx = I;
			break;
		}
	}
	if (FoundIdx == -1) return false;

	Invasives[FoundIdx].Coverage = FMath::Max(0.0f, Invasives[FoundIdx].Coverage - 25.0f);

	FInvasiveTreatedPayload Payload;
	Payload.PaddockId = PaddockId;
	Payload.Species = (int32)Species;
	Payload.NewCoverage = Invasives[FoundIdx].Coverage;
	Bus.InvasiveTreated.Emit(Payload);

	if (Invasives[FoundIdx].Coverage <= 0.0f)
	{
		Invasives.RemoveAt(FoundIdx);
	}

	return true;
}

FString FWildlifeSystem::SerializeJson() const
{
	auto SerArr = [](auto& Container) -> FString
	{
		FString S = TEXT("[");
		bool bFirst = true;
		for (const auto& Item : Container)
		{
			if (!bFirst) S += TEXT(",");
			bFirst = false;
			S += Item.SerializeJson();
		}
		S += TEXT("]");
		return S;
	};
	const FString PestsJson = SerArr(Pests);
	const FString InvasivesJson = SerArr(Invasives);

	FString LoggedJson = TEXT("[");
	{
		bool bFirst = true;
		for (const FString& Id : LoggedSpeciesIds)
		{
			if (!bFirst) LoggedJson += TEXT(",");
			bFirst = false;
			LoggedJson += FString::Printf(TEXT("\"%s\""), *Id);
		}
	}
	LoggedJson += TEXT("]");

	return FString::Printf(
		TEXT("{\"pests\":%s,\"invasives\":%s,\"loggedSpeciesIds\":%s}"),
		*PestsJson, *InvasivesJson, *LoggedJson);
}

void FWildlifeSystem::LoadFromJson(const FString& Json)
{
	Pests.Empty();
	Invasives.Empty();
	LoggedSpeciesIds.Empty();

	auto ParseArr = [&Json](const FString& ArrayToken, auto OnObject)
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
					OnObject(Json.Mid(ObjStart, P - ObjStart + 1));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0) break;
		}
	};
	ParseArr(TEXT("\"pests\":["), [&](const FString& Obj) { Pests.Add(FPestEntry::FromJson(Obj)); });
	ParseArr(TEXT("\"invasives\":["), [&](const FString& Obj) { Invasives.Add(FInvasiveEntry::FromJson(Obj)); });

	// loggedSpeciesIds: array of strings.
	const FString LTok = TEXT("\"loggedSpeciesIds\":[");
	const int32 LIdx = Json.Find(LTok);
	if (LIdx != INDEX_NONE)
	{
		int32 P = LIdx + LTok.Len();
		const int32 End = Json.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, P);
		while (End != INDEX_NONE && P < End)
		{
			const int32 Q = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, P);
			if (Q == INDEX_NONE || Q >= End) break;
			const int32 Q2 = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Q + 1);
			if (Q2 == INDEX_NONE || Q2 >= End) break;
			LoggedSpeciesIds.Add(Json.Mid(Q + 1, Q2 - Q - 1));
			P = Q2 + 1;
		}
	}
}
