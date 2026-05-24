#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TaraHUDWidget.generated.h"

class UTextBlock;

/**
 * UTaraHUDWidget — Phase 1 HUD shown in the corner of the screen.
 * Displays: Year/Day/Season, herd size + condition, current paddock grass kg/ha.
 *
 * Subclass this as a Blueprint widget (`WBP_TaraHUD`) so a designer can lay
 * out the text positions / fonts / colours; this C++ base does the data bind.
 */
UCLASS(Abstract)
class TARAGAME_API UTaraHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Bind these text blocks in the Blueprint subclass.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DateTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Herd;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Paddock;

	// Paddock to display grass for (default: A = Home).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara")
	FString FocusedPaddockId = TEXT("A");

private:
	float SecondsSinceRefresh = 0.0f;
	void Refresh();
};
