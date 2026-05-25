# Phase 2 — Watershed (river + creek + dams) Editor TODO

> **For:** executing `TS-3D-PHASE2-watercourse` after the C++ driver actors ship.
> **Plan:** [backlog_bandit/plans/TS-3D-PHASE2-watercourse.md](../../backlog_bandit/plans/TS-3D-PHASE2-watercourse.md) (revised 2026-05-25 to use UE5 Water plugin)
> **Pre-requisite reads:** [MAP_BRIEF.md §3-4](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md), [watershed.md](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/watershed.md), [BRIDGE_API.md](BRIDGE_API.md)
> **Time:** ~3-4 Editor sessions (~30-45 min each).
> **What's already in place:** the `OnSeasonChanged` synth-delegate already exists in the bridge (TS-3D-TWO-SEASON-MODE shipped it). `Subsystem->GetCurrentDroughtSeverity()` BlueprintPure already exists. We just need the engine-side water bodies + 2 thin driver actors.

---

## Order to do this in

Plugin enable → Build.cs update → 2 C++ driver actors → engine-side water bodies → materials → driver actor pointers → PIE verify.

### 0. (5 min) — Enable the Water plugin

1. Editor: **Edit → Plugins** → search "Water" → tick **"Water"** plugin
2. Editor will prompt: "Restart now?" → **Yes**
3. After restart, verify `WaterBodyRiver`, `WaterBodyLake`, `WaterBrushManager` show up when right-clicking in a level or in the Place Actors panel.

---

### 1. (10 min) — Add Water module to TaraGame.Build.cs

Outside the Editor — `Source/TaraGame/TaraGame.Build.cs`:

```cs
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "UMG",
    "Slate",
    "SlateCore",
    "Water",         // ← ADD THIS
    "TaraSimCore",
});
```

Regenerate VS project files: right-click `.uproject` → "Generate Visual Studio project files". Rebuild Tara_Station_3DEditor in VS2022.

---

### 2. (10 min) — Author `ARiverDroughtDriver` + `ACreekSeasonalDriver` C++

Outside the Editor — these are the two thin driver actors per the plan §5. Drop them in `Source/TaraGame/Public/Actors/` + `Private/Actors/`.

**`ARiverDroughtDriver.h`:**

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterBodyRiverActor.h"   // from Water plugin
#include "RiverDroughtDriver.generated.h"

UCLASS(Blueprintable)
class TARAGAME_API ARiverDroughtDriver : public AActor
{
    GENERATED_BODY()

public:
    ARiverDroughtDriver();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Water")
    TObjectPtr<AWaterBodyRiver> TargetRiver;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Water")
    FName WaterLevelParameter = TEXT("WaterLevel");

protected:
    virtual void Tick(float DeltaSeconds) override;

private:
    float Accumulator = 0.0f;
    static constexpr float TickInterval = 1.0f;  // 1 second; cheap
};
```

**`ARiverDroughtDriver.cpp`:**

```cpp
#include "Actors/RiverDroughtDriver.h"
#include "Bridge/TaraSimSubsystem.h"
#include "WaterBodyComponent.h"      // from Water plugin
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ARiverDroughtDriver::ARiverDroughtDriver()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARiverDroughtDriver::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Accumulator += DeltaSeconds;
    if (Accumulator < TickInterval) return;
    Accumulator = 0.0f;

    if (!TargetRiver) return;

    UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
    UTaraSimSubsystem* Sub = GI ? GI->GetSubsystem<UTaraSimSubsystem>() : nullptr;
    if (!Sub) return;

    const float WaterLevel = 1.0f - Sub->GetCurrentDroughtSeverity();
    if (UWaterBodyComponent* WC = TargetRiver->GetWaterBodyComponent())
    {
        if (UMaterialInstanceDynamic* MID = WC->CreateOrGetWaterMID())
        {
            MID->SetScalarParameterValue(WaterLevelParameter, WaterLevel);
        }
    }
}
```

**`ACreekSeasonalDriver.h`:**

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterBodyRiverActor.h"
#include "SeasonClock.h"   // for ESeason
#include "CreekSeasonalDriver.generated.h"

UCLASS(Blueprintable)
class TARAGAME_API ACreekSeasonalDriver : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Water")
    TObjectPtr<AWaterBodyRiver> TargetCreek;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    FDelegateHandle SeasonHandle;
    void ApplySeason(ESeason Season);
};
```

