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
	// In transit, the herd is in neither paddock. Mirrors 2D Herd.ts.
	if (IsInTransit()) return 0;
	return (PaddockId == CurrentPaddockId) ? GetSize() : 0;
}

int32 FHerd::ApplyBreakaway(int32 RequestedCount)
{
	if (RequestedCount <= 0) return 0;

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

bool FHerd::StartMuster(
	const FString& ToPaddockId,
	int32 Days,
	int32 VehicleType,
	const TArray<FString>& HandIds,
	int32 FuelCost,
	int32 BreakawayPending)
{
	if (Muster.Status == EMusterStatus::Transit) return false;
	if (ToPaddockId == CurrentPaddockId) return false;
	if (Days <= 0) return false;
	if (GetSize() == 0) return false;

	Muster.Status = EMusterStatus::Transit;
	Muster.ToPaddockId = ToPaddockId;
	Muster.DaysRemaining = Days;
	Muster.TotalDays = Days;
	Muster.VehicleType = VehicleType;
	Muster.HandIds = HandIds;
	Muster.FuelCost = FuelCost;
	Muster.BreakawayPending = BreakawayPending;
	return true;
}

FHerd::FCancelResult FHerd::CancelMuster()
{
	FCancelResult R;
	if (Muster.Status != EMusterStatus::Transit) return R;
	R.bWasInTransit = true;
	R.DaysElapsed = Muster.TotalDays - Muster.DaysRemaining;
	Muster = FMusterState{};
	return R;
}

FHerd::FMusterTickResult FHerd::TickMusterDay()
{
	FMusterTickResult R;
	if (Muster.Status != EMusterStatus::Transit) return R;

	Muster.DaysRemaining -= 1;
	if (Muster.DaysRemaining <= 0)
	{
		R.bCompletedThisTick = true;
		R.CompletedToPaddockId = Muster.ToPaddockId;
		const int32 Breakaway = Muster.BreakawayPending;

		CurrentPaddockId = Muster.ToPaddockId;
		Muster = FMusterState{};

		if (Breakaway > 0)
		{
			R.BreakawayApplied = ApplyBreakaway(Breakaway);
		}
	}
	return R;
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

	// Muster JSON — explicit even when idle, so FromJson round-trips cleanly.
	FString HandIdsJson = TEXT("[");
	for (int32 I = 0; I < Muster.HandIds.Num(); I++)
	{
		if (I > 0) HandIdsJson += TEXT(",");
		HandIdsJson += FString::Printf(TEXT("\"%s\""), *Muster.HandIds[I]);
	}
	HandIdsJson += TEXT("]");

	const FString MusterJson = FString::Printf(
		TEXT("{\"status\":%d,\"toPaddockId\":\"%s\",\"daysRemaining\":%d,\"totalDays\":%d,\"vehicleType\":%d,\"handIds\":%s,\"fuelCost\":%d,\"breakawayPending\":%d}"),
		(int32)Muster.Status, *Muster.ToPaddockId, Muster.DaysRemaining, Muster.TotalDays,
		Muster.VehicleType, *HandIdsJson, Muster.FuelCost, Muster.BreakawayPending);

	return FString::Printf(
		TEXT("{\"currentPaddockId\":\"%s\",\"cohorts\":%s,\"muster\":%s}"),
		*CurrentPaddockId, *CohortsJson, *MusterJson);
}

FHerd FHerd::FromJson(const FString& Json)
{
	FHerd Result;

	const FString CpToken = TEXT("\"currentPaddockId\":\"");
	const int32 CpIdx = Json.Find(CpToken);
	if (CpIdx != INDEX_NONE)
	{
		const int32 Start = CpIdx + CpToken.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE)
		{
			Result.CurrentPaddockId = Json.Mid(Start, End - Start);
		}
	}

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

	// M4a — read muster sub-object if present.
	const FString MusterToken = TEXT("\"muster\":{");
	const int32 MIdx = Json.Find(MusterToken);
	if (MIdx != INDEX_NONE)
	{
		int32 Depth = 1;
		int32 P = MIdx + MusterToken.Len();
		int32 ObjEnd = INDEX_NONE;
		while (P < Json.Len())
		{
			if (Json[P] == '{') Depth++;
			else if (Json[P] == '}')
			{
				Depth--;
				if (Depth == 0) { ObjEnd = P; break; }
			}
			P++;
		}
		if (ObjEnd != INDEX_NONE)
		{
			const FString MJson = Json.Mid(MIdx + MusterToken.Len() - 1, ObjEnd - MIdx - MusterToken.Len() + 2);

			auto ReadIntIn = [&](const TCHAR* Key, int32& Out)
			{
				const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
				const int32 Idx = MJson.Find(Token);
				if (Idx == INDEX_NONE) return;
				int32 Q = Idx + Token.Len();
				int32 Start = Q;
				while (Q < MJson.Len() && (FChar::IsDigit(MJson[Q]) || MJson[Q] == '-')) Q++;
				Out = FCString::Atoi(*MJson.Mid(Start, Q - Start));
			};
			auto ReadStrIn = [&](const TCHAR* Key, FString& Out)
			{
				const FString Token = FString::Printf(TEXT("\"%s\":\""), Key);
				const int32 Idx = MJson.Find(Token);
				if (Idx == INDEX_NONE) return;
				const int32 Start = Idx + Token.Len();
				const int32 End = MJson.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
				if (End != INDEX_NONE) Out = MJson.Mid(Start, End - Start);
			};

			int32 StatusInt = 0;
			ReadIntIn(TEXT("status"), StatusInt);
			Result.Muster.Status = (EMusterStatus)StatusInt;
			ReadStrIn(TEXT("toPaddockId"), Result.Muster.ToPaddockId);
			ReadIntIn(TEXT("daysRemaining"), Result.Muster.DaysRemaining);
			ReadIntIn(TEXT("totalDays"), Result.Muster.TotalDays);
			ReadIntIn(TEXT("vehicleType"), Result.Muster.VehicleType);
			ReadIntIn(TEXT("fuelCost"), Result.Muster.FuelCost);
			ReadIntIn(TEXT("breakawayPending"), Result.Muster.BreakawayPending);

			// HandIds array — simple comma-separated quoted strings parse.
			const int32 HIdx = MJson.Find(TEXT("\"handIds\":["));
			if (HIdx != INDEX_NONE)
			{
				const int32 Start = HIdx + 11;
				const int32 End = MJson.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
				if (End != INDEX_NONE)
				{
					const FString Inner = MJson.Mid(Start, End - Start);
					int32 Q = 0;
					while (Q < Inner.Len())
					{
						const int32 QStart = Inner.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Q);
						if (QStart == INDEX_NONE) break;
						const int32 QEnd = Inner.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, QStart + 1);
						if (QEnd == INDEX_NONE) break;
						Result.Muster.HandIds.Add(Inner.Mid(QStart + 1, QEnd - QStart - 1));
						Q = QEnd + 1;
					}
				}
			}
		}
	}

	return Result;
}
