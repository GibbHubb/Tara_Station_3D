#pragma once

#include "CoreMinimal.h"
#include "../Entities/Sensor.h"

class FStation;
class FEventBus;

// Mirrors src/sim/systems/SensorSystem.ts. Ticks installed sensor batteries
// down 1/day and generates a per-kind reading string for the HUD. In 3D the
// readings surface on a homestead-wall screen or the player's in-world
// "phone" pull-up (M8 hub).

class TARASIMCORE_API FSensorSystem
{
public:
	FSensorSystem(FStation& InStation, FEventBus& InBus);

	void TickDay();

	// Install a fresh sensor at the location. Mirrors 2D — auto-generates id.
	bool InstallSensor(ESensorKind Kind, const FString& LocationId);

	// Swap battery (resets to 180 days). Returns false on unknown sensor.
	bool SwapBattery(const FString& SensorId);

	const TArray<FSensor>& GetSensors() const { return Sensors; }
	TArray<FSensor>& GetSensorsMutable() { return Sensors; }

	const FSensor* SensorById(const FString& Id) const;

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

	static constexpr int32 DefaultBatteryDays = 180;

private:
	FStation& Station;
	FEventBus& Bus;
	TArray<FSensor> Sensors;

	int32 InstallCounter = 1;   // for unique-id generation
};
