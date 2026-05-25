#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TaraHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * UTaraHUDWidget — main player HUD. Refreshed at ~4 Hz (every 0.25s) by
 * NativeTick reading from `UTaraSimSubsystem`.
 *
 * **Subclass this as `WBP_TaraHUD`** in the Editor — bind each TextBlock /
 * ProgressBar by name (matching the `BindWidget` UPROPERTY names below);
 * arrange layout/colours/fonts in the Designer; the C++ base handles all
 * data binding.
 *
 * All bindings are **`BindWidgetOptional`** — if you don't want a stat shown,
 * just don't add that TextBlock to the WBP. The widget will skip missing
 * bindings silently. This keeps the HUD flexible per phase: Phase 1 might
 * only show DateTime + Herd + Paddock; Phase 5+ can light up everything.
 *
 * Covers the full 8/8 sim readout:
 *   - M1: Year/Day/Time/Season, herd size + condition, paddock grass
 *   - M2: water access in focused paddock
 *   - M3: cash, current cattle price, player role
 *   - M4a: muster transit status + destination + days remaining
 *   - M5: drought severity bar, lactating dam count, pregnant head count
 *   - M6: logged species count
 *   - M7: bankruptcy days warning
 *   - M8: sensor count (informational)
 */
UCLASS(Abstract)
class TARAGAME_API UTaraHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Which paddock to show focused-paddock stats for (grass, water access).
	// Update from BP when player crosses paddocks. Default "Holding" matches
	// the new Phase 2 paddock seed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|HUD")
	FString FocusedPaddockId = TEXT("Holding");

	// Refresh rate in Hz — 4 = every 0.25s. Most sim state changes daily, but
	// real-time clock advance is sub-second. 4Hz keeps the HUD responsive
	// without per-frame work.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|HUD")
	float RefreshHz = 4.0f;

	// ---- Bindings (all BindWidgetOptional — skip whatever you don't want) ----

	// M1 — basics
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DateTime;          // "Y1 D047 14:30"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Season;            // "Wet" / "Dry"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Herd;              // "Herd: 42 head · cond 78"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Paddock;           // "Holding · 2400 kg/ha · water Full"

	// M3 — economy
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Cash;              // "$3,250"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CattlePrice;       // "Beasts @ $850"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Role;              // "Ringer" / "Manager" / "Owner"

	// M4a — muster status
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MusterStatus;      // "Idle" or "Mustering → Pat's E (2d left)"

	// M5 — drought + breeding
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Drought;          // 0..1 progress bar

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DroughtSeverity;   // "Drought 0.4"

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BreedingState;     // "Pregnant: 18 head" / "Breeding window open"

	// M6 — researcher
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BirdLog;           // "Birds logged: 7/13"

	// M7 — bankruptcy warning (only shown if days > 0)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_BankruptcyWarning;

	// M8 — sensor count
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SensorCount;

	// Bad-weather decision pending alert (drought decision modal pending)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DecisionAlert;     // "⚠ Drought decision pending" or hidden

private:
	float SecondsSinceRefresh = 0.0f;
	void Refresh();

	// Helpers — set text only if the binding exists, else no-op.
	void SetTextSafe(UTextBlock* Target, const FString& Value);
	void SetVisibleSafe(class UWidget* Target, bool bVisible);
};
