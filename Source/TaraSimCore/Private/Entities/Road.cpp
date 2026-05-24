#include "Entities/Road.h"

FRoad::FRoad(const FRoadConfig& Config)
	: Id(Config.Id)
	, PaddockA(Config.PaddockA)
	, PaddockB(Config.PaddockB)
	, GradeQuality(Config.GradeQuality)
{
}

float FRoad::ClampGrade(float V)
{
	return FMath::Clamp(V, 0.0f, 100.0f);
}

bool FRoad::Connects(const FString& A, const FString& B) const
{
	return (PaddockA == A && PaddockB == B)
		|| (PaddockA == B && PaddockB == A);
}

FString FRoad::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"paddockA\":\"%s\",\"paddockB\":\"%s\",\"gradeQuality\":%.2f}"),
		*Id, *PaddockA, *PaddockB, GradeQuality);
}

FRoad FRoad::FromJson(const FString& Json)
{
	FRoadConfig Cfg;

	auto ReadStr = [&](const TCHAR* Key, FString& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":\""), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Token.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) Out = Json.Mid(Start, End - Start);
	};
	auto ReadFloat = [&](const TCHAR* Key, float& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		int32 P = Idx + Token.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '.' || Json[P] == '-')) P++;
		Out = FCString::Atof(*Json.Mid(Start, P - Start));
	};

	ReadStr(TEXT("id"), Cfg.Id);
	ReadStr(TEXT("paddockA"), Cfg.PaddockA);
	ReadStr(TEXT("paddockB"), Cfg.PaddockB);
	ReadFloat(TEXT("gradeQuality"), Cfg.GradeQuality);

	return FRoad(Cfg);
}