**`ACreekSeasonalDriver.cpp`:**

```cpp
#include "Actors/CreekSeasonalDriver.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Station.h"
#include "WaterBodyComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void ACreekSeasonalDriver::BeginPlay()
{
    Super::BeginPlay();
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
    {
        if (UTaraSimSubsystem* Sub = GI->GetSubsystem<UTaraSimSubsystem>())
        {
            ApplySeason(Sub->GetStation() ? Sub->GetStation()->GetClock().GetSeason() : ESeason::Wet);
            SeasonHandle = Sub->OnSeasonChanged.AddLambda(
                [this](ESeason, ESeason To) { ApplySeason(To); });
        }
    }
}

void ACreekSeasonalDriver::EndPlay(const EEndPlayReason::Type Reason)
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
    {
        if (UTaraSimSubsystem* Sub = GI->GetSubsystem<UTaraSimSubsystem>())
        {
            Sub->OnSeasonChanged.Remove(SeasonHandle);
        }
    }
    Super::EndPlay(Reason);
}

void ACreekSeasonalDriver::ApplySeason(ESeason Season)
{
    if (!TargetCreek) return;
    if (UWaterBodyComponent* WC = TargetCreek->GetWaterBodyComponent())
    {
        // 5.7 API: SetWaterMeshActive or component visibility — confirm at compile.
        WC->SetVisibility(Season == ESeason::Wet, true /* propagate */);
    }
}
```

> **Note:** the exact Water-plugin API for enable/disable may differ slightly in 5.7 — if `SetWaterMeshActive` exists, use it; otherwise fall back to `SetVisibility` on the WaterBodyComponent. Confirm at compile time.

Build in VS2022 (Ctrl+Shift+B).

---

### 3. (15 min) — Place `WaterBrushManager` (one per level)

Required singleton actor for the Water plugin to function.

1. Open `L_HubArea` (or whatever your main level is).
2. Place Actors panel → search **"WaterBrushManager"** → drag one into the level.
3. Position doesn't matter — it's a logic actor, no visible mesh.
4. Save the level.

---

### 4. (30-45 min) — Place the central river `WaterBodyRiver`

1. Place Actors → **WaterBodyRiver** → drag into the level.
2. The actor pops a spline with 2 points. Select it.
3. With the actor selected, you can:
   - Press **Alt+click** at the end of the spline to add new spline points
   - Right-click a spline point → various options for tangent/width control
4. Build the river path per MAP_BRIEF §3:
   - **Start point (point 0):** NW edge of the Landscape (the future gorge end — Phase 4)
   - **Mid points (1-15ish):** flow S/SE through Sandridge area → Sandridge-S → Limestone-SW
   - **End point:** southern dam cluster (around Tara Dam location)
5. Adjust per-point **width** (set per-point in Details when a point is selected): start narrow (~10m) at NW source, widen to ~30m in the river-spine paddocks, narrow back to ~20m near the southern dams. Width is set in cm so type values like **2000** for 20m.
6. The plugin will auto-carve the Landscape into a riverbed channel — verify in the viewport you see the channel below ground level.
7. **Save the level** + take a top-down screenshot for the done-log.

---

### 5. (20 min) — Place the creek branch

1. Place Actors → **WaterBodyRiver** → drag into level.
2. Position: branches off the main river near Sandridge-S, loops through Telephone-N paddock, rejoins the river or terminates.
3. Width: ~3-8m (smaller than main river).
4. Spline points: ~6-8 should be enough.
5. **Save.**

---

### 6. (20 min) — Place three `WaterBodyLake` actors for the southern dams

For each of `Tara Dam`, `Dam II`, `Dam 2`:

1. Place Actors → **WaterBodyLake** → drag at the southern terminus location.
2. Each lake actor has a closed-polygon spline. Edit points to roughly match each dam's approximate shape from MAP_BRIEF (typically 50-150m diameter for a dam).
3. Position so the river's final spline point flows INTO the first lake (Tara Dam), then Dam II adjacent, then Dam 2.
4. Naming: in the World Outliner, rename the actors to `BP_Lake_TaraDam`, `BP_Lake_DamII`, `BP_Lake_Dam2` for clarity.

---

### 7. (15 min) — Create `MI_River_Water` material instance + assign

