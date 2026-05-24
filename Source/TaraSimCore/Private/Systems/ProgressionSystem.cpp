#include "Systems/ProgressionSystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/Player.h"
#include "Entities/Herd.h"
#include "Systems/EconomySystem.h"

FProgressionSystem::FProgressionSystem(FStation& InStation, FEventBus& InBus)
	: Station(InStation)
	, Bus(InBus)
{
}

void FProgressionSystem::TickDay()
{
	// Bankruptcy tracking — owner only. Mirrors 2D.
	if (Station.GetPlayer().Role == EPlayerRole::Owner)
	{
		if (Station.GetPlayer().Cash < 0)
		{
			DaysInBankruptcy += 1;
			if (DaysInBankruptcy >= BankruptcyDebtDays && !bBankrupted)
			{
				bBankrupted = true;
				FBankruptcyDeclaredPayload P;
				P.Year = Station.GetClock().Year;
				P.DaysInDebt = DaysInBankruptcy;
				Bus.BankruptcyDeclared.Emit(P);
			}
		}
		else
		{
			DaysInBankruptcy = 0;
		}
	}
}

FYearEvaluation FProgressionSystem::EvaluateYear(int32 BirdLogCount)
{
	FYearEvaluation Eval;

	const int32 CattleAlive = Station.GetHerd().GetSize();
	const int32 CattleDied = Station.GetEconomySystem().CattleDiedThisYear;
	const int32 CashEnd = Station.GetPlayer().Cash;
	const float ConditionMean = Station.GetHerd().GetConditionMean();

	// Score factors (each 0..100, weighted). Byte-for-byte from 2D.
	const float SurvivalScore = (CattleAlive > 0)
		? FMath::Min(100.0f, 100.0f * ((float)CattleAlive / FMath::Max(1, CattleAlive + CattleDied)))
		: 0.0f;
	const float ConditionScore = ConditionMean;
	const float CashScore = FMath::Clamp((float)CashEnd / 100.0f, 0.0f, 100.0f);   // $10k = 100
	const float SkillScore = Station.GetPlayer().Skills.Mustering;
	const float BirdScore = FMath::Min(100.0f, BirdLogCount * 10.0f);

	const float Score =
		SurvivalScore * 0.35f +
		ConditionScore * 0.25f +
		CashScore * 0.20f +
		SkillScore * 0.10f +
		BirdScore * 0.10f;

	const EPlayerRole Role = Station.GetPlayer().Role;
	int32 NewRole = (int32)Role;
	bool bPromoted = false;
	bool bFired = false;

	if (Role == EPlayerRole::Ringer)
	{
		if (Score >= RingerToManagerScore)
		{
			Station.GetPlayer().Role = EPlayerRole::Manager;
			NewRole = (int32)EPlayerRole::Manager;
			bPromoted = true;

			FRoleChangedPayload P;
			P.From = (int32)EPlayerRole::Ringer;
			P.To = (int32)EPlayerRole::Manager;
			Bus.RoleChanged.Emit(P);
		}
		else if (Score <= FireRingerScore)
		{
			NewRole = 3;   // sentinel for "fired"; role stays Ringer for replay simplicity
			bFired = true;
		}
	}
	// Manager → Owner only via BuyProperty (player action), not auto-evaluation.

	Eval.Year = Station.GetClock().Year;
	Eval.Score = Score;
	Eval.CattleAlive = CattleAlive;
	Eval.CattleDied = CattleDied;
	Eval.CashEnd = CashEnd;
	Eval.ConditionMean = ConditionMean;
	Eval.BirdLogCount = BirdLogCount;
	Eval.NewRole = NewRole;
	Eval.bPromoted = bPromoted;
	Eval.bFired = bFired;

	FYearEvaluatedPayload Payload;
	Payload.Year = Eval.Year;
	Payload.Score = Eval.Score;
	Payload.NewRole = Eval.NewRole;
	Payload.bPromoted = Eval.bPromoted;
	Payload.bFired = Eval.bFired;
	Bus.YearEvaluated.Emit(Payload);

	return Eval;
}

bool FProgressionSystem::CanBuyProperty() const
{
	return Station.GetPlayer().Role == EPlayerRole::Manager
		&& Station.GetPlayer().Cash >= ManagerPropertyBuyPrice;
}

bool FProgressionSystem::BuyProperty()
{
	if (!CanBuyProperty()) return false;
	Station.GetEconomySystem().AddCash(-ManagerPropertyBuyPrice, TEXT("purchased property"));
	Station.GetPlayer().Role = EPlayerRole::Owner;

	FRoleChangedPayload P;
	P.From = (int32)EPlayerRole::Manager;
	P.To = (int32)EPlayerRole::Owner;
	Bus.RoleChanged.Emit(P);

	FPropertyPurchasedPayload Q;
	Q.Cost = ManagerPropertyBuyPrice;
	Bus.PropertyPurchased.Emit(Q);
	return true;
}

FString FProgressionSystem::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"daysInBankruptcy\":%d,\"bankrupted\":%s}"),
		DaysInBankruptcy, bBankrupted ? TEXT("true") : TEXT("false"));
}

void FProgressionSystem::LoadFromJson(const FString& Json)
{
	const FString DTok = TEXT("\"daysInBankruptcy\":");
	const int32 DIdx = Json.Find(DTok);
	if (DIdx != INDEX_NONE)
	{
		int32 P = DIdx + DTok.Len();
		int32 S = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		DaysInBankruptcy = FCString::Atoi(*Json.Mid(S, P - S));
	}

	const int32 BIdx = Json.Find(TEXT("\"bankrupted\":"));
	if (BIdx != INDEX_NONE)
	{
		bBankrupted = Json.Mid(BIdx + 13, 4).StartsWith(TEXT("true"));
	}
}
