#include "Entities/Herd.h"

FHerd::FHerd(const FCattleCohort& StartingCohort, const FString& StartPaddockId)
	: CurrentPaddockId(StartPaddockId)
{
	Cohorts.Add(StartingCohort);
}

int32 FHerd::GetSize() const
{
	int32 Sum = 0;
	for (const FCattleCohort& C : Cohorts)
	{
		Sum += C.Count;
	}
	return Sum;
}

float FHerd::GetConditionMean() const
{
	float Weighted = 0.0f;
	int32 Total = 0;
	for (const FCattleCohort& C : Cohorts)
	{
		if (C.Count <= 0) continue;
		Weighted += C.ConditionMean * C.Count;
		Total += C.Count;
	}
	return (Total == 0) ? 0.0f : (Weighted / Total);
}

int32 FHerd::HerdInPaddock(const FString& PaddockId) const
{
	return (PaddockId == CurrentPaddockId) ? GetSize() : 0;
}

int32 FHerd::ApplyBreakaway(int32 RequestedCount)
{
	if (RequestedCount <= 0) return 0;

	// Sort cohorts by count descending (stable preferred but not critical here).
	TArray<int32> SortedIdx;
	SortedIdx.Reserve(Cohorts.Num());
	for (int32 I = 0; I < Cohorts.Num(); I++) SortedIdx.Add(I);
	SortedIdx.Sort([this](const int32& A, const int32& B)
	{
		return Cohorts[A].Count > Cohorts[B].Count;
	});

	int32 Remaining = RequestedCount;
	int32 Removed = 0;
	for (int32 Idx : SortedIdx)
	{
		if (Remaining <= 0) break;
		const int32 Take = FMath::Min(Cohorts[Idx].Count, Remaining);
		Cohorts[Idx].Count -= Take;
		Removed += Take;
		Remaining -= Take;
	}
	return Removed;
}

FString FHerd::SerializeJson() const
{
	FString CohortsJson = TEXT("[");
	for (int32 I = 0; I < Cohorts.Num(); I++)
	{
		if (I > 0) CohortsJson += TEXT(",");
		CohortsJson += Cohorts[I].SerializeJson();
	}
	CohortsJson += TEXT("]");
	return FString::Printf(
		TEXT("{\"currentPaddockId\":\"%s\",\"cohorts\":%s}"),
		*CurrentPaddockId, *CohortsJson);
}

FHerd FHerd::FromJson(const FString& Json)
{
	FHerd Result;

	// Pull currentPaddockId.
	const FString Token = TEXT("\"currentPaddockId\":\"");
	const int32 Idx = Json.Find(Token);
	if (Idx != INDEX_NONE)
	{
		const int32 Start = Idx + Token.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE)
		{
			Result.CurrentPaddockId = Json.Mid(Start, End - Start);
		}
	}

	// Pull cohort objects. This is a tiny ad-hoc parser; production code should
	// use a proper JSON library or swap to a binary serialiser. Adequate for
	// Phase 0 round-trips.
	const FString ArrayToken = TEXT("\"cohorts\":[");
	const int32 ArrIdx = Json.Find(ArrayToken);
	if (ArrIdx != INDEX_NONE)
	{
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
					const FString ObjJson = Json.Mid(ObjStart, P - ObjStart + 1);
					Result.Cohorts.Add(FCattleCohort::FromJson(ObjJson));
					ObjStart = INDEX_NONE;
				}
			}
			else if (Ch == ']' && Depth == 0)
			{
				break;
			}
		}
	}

	return Result;
}
