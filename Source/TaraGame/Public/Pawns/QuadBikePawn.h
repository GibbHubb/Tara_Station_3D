#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "QuadBikePawn.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * AQuadBikePawn — Phase 1 player vehicle. A minimal arcade-feel quad bike:
 * WASD / left-stick = throttle + steer; jump + handbrake stubbed.
 *
 * NOT a full Chaos Vehicle. Phase 1 needs "drivable and fun enough to traverse
 * one paddock"; real vehicle physics arrives in Phase 2 when multi-paddock
 * terrain demands it.
 */
UCLASS()
class TARAGAME_API AQuadBikePawn : public APawn
{
	GENERATED_BODY()

public:
	AQuadBikePawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UCameraComponent> Camera;

	// Tunable feel — tweak after the first 30 seconds of driving.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Drive")
	float MaxSpeedCmPerSec = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Drive")
	float AccelCmPerSec2 = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Drive")
	float TurnDegreesPerSec = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Drive")
	float Friction = 1.5f;

	// Input assets — assigned in Blueprint subclass or default Editor settings.
	UPROPERTY(EditDefaultsOnly, Category = "Tara|Input")
	TObjectPtr<UInputAction> ThrottleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Tara|Input")
	TObjectPtr<UInputAction> SteerAction;

	UPROPERTY(EditDefaultsOnly, Category = "Tara|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Tara|Input")
	TObjectPtr<UInputAction> TickDayAction;

protected:
	virtual void BeginPlay() override;

	void OnThrottleInput(const struct FInputActionValue& Value);
	void OnSteerInput(const FInputActionValue& Value);
	void OnTickDayInput(const FInputActionValue& Value);

	float CurrentSpeed = 0.0f;
	float ThrottleInput = 0.0f;   // -1 .. +1 (W forward, S reverse)
	float SteerInput = 0.0f;      // -1 .. +1 (A left, D right)
};
