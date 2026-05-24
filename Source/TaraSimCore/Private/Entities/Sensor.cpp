#include "Entities/Sensor.h"

FSensorProfile GetSensorProfile(ESensorKind Kind)
{
	switch (Kind)
	{
	case ESensorKind::RainGauge:    return { 350, TEXT("Live mm rainfall per paddock") };
	case ESensorKind::TankFlow:     return { 600, TEXT("Per-tank fill rate + level") };
	case ESensorKind::BorePressure: return { 800, TEXT("Predicts bore failure before it hits") };
	default: return { 0, TEXT("") };
	}
}

FString SensorKindToString(ESensorKind Kind)
{
	switch (Kind)
	{
	case ESensorKind::RainGauge:    return TEXT("rain-gauge");
	case ESensorKind::TankFlow:     return TEXT("tank-flow");
	case ESensorKind::BorePressure: return TEXT("bore-pressure");
	default: return TEXT("unknown");
	}
}

bool TryParseSensorKind(const FString& Name, ESensorKind& OutKind)
{
	if (Name == TEXT("rain-gauge")) { OutKind = ESensorKind::RainGauge; return true; }
	if (Name == TEXT("tank-flow")) { OutKind = ESensorKind::TankFlow; return true; }
	if (Name == TEXT("bore-pressure")) { OutKind = ESensorKind::BorePressure; return true; }
	return false;
}

FSensor::FSensor(const FSensorConfig& Config)
	: Id(Config.Id)
	, Kind(Config.Kind)
	, LocationId(Config.LocationId)
	, bInstalled(Config.bInstalled)
	, BatteryDays(Config.BatteryDays)
{
}

FString FSensor::SerializeJson() const
{
	// Escape last-reading minimally — replace inner quotes/newlines with safe forms.
	FString Reading = LastReading.Replace(TEXT("\""), TEXT("'"));
	Reading.ReplaceInline(TEXT("\n"), TEXT(" "));
	return FString::Printf(
		TEXT("{\"id\":\"%s\",\"kind\":%d,\"locationId\":\"%s\",\"installed\":%s,\"batteryDays\":%d,\"lastReading\":\"%s\"}"),
		*Id, (int32)Kind, *LocationId,
		bInstalled ? TEXT("true") : TEXT("false"),
		BatteryDays, *Reading);
}

FSensor FSensor::FromJson(const FString& Json)
{
	FSensorConfig Cfg;
	FSensor S;

	auto ReadStr = [&](const TCHAR* Key, FString& Out)
	{
		const FString Tok = FString::Printf(TEXT("\"%s\":\""), Key);
		const int32 Idx = Json.Find(Tok);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Tok.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) Out = Json.Mid(Start, End - Start);
	};
	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString T = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 II = Json.Find(T);
		if (II == INDEX_NONE) return;
		int32 P = II + T.Len();
		int32 St = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		Out = FCString::Atoi(*Json.Mid(St, P - St));
	};

	ReadStr(TEXT("id"), Cfg.Id);
	int32 KindInt = 0;
	ReadInt(TEXT("kind"), KindInt);
	Cfg.Kind = (ESensorKind)KindInt;
	ReadStr(TEXT("locationId"), Cfg.LocationId);
	ReadInt(TEXT("batteryDays"), Cfg.BatteryDays);
	const int32 IIdx = Json.Find(TEXT("\"installed\":"));
	if (IIdx != INDEX_NONE)
	{
		Cfg.bInstalled = Json.Mid(IIdx + 12, 4).StartsWith(TEXT("true"));
	}

	S = FSensor(Cfg);
	ReadStr(TEXT("lastReading"), S.LastReading);
	if (S.LastReading.IsEmpty()) S.LastReading = TEXT("—");
	return S;
}
