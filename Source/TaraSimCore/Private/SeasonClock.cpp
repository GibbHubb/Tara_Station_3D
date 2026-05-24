#include "SeasonClock.h"

void FSeasonClock::TickDay()
{
	DayOfYear += 1;
	if (DayOfYear > DaysPerYear)
	{
		DayOfYear = 1;
		Year += 1;
	}
	Hour = WakeHour;
	Minute = 0;
}

bool FSeasonClock::AdvanceMinutes(int32 Minutes)
{
	if (Minutes <= 0) return false;

	Minute += Minutes;
	while (Minute >= 60)
	{
		Minute -= 60;
		Hour += 1;
	}
	if (Hour >= SleepHour)
	{
		// Sleep boundary hit — reset clock to next morning's WakeHour. The
		// CALLER is responsible for running Station::TickDay() if it wants
		// the sim day-tick to fire (i.e. all systems advance). This is the
		// contract the bridge expects: AdvanceMinutes returns true → bridge
		// calls TickDay → TickDay's DayStarted event emits with the OLD day
		// before Clock.TickDay() bumps DayOfYear and runs systems for the
		// new day. (Compare: if AdvanceMinutes auto-bumped DayOfYear, the
		// DayStarted event in TickDay would emit the stale-new day.)
		Hour = WakeHour;
		Minute = 0;
		return true;
	}
	return false;
}

bool FSeasonClock::AdvanceRealTime(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f) return false;

	const float SecPerMin = RealSecondsPerInGameMinute();
	if (SecPerMin <= 0.0f) return false;

	RealTimeAccumulatorSeconds += DeltaSeconds;
	const int32 WholeMinutes = (int32)(RealTimeAccumulatorSeconds / SecPerMin);
	if (WholeMinutes <= 0) return false;

	RealTimeAccumulatorSeconds -= WholeMinutes * SecPerMin;
	return AdvanceMinutes(WholeMinutes);
}

ESeason FSeasonClock::GetSeason() const
{
	// Dry season Mar–Nov (days 60–334); Wet season Dec–Feb (335+, 1–59).
	return (DayOfYear >= 335 || DayOfYear <= 59) ? ESeason::Wet : ESeason::Dry;
}

FString FSeasonClock::FormatTime() const
{
	return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
}

FString FSeasonClock::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"year\":%d,\"dayOfYear\":%d,\"hour\":%d,\"minute\":%d}"),
		Year, DayOfYear, Hour, Minute);
}

FSeasonClock FSeasonClock::FromJson(const FString& Json)
{
	// Phase 0 minimal: parse with FString manipulation. Phase 1 can swap in
	// FJsonObjectConverter or hand-written FJsonReader. Pure-sim discipline —
	// JsonUtilities lives in Engine, so we keep it out of TaraSimCore.
	FSeasonClock C;

	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Index = Json.Find(Token);
		if (Index == INDEX_NONE) return;
		const int32 ValueStart = Index + Token.Len();
		int32 ValueEnd = ValueStart;
		while (ValueEnd < Json.Len() && (FChar::IsDigit(Json[ValueEnd]) || Json[ValueEnd] == '-'))
		{
			ValueEnd++;
		}
		const FString Slice = Json.Mid(ValueStart, ValueEnd - ValueStart);
		Out = FCString::Atoi(*Slice);
	};

	ReadInt(TEXT("year"), C.Year);
	ReadInt(TEXT("dayOfYear"), C.DayOfYear);
	ReadInt(TEXT("hour"), C.Hour);
	ReadInt(TEXT("minute"), C.Minute);
	return C;
}
