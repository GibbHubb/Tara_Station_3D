#pragma once

#include "CoreMinimal.h"

class FStation;
class FEventBus;

// Mirrors src/sim/systems/BreedingSystem.ts. Yearly joining window (mid-dry)
// → conception roll at window close → ~280-day gestation → calving with 5%
// loss rate spawns a new unweaned cohort.
//
// Per CORE_LOOP §2: synchronised calving is the goal that drives the year —
// good joining timing → tight calving window → efficient branding round. The
// 3D layer (Phase 5+) adds branding/weaning/preg-test sub-scenes on top of
// this state machine.

struct FPregnantRecord
{
	int32 DamCohortBirthYear = 0;
	int32 ConceivedDay = 0;
	int32 ConceivedYear = 0;
	int32 Count = 0;
	int32 DaysSinceConception = 0;
};

class TARASIMCORE_API FBreedingSystem
{
public:
	FBreedingSystem(FStation& InStation, FEventBus& InBus);

	void TickDay();

	// Tuning constants — byte-for-byte from 2D BreedingSystem.ts.
	static constexpr int32 PreConceptionOpenDay = 75;
	static constexpr int32 PreConceptionDurationDays = 90;
	static constexpr int32 GestationTotalDays = 280;
	static constexpr float MinConditionForConception = 55.0f;
	static constexpr float ConceptionRate = 0.65f;
	static constexpr float CalvingLossRate = 0.05f;

	int32 GetWindowOpenForYear() const { return WindowOpenForYear; }
	int32 GetWindowDaysRemaining() const { return WindowDaysRemaining; }
	bool GetWindowSuccess() const { return bWindowSuccess; }
	const TArray<FPregnantRecord>& GetPregnant() const { return Pregnant; }

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

private:
	FStation& Station;
	FEventBus& Bus;

	int32 WindowOpenForYear = -1;
	int32 WindowDaysRemaining = 0;
	bool bWindowSuccess = false;
	TArray<FPregnantRecord> Pregnant;

	void AttemptConception(int32 CurrentYear);
	void Calf(const FPregnantRecord& Record);
};
