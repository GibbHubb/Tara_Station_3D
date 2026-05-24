#include "Systems/EconomySystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Herd.h"
#include "Entities/CattleCohort.h"

FEconomySystem::FEconomySystem(FEventBus& InBus, FStation& InStation)
	: Bus(InBus), Station(InStation)
{
	CashAtYearStart = Station.GetPlayer().Cash;
	SubIdCattleDied = Bus.CattleDied.Subscribe([this](const FCattleDiedPayload& P)
	{
		CattleDiedThisYear += P.Count;
	});
}

void FEconomySystem::TickDay(int32 DayOfYear, int32 Year)
{
	// Wage drip — ringers only.
	if (Station.GetPlayer().Role == EPlayerRole::Ringer)
	{
		PendingWageDay += 1;
		if (PendingWageDay >= DaysPerWage)
		{
			PendingWageDay = 0;
			AddCash(RingerWeeklyWage, TEXT("ringer wage"));
		}
	}

	// Tick supplements.
	for (FSupplementEffect& Supp : ActiveSupplements)
	{
		if (Supp.DaysUntilArrival > 0)
		{
			Supp.DaysUntilArrival -= 1;
		}
		else
		{
			Supp.DaysRemaining = FMath::Max(0, Supp.DaysRemaining - 1);
		}
	}
	ActiveSupplements.RemoveAll([](const FSupplementEffect& S)
	{
		return S.DaysRemaining <= 0 && S.DaysUntilArrival <= 0;
	});

	// Year-end flag — set once on day 365.
	if (DayOfYear == 365 && Year == Station.GetClock().Year)
	{
		bYearEndPending = true;
	}
}

void FEconomySystem::AddCash(int32 Delta, const FString& Reason)
{
	FPlayer& P = Station.GetPlayer();
	P.Cash = FMath::Max(0, P.Cash + Delta);

	FMoneyChangedPayload Payload;
	Payload.Delta = Delta;
	Payload.Reason = Reason;
	Payload.NewCash = P.Cash;
	Bus.MoneyChanged.Emit(Payload);
}

bool FEconomySystem::BuySupplement(ESupplementKind Kind)
{
	const int32 BaseDays = FSupplementProfile::DefaultDurationDays(Kind);
	int32 DailyCost = 0;
	const FPriceTable& Prices = Station.GetPrices();
	switch (Kind)
	{
	case ESupplementKind::Lick:     DailyCost = Prices.BasePerLickDay; break;
	case ESupplementKind::Molasses: DailyCost = Prices.BasePerMolassesDay; break;
	case ESupplementKind::Feed:     DailyCost = Prices.BasePerFeedDay; break;
	}
	const int32 TotalCost = DailyCost * BaseDays;

	if (Station.GetPlayer().Cash < TotalCost) return false;

	// Feed blocked during waterless crisis.
	if (Kind == ESupplementKind::Feed && !IsFeedAvailableHere()) return false;

	const FString KindStr =
		Kind == ESupplementKind::Lick ? TEXT("lick") :
		Kind == ESupplementKind::Molasses ? TEXT("molasses") : TEXT("feed");
	AddCash(-TotalCost, FString::Printf(TEXT("buy %s (%dd)"), *KindStr, BaseDays));

	FSupplementEffect Effect;
	Effect.Kind = Kind;
	Effect.DaysRemaining = BaseDays;
	Effect.DaysUntilArrival = FSupplementProfile::DefaultDeliveryDays(Kind);
	Effect.GrassFloorLift = FSupplementProfile::GrassFloorLift(Kind);
	Effect.ConditionRecoveryBoost = FSupplementProfile::ConditionRecoveryBoost(Kind);
	Effect.TotalDays = BaseDays;
	ActiveSupplements.Add(Effect);

	FSupplementOrderedPayload P;
	P.Kind = KindStr;
	P.Cost = TotalCost;
	P.ArrivesInDays = Effect.DaysUntilArrival;
	Bus.SupplementOrdered.Emit(P);

	return true;
}

bool FEconomySystem::IsFeedAvailableHere() const
{
	// Locked from TS8: feed/molasses cannot save a herd in a waterless paddock.
	const FString PaddockId = Station.GetHerd().CurrentPaddockId;
	return Station.GetWaterAccess(PaddockId) != EWaterAccess::None;
}

