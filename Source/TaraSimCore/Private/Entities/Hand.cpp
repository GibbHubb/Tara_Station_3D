#include "Entities/Hand.h"

FHand::FHand(const FHandConfig& Config)
	: Id(Config.Id)
	, Name(Config.Name)
	, Skill(Config.Skill)
	, WagePerDay(Config.WagePerDay)
	, bHired(Config.bHired)
{
}

FString FHand::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"name\":\"%s\",\"skill\":%d,\"wagePerDay\":%d,\"hired\":%s}"),
		*Id, *Name, Skill, WagePerDay, bHired ? TEXT("true") : TEXT("false"));
}

FHand FHand::FromJson(const FString& Json)
{
	FHandConfig Cfg;

	auto ReadStr = [&](const TCHAR* Key, FString& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":\""), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Token.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) Out = Json.Mid(Start, End - Start);
	};
	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		int32 P = Idx + Token.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		Out = FCString::Atoi(*Json.Mid(Start, P - Start));
	};

	ReadStr(TEXT("id"), Cfg.Id);
	ReadStr(TEXT("name"), Cfg.Name);
	ReadInt(TEXT("skill"), Cfg.Skill);
	ReadInt(TEXT("wagePerDay"), Cfg.WagePerDay);

	const int32 HiredIdx = Json.Find(TEXT("\"hired\":"));
	if (HiredIdx != INDEX_NONE)
	{
		const FString Tail = Json.Mid(HiredIdx + 8, 4);
		Cfg.bHired = Tail.StartsWith(TEXT("true"));
	}

	return FHand(Cfg);
}
