#include "UI/RadioBanner.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Engine/World.h"

void URadioBanner::NativeConstruct()
{
	Super::NativeConstruct();
	// Hidden by default; ShowMessage flips visible + sets the auto-hide timer.
	SetVisibility(ESlateVisibility::Hidden);
}

void URadioBanner::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}
	Super::NativeDestruct();
}

void URadioBanner::ShowMessage(const FString& Speaker, const FString& Body)
{
	if (SpeakerText)
	{
		SpeakerText->SetText(FText::FromString(Speaker));
	}

	if (MessageText)
	{
		// If no separate SpeakerText widget exists, combine speaker + body into
		// one line: "[Speaker]: body"
		const FString Combined = SpeakerText
			? Body
			: FString::Printf(TEXT("[%s]: %s"), *Speaker, *Body);
		MessageText->SetText(FText::FromString(Combined));
	}

	SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().SetTimer(HideTimer, this,
			&URadioBanner::HandleAutoHide,
			FMath::Max(0.5f, MessageDurationSeconds), false);
	}
}

void URadioBanner::HandleAutoHide()
{
	SetVisibility(ESlateVisibility::Hidden);
}
