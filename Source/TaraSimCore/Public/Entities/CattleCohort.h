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

// Pregnancy stage for preg-test grouping — populated by BreedingSystem when
// pregnancies advance through gestation thirds. Per CORE_LOOP §2: "preg-test
// the cows, separate by stage of pregnancy, and put similarly-pregnant cows
// in matched paddocks." Stage tags carry on the cohort so preg-test gate
// drafting (Phase 5+) can sort the mob.
enum class EBreederStage : uint8
{
	NotPregnant = 0,
	Early       = 1,   // 0..93 days   (~first trimester equivalent)
	Mid         = 2,   // 94..187 days (~second)
	Late        = 3,   // 188..280 days (~third)
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
	float MusterTrainedness = 0.0f; // 0..100 — see field doc below
	EBreederStage BreederStage = EBreederStage::NotPregnant;
	bool bLactating = false;        // see field doc below
	int32 BrandedDay = 0;           // see field doc below
	bool bHorned = false;            // see field doc below
};

class TARASIMCORE_API FCattleCohort
{
public:
	explicit FCattleCohort(const FCattleCohortConfig& Config);

	static float ClampCondition(float V);

	// 0..1 difficulty score for mustering (drives breakaway risk + extra days
	// in the 2D MusterSystem; same hooks in 3D's spatial mustering). Reflects
	// behaviour profile only.
	float DifficultyFactor() const;

	// Combines DifficultyFactor with MusterTrainedness — well-schooled
	// cohorts (high MusterTrainedness) reduce effective difficulty up to
	// halve at 100. The 3D spatial mustering AI reads this when picking
	// flight-zone widths and breakaway probabilities. At Trainedness 0 →
	// base difficulty unchanged; at 50 (default) → 75% of base; at 100 →
	// half. The 2D project's computeMuster has no equivalent — this is the
	// 3D version of "well-trained weaners are easier to muster years later"
	// per CORE_LOOP §2 + TS-3D-WEANER-SCHOOL design.
	float EffectiveMusterDifficulty() const;

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

	// Per CORE_LOOP §2 + TS-3D-WEANER-SCHOOL design: well-schooled weaners
	// muster faster + cleaner years later. This stat rises over in-game days
	// while a cohort is held as a mob in weaner-school (Phase 5+ sub-scene)
	// and persists through life. Higher = easier to muster (cohort applies a
	// reduction to base muster days + breakaway risk when the 3D layer's
	// spatial AI reads this value). Starts at 0 for freshly-weaned cohorts
	// + 50 (default) for older cohorts to avoid penalising play before the
	// weaner-school sub-scene exists.
	float MusterTrainedness = 50.0f;

	// Pregnancy stage — preg-test sub-scene (TS-3D-PREG-TEST) reads this to
	// route cows through gates by stage. BreedingSystem advances it as
	// gestation progresses; resets to NotPregnant after calving.
	EBreederStage BreederStage = EBreederStage::NotPregnant;

	// Per CORE_LOOP §2 + TS-3D-WEANER-SCHOOL design: while a breeder cohort
	// has dependent calves, it is lactating. ConditionSystem applies a
	// -0.2/day extra drift on the cohort (the cost of producing milk). The
	// wean event (Station::WeanCohort) flips this false + the cohort's
	// condition recovers naturally. Body-score-protection IS this flag
	// switching off — that's why weaning timing matters per CORE_LOOP §2.
	bool bLactating = false;

	// Day-of-year the cohort was last branded (TS-3D-BRANDING). 0 = never
	// branded. The year-end progression score bonuses a fully-branded year.
	int32 BrandedDay = 0;

	// Set ~30% of the time when BreedingSystem creates a new Unweaned cohort
	// (mixed-breed default; tune per cohort lineage in Phase 5+). Drives the
	// dehorning step at branding; cleared once dehorned.
	bool bHorned = false;
};
