#include "Entities/CattleCohort.h"

FCattleCohort::FCattleCohort(const FCattleCohortConfig& Config)
	: BirthYear(Config.BirthYear)
	, Count(Config.Count)
	, SexRatio(Config.SexRatio)
	, ConditionMean(Config.ConditionMean)
	, ConditionVariance(Config.ConditionVariance)
	, State(Config.State)
	, BehaviourProfile(Config.BehaviourProfile)
	, MusterTrainedness(Config.MusterTrainedness)
	, BreederStage(Config.BreederStage)
	, bLactating(Config.bLactating)
	, BrandedDay(Config.BrandedDay)
	, bHorned(Config.bHorned)
{
}

float FCattleCohort::ClampCondition(float V)
{
	return FMath::Clamp(V, 0.0f, 100.0f);
}

float FCattleCohort::DifficultyFactor() const
{
	switch (BehaviourProfile)
	{
	case EBehaviourProfile::Calm:   return 0.0f;
	case EBehaviourProfile::Mixed:  return 0.2f;
	case EBehaviourProfile::Spooky: return 0.4f;
	case EBehaviourProfile::Wild:   return 0.7f;
	default: return 0.0f;
	}
}

float FCattleCohort::EffectiveMusterDifficulty() const
{
	const float Base = DifficultyFactor();
	const float TrainingNorm = FMath::Clamp(MusterTrainedness, 0.0f, 100.0f) / 100.0f;
	const float TrainingMul = 1.0f - TrainingNorm * 0.5f;  // 1.0 at T=0 → 0.5 at T=100
	return Base * TrainingMul;
}

FString FCattleCohort::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"birthYear\":%d,\"count\":%d,\"sexRatio\":%.3f,\"conditionMean\":%.2f,\"conditionVariance\":%.2f,\"state\":%d,\"behaviourProfile\":%d,\"consecutiveStarvationDays\":%d,\"musterTrainedness\":%.2f,\"breederStage\":%d,\"lactating\":%s,\"brandedDay\":%d,\"horned\":%s}"),
		BirthYear, Count, SexRatio, ConditionMean, ConditionVariance,
		(int32)State, (int32)BehaviourProfile, ConsecutiveStarvationDays,
		MusterTrainedness, (int32)BreederStage,
		bLactating ? TEXT("true") : TEXT("false"),
		BrandedDay,
		bHorned ? TEXT("true") : TEXT("false"));
}

FCattleCohort FCattleCohort::FromJson(const FString& Json)
{
	FCattleCohortConfig Cfg;
	FCattleCohort Result(Cfg);

	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Index = Json.Find(Token);
		if (Index == INDEX_NONE) return;
		const int32 Start = Index + Token.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '-'))
		{
			End++;
		}
		Out = FCString::Atoi(*Json.Mid(Start, End - Start));
	};
	auto ReadFloat = [&](const TCHAR* Key, float& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Index = Json.Find(Token);
		if (Index == INDEX_NONE) return;
		const int32 Start = Index + Token.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '.' || Json[End] == '-'))
		{
			End++;
		}
		Out = FCString::Atof(*Json.Mid(Start, End - Start));
	};

	ReadInt(TEXT("birthYear"), Result.BirthYear);
	ReadInt(TEXT("count"), Result.Count);
	ReadFloat(TEXT("sexRatio"), Result.SexRatio);
	ReadFloat(TEXT("conditionMean"), Result.ConditionMean);
	ReadFloat(TEXT("conditionVariance"), Result.ConditionVariance);

	int32 StateInt = 0, BehaviourInt = 0;
	ReadInt(TEXT("state"), StateInt);
	ReadInt(TEXT("behaviourProfile"), BehaviourInt);
	Result.State = (ECohortState)StateInt;
	Result.BehaviourProfile = (EBehaviourProfile)BehaviourInt;

	ReadInt(TEXT("consecutiveStarvationDays"), Result.ConsecutiveStarvationDays);

	// New fields default to whatever the in-memory struct already has — JSON
	// keys absent from older saves leave fields at the FCattleCohortConfig
	// defaults set in the ctor.
	ReadFloat(TEXT("musterTrainedness"), Result.MusterTrainedness);
	int32 StageInt = (int32)Result.BreederStage;
	ReadInt(TEXT("breederStage"), StageInt);
	Result.BreederStage = (EBreederStage)StageInt;

	// Phase 5+ fields — forward-compat: absent JSON keys leave the ctor defaults.
	auto ReadBool = [&](const TCHAR* Key, bool& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		Out = Json.Mid(Idx + Token.Len(), 4).StartsWith(TEXT("true"));
	};
	ReadBool(TEXT("lactating"), Result.bLactating);
	ReadInt(TEXT("brandedDay"), Result.BrandedDay);
	ReadBool(TEXT("horned"), Result.bHorned);

	return Result;
}
