#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/CattleCohort.ts. The hybrid-cohort model the 2D
// project locked in on agree TS8-10: each cohort = one birth year's worth of
// cattle as an aggregate (count + condition stats + lifecycle state).
//
// In 3D, the cohort is still the source-of-truth count; the visible cattle
// in the world are spawned by ACattleActor as a subset drawn from the cohort
// (typically 10-30 visible head per cohort regardless of true count — looks
// right at any density via instancing).

enum class ECohortState : uint8
{
	Unweaned,
	Weaner,
	Steer,
	Heifer,
	Breeder,
	Old,
};

enum class EBehaviourProfile : uint8
{
	Calm,
	Mixed,
	Spooky,
	Wild,
};

struct FCattleCohortConfig
{
	int32 BirthYear = 0;
	int32 Count = 8;
	float SexRatio = 0.5f;          // female fraction
	float ConditionMean = 80.0f;    // 0..100
	float ConditionVariance = 10.0f;
	ECohortState State = ECohortState::Weaner;
	EBehaviourProfile BehaviourProfile = EBehaviourProfile::Calm;
};

class TARASIMCORE_API FCattleCohort
{
public:
	explicit FCattleCohort(const FCattleCohortConfig& Config);

	static float ClampCondition(float V);

	// 0..1 difficulty score for mustering (drives breakaway risk + extra days
	// in the 2D MusterSystem; same hooks in 3D's spatial mustering).
	float DifficultyFactor() const;

	FString SerializeJson() const;
	static FCattleCohort FromJson(const FString& Json);

public:
	int32 BirthYear = 0;
	int32 Count = 8;
	float SexRatio = 0.5f;
	float ConditionMean = 80.0f;
	float ConditionVariance = 10.0f;
	ECohortState State = ECohortState::Weaner;
	EBehaviourProfile BehaviourProfile = EBehaviourProfile::Calm;
	int32 ConsecutiveStarvationDays = 0;
};
