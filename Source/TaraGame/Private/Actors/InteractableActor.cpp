#include "Actors/InteractableActor.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AInteractableActor::AInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	TriggerBox->SetGenerateOverlapEvents(true);

	PromptLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptLabel"));
	PromptLabel->SetupAttachment(RootComponent);
	PromptLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
	PromptLabel->SetHorizontalAlignment(EHTA_Center);
	PromptLabel->SetWorldSize(40.0f);
	PromptLabel->SetVisibility(false);
}

void AInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AInteractableActor::HandleOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AInteractableActor::HandleOverlapEnd);

	RefreshPrompt();
}

bool AInteractableActor::OnInteract_Implementation(APawn* InstigatorPawn)
{
	// Default no-op; subclasses override.
	return false;
}

bool AInteractableActor::IsInteractAvailable_Implementation() const
{
	return true;
}

void AInteractableActor::HandleOverlapBegin(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (APawn* AsPawn = Cast<APawn>(OtherActor))
	{
		// Only care about player Pawns — skip cattle / wildlife.
		if (AsPawn->IsPlayerControlled())
		{
			CurrentOverlappingPawn = AsPawn;
			RefreshPrompt();
		}
	}
}

void AInteractableActor::HandleOverlapEnd(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	if (APawn* AsPawn = Cast<APawn>(OtherActor))
	{
		if (CurrentOverlappingPawn.Get() == AsPawn)
		{
			CurrentOverlappingPawn.Reset();
			RefreshPrompt();
		}
	}
}

void AInteractableActor::RefreshPrompt()
{
	if (!PromptLabel) return;

	const bool bShouldShow =
		CurrentOverlappingPawn.IsValid() && IsInteractAvailable();

	PromptLabel->SetVisibility(bShouldShow);
	if (bShouldShow)
	{
		PromptLabel->SetText(FText::Format(
			NSLOCTEXT("Tara", "InteractPromptFmt", "[E] {0}"), PromptVerb));
	}
}

bool AInteractableActor::TryInteractWithNearest(APawn* InstigatorPawn)
{
	if (!InstigatorPawn) return false;
	UWorld* World = InstigatorPawn->GetWorld();
	if (!World) return false;

	AInteractableActor* Best = nullptr;
	float BestDistSq = FLT_MAX;
	const FVector PawnLoc = InstigatorPawn->GetActorLocation();

	for (TActorIterator<AInteractableActor> It(World); It; ++It)
	{
		AInteractableActor* A = *It;
		if (!A || !A->IsInteractAvailable()) continue;
		if (A->CurrentOverlappingPawn.Get() != InstigatorPawn) continue;

		const float D = FVector::DistSquared(A->GetActorLocation(), PawnLoc);
		if (D < BestDistSq)
		{
			BestDistSq = D;
			Best = A;
		}
	}

	return Best ? Best->OnInteract(InstigatorPawn) : false;
}