float FEconomySystem::ActiveGrassFloorLift() const
{
	float Sum = 0.0f;
	for (const FSupplementEffect& S : ActiveSupplements)
	{
		if (S.DaysUntilArrival == 0 && S.DaysRemaining > 0)
		{
			Sum += S.GrassFloorLift;
		}
	}
	return Sum;
}

float FEconomySystem::ActiveRecoveryBoost() const
{
	float Sum = 0.0f;
	for (const FSupplementEffect& S : ActiveSupplements)
	{
		if (S.DaysUntilArrival == 0 && S.DaysRemaining > 0)
		{
			Sum += S.ConditionRecoveryBoost;
		}
	}
	return Sum;
}

FEconomySystem::FSellResult FEconomySystem::SellCattle(int32 Count, ESeason Season)
{
	FSellResult Result;
	if (Count <= 0) return Result;

	FHerd& Herd = Station.GetHerd();

	// Sort cohort indices by count descending.
	TArray<int32> SortedIdx;
	SortedIdx.Reserve(Herd.Cohorts.Num());
	for (int32 I = 0; I < Herd.Cohorts.Num(); I++) SortedIdx.Add(I);
	SortedIdx.Sort([&Herd](const int32& A, const int32& B)
	{
		return Herd.Cohorts[A].Count > Herd.Cohorts[B].Count;
	});

	int32 Remaining = Count;
	int32 Sold = 0;
	for (int32 Idx : SortedIdx)
	{
		if (Remaining <= 0) break;
		const int32 Take = FMath::Min(Herd.Cohorts[Idx].Count, Remaining);
		Herd.Cohorts[Idx].Count -= Take;
		Sold += Take;
		Remaining -= Take;
		if (Herd.Cohorts[Idx].Count == 0)
		{
			Herd.Cohorts[Idx].ConsecutiveStarvationDays = 0;
		}
	}

	if (Sold == 0) return Result;

	const int32 PricePerHead = Station.GetPrices().CurrentCattlePrice(Season);
	const int32 Total = PricePerHead * Sold;
	AddCash(Total, FString::Printf(TEXT("sold %d head @ $%d"), Sold, PricePerHead));

	FCattleSoldPayload P;
	P.Count = Sold;
	P.PricePerHead = PricePerHead;
	P.Total = Total;
	Bus.CattleSold.Emit(P);

	Result.bSuccess = true;
	Result.Amount = Total;
	Result.Sold = Sold;
	return Result;
}

bool FEconomySystem::TryConsumeYearEnd(FYearEndSummary& OutSummary)
{
	if (!bYearEndPending) return false;
	bYearEndPending = false;

	const FPlayer& P = Station.GetPlayer();
	OutSummary.Year = Station.GetClock().Year;
	OutSummary.CashStart = CashAtYearStart;
	OutSummary.CashEnd = P.Cash;
	OutSummary.CattleAlive = Station.GetHerd().GetSize();
	OutSummary.CattleDied = CattleDiedThisYear;
	OutSummary.HerdConditionMean = FMath::RoundToFloat(Station.GetHerd().GetConditionMean());

	// Reset annual counters.
	CashAtYearStart = P.Cash;
	CattleDiedThisYear = 0;

	FYearEndedPayload Pay;
	Pay.Year = OutSummary.Year;
	Bus.YearEnded.Emit(Pay);
	return true;
}

FString FEconomySystem::SerializeJson() const
{
	// Note: ActiveSupplements not serialised in Phase 0/1/2/3 because they're
	// in-flight state. If a session interrupts during a 10-day molasses run,
	// the supplement vanishes on reload — Phase 5+ adds proper round-trip.
	return FString::Printf(
		TEXT("{\"pendingWageDay\":%d,\"cattleDiedThisYear\":%d,\"cashAtYearStart\":%d,\"yearEndPending\":%s}"),
		PendingWageDay, CattleDiedThisYear, CashAtYearStart,
		bYearEndPending ? TEXT("true") : TEXT("false"));
}

void FEconomySystem::LoadFromJson(const FString& Json)
{
	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Token.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '-')) End++;
		Out = FCString::Atoi(*Json.Mid(Start, End - Start));
	};
	ReadInt(TEXT("pendingWageDay"), PendingWageDay);
	ReadInt(TEXT("cattleDiedThisYear"), CattleDiedThisYear);
	ReadInt(TEXT("cashAtYearStart"), CashAtYearStart);
	bYearEndPending = Json.Contains(TEXT("\"yearEndPending\":true"));
}
