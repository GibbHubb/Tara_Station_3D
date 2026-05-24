#pragma once

#include "CoreMinimal.h"
#include "../SeasonClock.h"
#include "../Entities/Supplement.h"
#include "../Entities/PriceTable.h"
#include "../Entities/Player.h"

class FEventBus;
class FStation;

struct TARASIMCORE_API FYearEndSummary
{
	int32 Year = 0;
	int32 CashStart = 0;
	int32 CashEnd = 0;
	int32 CattleAlive = 0;
	int32 CattleDied = 0;
	float HerdConditionMean = 0.0f;
};

// Mirrors src/sim/systems/EconomySystem.ts. Pure logic — wage drip, supplement
// ordering, cattle sale, year-end summary capture. Ports unchanged per audit.

class TARASIMCORE_API FEconomySystem
{
public:
	static constexpr int32 RingerWeeklyWage = 500;
	static constexpr int32 DaysPerWage = 7;

	FEconomySystem(FEventBus& InBus, FStation& InStation);

	void TickDay(int32 DayOfYear, int32 Year);

	void AddCash(int32 Delta, const FString& Reason);

	// True if purchase succeeded.
	bool BuySupplement(ESupplementKind Kind);

	// Feed is blocked during a waterless-paddock crisis (locked from TS8).
	bool IsFeedAvailableHere() const;

	// Sum of active (delivered, non-expired) supplement floor lifts.
	float ActiveGrassFloorLift() const;
	float ActiveRecoveryBoost() const;

	struct FSellResult { bool bSuccess = false; int32 Amount = 0; int32 Sold = 0; };
	FSellResult SellCattle(int32 Count, ESeason Season);

	// Year-end summary — pulled by the scene/UI when day 365 is hit and the
	// flag is set. Returns nullopt-equivalent (Year == 0) if not pending.
	bool TryConsumeYearEnd(FYearEndSummary& OutSummary);

	// External tracking — incremented by ConditionSystem via the event bus.
	int32 CattleDiedThisYear = 0;

	TArray<FSupplementEffect> ActiveSupplements;
	int32 PendingWageDay = 0;
	int32 CashAtYearStart = 250;
	bool bYearEndPending = false;

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

private:
	FEventBus& Bus;
	FStation& Station;
	int32 SubIdCattleDied = -1;
};
