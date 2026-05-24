#include "Entities/StationEvent.h"

FStationEvent::FStationEvent(const FStationEventConfig& Config)
	: Id(Config.Id)
	, Type(Config.Type)
	, Kind(Config.Kind)
	, Severity(Config.Severity)
	, StartDay(Config.StartDay)
	, DurationDays(Config.DurationDays)
	, PaddockIds(Config.PaddockIds)
{
}

FString FStationEvent::SerializeJson() const
{
	FString PaddockIdsJson = TEXT("[");
	for (int32 I = 0; I < PaddockIds.Num(); I++)
	{
		if (I > 0) PaddockIdsJson += TEXT(",");
		PaddockIdsJson += FString::Printf(TEXT("\"%s\""), *PaddockIds[I]);
	}
	PaddockIdsJson += TEXT("]");

	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"type\":%d,\"kind\":%d,\"severity\":%.3f,\"startDay\":%d,\"durationDays\":%d,\"paddockIds\":%s,\"resolved\":%s,\"outcome\":\"%s\"}"),
		*Id, (int32)Type, (int32)Kind, Severity, StartDay, DurationDays,
		*PaddockIdsJson,
		bResolved ? TEXT("true") : TEXT("false"),
		*Outcome);
}

FStationEvent FStationEvent::FromJson(const FString& Json)
{
	FStationEventConfig Cfg;

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
	int32 TypeInt = 0, KindInt = 0;
	ReadInt(TEXT("type"), TypeInt);
	ReadInt(TEXT("kind"), KindInt);
	Cfg.Type = (EStationEventType)TypeInt;
	Cfg.Kind = (EStationEventKind)KindInt;
	ReadFloat(TEXT("severity"), Cfg.Severity);
	ReadInt(TEXT("startDay"), Cfg.StartDay);
	ReadInt(TEXT("durationDays"), Cfg.DurationDays);

	FStationEvent E(Cfg);
	const int32 RIdx = Json.Find(TEXT("\"resolved\":"));
	if (RIdx != INDEX_NONE)
	{
		E.bResolved = Json.Mid(RIdx + 11, 4).StartsWith(TEXT("true"));
	}
	ReadStr(TEXT("outcome"), E.Outcome);

	return E;
}