1. Content Browser → navigate to `Engine Content → Water → Materials` (toggle "Show Engine Content" in the Content Browser settings if you don't see it).
2. Find `M_Water_River` (or whatever Epic ships as the master water material in 5.7 — name may have shifted to `M_WaterRiver` or similar).
3. Right-click → **Create Material Instance** → place in `Content/Materials/MI_River_Water.uasset`.
4. Open `MI_River_Water`. Expose a `WaterLevel` scalar parameter (default 1.0).
5. **Assign `MI_River_Water` to all three water bodies' Water Material slot**:
   - Select each WaterBodyRiver / WaterBodyLake in turn → Details → search "Water Material" → assign `MI_River_Water`.

---

### 8. (10 min) — Place + configure the C++ driver actors

1. Place Actors → search **"RiverDroughtDriver"** → drag into level.
2. Select it → Details → set **Target River** to the central river actor.
3. (Optional) Adjust **Water Level Parameter** if you named the material parameter differently than `WaterLevel`.
4. Place Actors → search **"CreekSeasonalDriver"** → drag in.
5. Select it → set **Target Creek** to the creek branch actor.

---

### 9. (15 min) — River red gum placement

Placeholder for Phase 3 visual pass — use any Engine starter content tree mesh.

1. **Modes → Foliage**.
2. Add a Foliage Type from a placeholder tree mesh (e.g. `Engine Content/Starter Content/Architecture/Tree_01` or similar).
3. Set **Density / Radius** so gums spawn sparsely (one every 5-10m along the river path).
4. Paint along both banks of the central river + the creek + around the lakes. Aim for ~30-50 trees total.
5. Skip the Limestone NE / monolith E paddocks — gums only along watercourses.

Phase 3 visual pass swaps the placeholder for proper river-red-gum mesh.

---

### 10. (20 min) — PIE verification

1. Run `Tara.RunSmokeTest 365` in console — confirms sim hasn't broken.
2. **Drought test:** Console → `Subsystem->GetStationMutable()->GetPrices().DroughtSeverity = 0.8` (or use a debug Blueprint that exposes this). Within ~1 second, watch the river's water surface visibly drop / fade as `WaterLevel` parameter updates. Screenshot.
3. **Season-flip test:** Console → `Subsystem->TickDay()` N times to push DayOfYear past 334 (Dry→Wet transition). Creek's water becomes visible. Screenshot.
4. **Save round-trip:** save game (autosaves on DayEnded) → restart PIE → river/creek still configured correctly from new state.

---

### 11. (10 min) — Done-log entry

1. Screenshots: river path from above + dry vs wet creek + drought vs full river.
2. Update `done_log/<current-week>.md` with the screenshots + the actual Water-plugin API used (`SetVisibility` vs `SetWaterMeshActive` — record which worked for future reference).
3. Move `plans/TS-3D-PHASE2-watercourse.md` → `plans/done/TS-3D-PHASE2-watercourse.md`.
4. Remove the BACKLOG row.
5. Commit + push backlog_bandit + Tara_Station_3D.

---

## Troubleshooting

- **Water bodies render nothing / look black** → missing `WaterBrushManager` in the level. Add one (step 3). Or the Water plugin needs another restart cycle.
- **Landscape doesn't carve to a riverbed channel** → check WaterBrushManager exists + that the WaterBodyRiver's `Affects Landscape` flag is enabled in Details.
- **`AWaterBodyRiver` not found at compile time** → check `"Water"` is in `TaraGame.Build.cs` PublicDependencyModuleNames + that you regenerated VS project files after the edit.
- **`SetWaterMeshActive` doesn't exist on UWaterBodyComponent** → the 5.7 API may use a different name. Try `SetVisibility(bool, bool)` instead (already in the cpp above as the fallback).
- **Drought parameter doesn't visually change anything** → check the `MI_River_Water` instance's MasterMaterial DOES read `WaterLevel` — open the master material `M_Water_River`, search for "WaterLevel" usage, if not present add a scalar param wired to base colour alpha or material masking.
- **Creek doesn't disable on Dry season** → verify `OnSeasonChanged` is firing (Output Log will show `[TaraSim] SeasonChanged: ... -> ...` when the season boundary crosses). If not firing, the smoke test passes but real PIE doesn't tick days fast enough — set `Subsystem->SetSecondsPerInGameDay(60.0f)` for a quick test cycle (1 min real = 1 day in-game).

---

When all 9 acceptance criteria from the plan pass + screenshots are in the done-log, `complete TS-3D-PHASE2-watercourse` to close it out.
