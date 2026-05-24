#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Road.ts. Road between two adjacent paddocks.
// Quality decays from weather + cattle travel. Wheeled vehicles (2-wheeler,
// 4-wheeler, buggy) suffer a time penalty on rough roads — see
// FVehicle::IsWheeled.
//
// Audit: 🟢 — entity carries forward. Maintenance via grader (M8 work machine)
// restores gradeQuality.

struct FRoadConfig
{
	FString Id;
	FString PaddockA;
	FString PaddockB;
	float GradeQuality = 60.0f;
};

class TARASIMCORE_API FRoad
{
public:
	explicit FRoad(const FRoadConfig& Config);

	static float ClampGrade(float V);
	bool Connects(const FString& A, const FString& B) const;

	FString SerializeJson() const;
	static FRoad FromJson(const FString& Json);

public:
	FString Id;
	FString PaddockA;
	FString PaddockB;
	float GradeQuality = 60.0f;
};
