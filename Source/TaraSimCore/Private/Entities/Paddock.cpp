#include "Entities/Paddock.h"

FPaddock::FPaddock(const FPaddockConfig& Config)
	: Id(Config.Id)
	, Name(Config.Name)
	, AreaHa(Config.AreaHa)
	, GrassKgPerHa(Config.StartingGrassKgPerHa)
{
}

void FPaddock::SimulateDay(ESeason Season, int32 HerdInPaddock, float GrowthMultiplier)
{
	const float SeasonGrowth = (Season == ESeason::Wet ? WetGrowthKgPerHa : DryGrowthKgPerHa);
	const float Growth = SeasonGrowth * GrowthMultiplier;

	const float WantIntake = (HerdInPaddock * DailyIntakeKgPerHead) / FMath::Max(AreaHa, 1.0f);
	const float IntakeKgPerHa = FMath::Min(WantIntake, MaxIntakeKgPerHa);

	GrassKgPerHa = FMath::Clamp(GrassKgPerHa + Growth - IntakeKgPerHa, 0.0f, MaxGrassKgPerHa);
}

float FPaddock::GrassRatio() const
{
	return GrassKgPerHa / MaxGrassKgPerHa;
}

FString FPaddock::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"name\":\"%s\",\"areaHa\":%.2f,\"grassKgPerHa\":%.2f}"),
		*Id, *Name, AreaHa, GrassKgPerHa);
}

FPaddock FPaddock::FromJson(const FString& Json)
{
	// Same hand-rolled parser pattern as SeasonClock — keeps the sim free of
	// JsonUtilities (which lives in Engine modules). Production-quality JSON
	// parsing without engine dependencies is a sub-task; this minimal parser
	// is enough for Phase 0.
	FPaddockConfig Cfg;

	auto ReadString = [&](const TCHAR* Key, FString& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":\""), Key);
		const int32 Index = Json.Find(Token);
		if (Index == INDEX_NONE) return;
		const int32 Start = Index + Token.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End == INDEX_NONE) return;
		Out = Json.Mid(Start, End - Start);
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

	ReadString(TEXT("id"), Cfg.Id);
	ReadString(TEXT("name"), Cfg.Name);
	ReadFloat(TEXT("areaHa"), Cfg.AreaHa);
	ReadFloat(TEXT("grassKgPerHa"), Cfg.StartingGrassKgPerHa);

	return FPaddock(Cfg);
}
