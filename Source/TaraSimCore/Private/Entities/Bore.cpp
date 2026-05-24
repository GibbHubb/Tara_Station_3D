#include "Entities/Bore.h"

FBore::FBore(const FBoreConfig& Config)
	: Id(Config.Id)
	, ServesPaddockIds(Config.ServesPaddockIds)
	, Condition(Config.Condition)
	, Status(Config.Status)
{
}

float FBore::ClampCondition(float V)
{
	return FMath::Clamp(V, 0.0f, 100.0f);
}

FString FBore::SerializeJson() const
{
	FString ServesJson = TEXT("[");
	for (int32 I = 0; I < ServesPaddockIds.Num(); I++)
	{
		if (I > 0) ServesJson += TEXT(",");
		ServesJson += FString::Printf(TEXT("\"%s\""), *ServesPaddockIds[I]);
	}
	ServesJson += TEXT("]");

	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"servesPaddockIds\":%s,\"condition\":%.2f,\"status\":%d,\"lastCheckedDay\":%d}"),
		*Id, *ServesJson, Condition, (int32)Status, LastCheckedDay);
}

FBore FBore::FromJson(const FString& Json)
{
	FBoreConfig Cfg;

	// Pull id.
	const FString IdToken = TEXT("\"id\":\"");
	const int32 IdIdx = Json.Find(IdToken);
	if (IdIdx != INDEX_NONE)
	{
		const int32 Start = IdIdx + IdToken.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) Cfg.Id = Json.Mid(Start, End - Start);
	}

	// Pull servesPaddockIds array.
	const FString ServesToken = TEXT("\"servesPaddockIds\":[");
	const int32 ServesIdx = Json.Find(ServesToken);
	if (ServesIdx != INDEX_NONE)
	{
		int32 P = ServesIdx + ServesToken.Len();
		while (P < Json.Len())
		{
			if (Json[P] == ']') break;
			if (Json[P] == '"')
			{
				const int32 StrStart = P + 1;
				const int32 StrEnd = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, StrStart);
				if (StrEnd == INDEX_NONE) break;
				Cfg.ServesPaddockIds.Add(Json.Mid(StrStart, StrEnd - StrStart));
				P = StrEnd + 1;
			}
			else
			{
				P++;
			}
		}
	}

	// Pull condition (float).
	const FString CondToken = TEXT("\"condition\":");
	const int32 CondIdx = Json.Find(CondToken);
	if (CondIdx != INDEX_NONE)
	{
		const int32 Start = CondIdx + CondToken.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '.' || Json[End] == '-')) End++;
		Cfg.Condition = FCString::Atof(*Json.Mid(Start, End - Start));
	}

	// Pull status (int → enum).
	const FString StatusToken = TEXT("\"status\":");
	const int32 StatusIdx = Json.Find(StatusToken);
	if (StatusIdx != INDEX_NONE)
	{
		const int32 Start = StatusIdx + StatusToken.Len();
		int32 End = Start;
		while (End < Json.Len() && FChar::IsDigit(Json[End])) End++;
		Cfg.Status = (EBoreStatus)FCString::Atoi(*Json.Mid(Start, End - Start));
	}

	FBore Bore(Cfg);

	// lastCheckedDay.
	const FString LastToken = TEXT("\"lastCheckedDay\":");
	const int32 LastIdx = Json.Find(LastToken);
	if (LastIdx != INDEX_NONE)
	{
		const int32 Start = LastIdx + LastToken.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '-')) End++;
		Bore.LastCheckedDay = FCString::Atoi(*Json.Mid(Start, End - Start));
	}

	return Bore;
}
