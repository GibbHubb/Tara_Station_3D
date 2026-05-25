#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RadioBanner.generated.h"

class UTextBlock;
class UTaraSimSubsystem;
struct FRadioMessageReceivedPayload;   // forward; payload landing in TS-3D-RADIO
                                        // execute step (FRadioMessage sim entity)

// Base for WBP_RadioBanner (TS-3D-RADIO plan §6). Top-of-screen overlay that
// fades in for ~8s when a radio message lands, then fades out. Subscribes to
// a bridge-side OnRadioMessageReceived delegate that TS-3D-RADIO execute will
// add. Until that wiring is shipped, the widget compiles + can be tested via
// the public `ShowMessage(speaker, body)` Blueprint-callable.

UCLASS(Abstract, Blueprintable)
class TARAGAME_API URadioBanner : public UUserWidget
{
	GENERATED_BODY()

public:
	// Show a message immediately. Use this from BP for debug + as the wire-up
	// target for the OnRadioMessageReceived delegate once TS-3D-RADIO ships.
	UFUNCTION(BlueprintCallable, Category = "Tara|Radio")
	void ShowMessage(const FString& Speaker, const FString& Body);

	// How long the banner stays visible per message. Tunable in BP defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tara|Radio")
	float MessageDurationSeconds = 8.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// BindWidget — required: a TextBlock for the speaker + body line.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText = nullptr;

	// Optional — a second TextBlock if you want speaker styling separate.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerText = nullptr;

private:
	FTimerHandle HideTimer;

	// Called by the auto-hide timer.
	UFUNCTION()
	void HandleAutoHide();
};
