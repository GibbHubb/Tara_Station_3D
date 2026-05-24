#pragma once

#include "CoreMinimal.h"

class FStation;
class FEventBus;

// Mirrors src/sim/systems/ProgressionSystem.ts. Year-end role transitions +
// property purchase + bankruptcy tracking.
//
// Per CORE_LOOP §7: the ringer→manager→owner arc is TUNABLE — starting
// hypothesis ~4 in-game years (≈1 ringer / ~2 manager / then owner). Score
// thresholds below are the 2D defaults; playtest tunes from here.

struct FYearEvaluation
{
	int32 Year = 0;
	float Score = 0.0f;                 // 0..100
	int32 CattleAlive = 0;
	int32 CattleDied = 0;
	int32 CashEnd = 0;
	float ConditionMean = 0.0f;
	int32 BirdLogCount = 0;
	int32 NewRole = 0;                  // 0=Ringer/1=Manager/2=Owner (or 3=Fired sentinel)
	bool bPromoted = false;
	bool bFired = false;
};

class TARASIMCORE_API FProgressionSystem
{
public:
	FProgressionSystem(FStation& InStation, FEventBus& InBus);

	void TickDay();

	// Year-end evaluation. Caller passes the current bird-log count (from
	// WildlifeSystem). Applies role transition + emits RoleChanged on promote.
	FYearEvaluation EvaluateYear(int32 BirdLogCount);

	bool CanBuyProperty() const;
	bool BuyProperty();

	int32 GetDaysInBankruptcy() const { return DaysInBankruptcy; }
	bool IsBankrupted() const { return bBankrupted; }

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

	// Constants — byte-for-byte from 2D ProgressionSystem.ts. Tunable per
	// CORE_LOOP §7 ~4yr-to-ownership starting hypothesis; thresholds here are
	// the 2D defaults.
	static constexpr int32 RingerToManagerScore = 70;
	static constexpr int32 FireRingerScore = 25;
	static constexpr int32 ManagerPropertyBuyPrice = 250000;
	static constexpr int32 BankruptcyDebtDays = 60;

private:
	FStation& Station;
	FEventBus& Bus;

	int32 DaysInBankruptcy = 0;
	bool bBankrupted = false;
};
