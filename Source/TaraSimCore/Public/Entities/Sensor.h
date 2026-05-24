#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Sensor.ts. M8 sensor network — rain gauges /
// tank-flow / bore-pressure. Let the player see station state without
// driving the whole property. In 3D (Phase 5+), readings surface in a
// homestead-wall screen or the player's in-world phone.

enum class ESensorKind : uint8
{
	RainGauge    = 0,
	TankFlow     = 1,
	BorePressure = 2,
};

struct FSensorProfile
{
	int32 PurchasePrice = 0;
	const TCHAR* Description = TEXT("");
};

TARASIMCORE_API FSensorProfile GetSensorProfile(ESensorKind Kind);
TARASIMCORE_API FString SensorKindToString(ESensorKind Kind);
TARASIMCORE_API bool TryParseSensorKind(const FString& Name, ESensorKind& OutKind);

struct FSensorConfig
{
	FString Id;
	ESensorKind Kind = ESensorKind::RainGauge;
	FString LocationId;        // paddockId or boreId
	bool bInstalled = false;
	int32 BatteryDays = 180;
};

class TARASIMCORE_API FSensor
{
public:
	FSensor() = default;
	explicit FSensor(const FSensorConfig& Config);

	FString SerializeJson() const;
	static FSensor FromJson(const FString& Json);

public:
	FString Id;
	ESensorKind Kind = ESensorKind::RainGauge;
	FString LocationId;
	bool bInstalled = false;
	int32 BatteryDays = 180;
	FString LastReading = TEXT("—");
};
