#include "Pawns/QuadBikePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AQuadBikePawn::AQuadBikePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	RootComponent = BodyMesh;
	BodyMesh->SetRelativeScale3D(FVector(1.6f, 1.0f, 0.6f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AQuadBikePawn::BeginPlay()
{
	Super::BeginPlay();

	// Push the mapping context onto the local player's input subsystem.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSys =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (MappingContext)
			{
				InputSys->AddMappingContext(MappingContext, 0);
			}
		}
	}
}

void AQuadBikePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Throttle accel / friction decel.
	CurrentSpeed += ThrottleInput * AccelCmPerSec2 * DeltaSeconds;
	if (FMath::IsNearlyZero(ThrottleInput))
	{
		const float Sign = FMath::Sign(CurrentSpeed);
		CurrentSpeed -= Sign * Friction * AccelCmPerSec2 * DeltaSeconds;
		if (Sign != FMath::Sign(CurrentSpeed)) CurrentSpeed = 0.0f;
	}
	CurrentSpeed = FMath::Clamp(CurrentSpeed, -MaxSpeedCmPerSec * 0.4f, MaxSpeedCmPerSec);

	// Apply steering only when moving (so you don't spin in place).
	if (!FMath::IsNearlyZero(CurrentSpeed))
	{
		const float Yaw = SteerInput * TurnDegreesPerSec * DeltaSeconds * FMath::Sign(CurrentSpeed);
		AddActorLocalRotation(FRotator(0.0f, Yaw, 0.0f));
	}

	// Move forward in actor-local X.
	const FVector Delta = GetActorForwardVector() * CurrentSpeed * DeltaSeconds;
	AddActorWorldOffset(Delta, true /* sweep */);
}

void AQuadBikePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ThrottleAction) EI->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AQuadBikePawn::OnThrottleInput);
		if (ThrottleAction) EI->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AQuadBikePawn::OnThrottleInput);
		if (SteerAction)    EI->BindAction(SteerAction,    ETriggerEvent::Triggered, this, &AQuadBikePawn::OnSteerInput);
		if (SteerAction)    EI->BindAction(SteerAction,    ETriggerEvent::Completed, this, &AQuadBikePawn::OnSteerInput);
		if (TickDayAction)  EI->BindAction(TickDayAction,  ETriggerEvent::Started,   this, &AQuadBikePawn::OnTickDayInput);
	}
}

void AQuadBikePawn::OnThrottleInput(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();
}

void AQuadBikePawn::OnSteerInput(const FInputActionValue& Value)
{
	SteerInput = Value.Get<float>();
}

void AQuadBikePawn::OnTickDayInput(const FInputActionValue& /*Value*/)
{
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UTaraSimSubsystem* Sim = GI->GetSubsystem<UTaraSimSubsystem>())
		{
			Sim->TickDay();
		}
	}
}
