#include "Entities/Fence.h"

FFence::FFence(const FFenceConfig& Config)
	: Id(Config.Id)
	, PaddockA(Config.PaddockA)
	, PaddockB(Config.PaddockB)
	, Integrity(Config.Integrity)
{
}

float FFence::ClampIntegrity(float V)
{
	return FMath::Clamp(V, 0.0f, 100.0f);
}

bool FFence::Connects(const FString& A, const FString& B) const
{
	return (PaddockA == A && PaddockB == B)
		|| (PaddockA == B && PaddockB == A);
}

FString FFence::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"paddockA\":\"%s\",\"paddockB\":\"%s\",\"integrity\":%.2f}"),
		*Id, *PaddockA, *PaddockB, Integrity);
}

FFence FFence::FromJson(const FString& Json)
{
	FFenceConfig Cfg;

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
	ReadFloat(TEXT("integrity"), Cfg.Integrity);

	return FFence(Cfg);
}
