# Phase 1 — Editor-side work to-do

> The code scaffold for Phase 1 is in place. This is the **non-code** work you do **inside the Unreal Editor** to bring the vertical slice to life. None of these steps can be done from outside the Editor.
>
> Phase 1 acceptance from `docs/3d-pivot/3d-milestone-plan.md`: "if you stand the quad facing the sun at hour 17:30, the gold grass blows out near-white against a colourless deep-blue sky. **That's the screenshot.**"

---

## ✦ Refresh 2026-05-25 — what's changed since this doc was written

The sim layer this doc was originally written against (M1 only) is now **complete 8/8** + has pacing dials + the bridge surface is full. Specifically:

| What changed | What it means for Phase 1 |
|---|---|
| **Sim port 8/8 complete** (M1+M2+M3+M4a+M5+M6+M7+M8 all live; pacing dials shipped) | The vertical slice gets WAY more sim depth for free. The bore actor auto-fails/repairs (M2), the herd makes calves (M5), birds can be logged (M6) — even though Phase 1 doesn't visualise most of this. Save autosaves on `OnDayEnded` already. See [BRIDGE_API.md](BRIDGE_API.md) for the full bridge surface. |
| **Engine version locked: UE5.4+** (newest stable — UE5.7 is fine) | Update `Tara_Station_3D.uproject`'s `EngineAssociation` to your installed version. World Partition is the default for new levels in 5.4+. |
| **Project moved to `Bureaublad/code/Tara_Station_3D/`** | All paths in this doc that reference the old `Bureaublad/Tara_Station_3D/` location now have `code/` between them. |
| **Pacing system: `UTaraSimSubsystem::AdvanceRealTime(DeltaSeconds)`** | **You don't need a `T` key binding to tick days anymore.** Wire `BP_TaraGameMode::Tick → Subsystem.AdvanceRealTime(Delta)` and the sim auto-advances at 30 in-game min / real-second by default (CORE_LOOP §7). Settings UI can mutate via `Subsystem.SetSecondsPerInGameDay(seconds)`. |
| **TS-3D-TWO-SEASON-MODE C++ shipped** | When you finish the basic Phase 1 vertical slice and start on Phase 2, jump straight to [PHASE2_SHED_EDITOR_TODO.md](PHASE2_SHED_EDITOR_TODO.md) for the shed-action-menu hookup. The C++ widget base + DT_ShedActions CSV + bankruptcy override are already in place. |
| **CORE_LOOP §7 daily-loop anchor** | Phase 1 vertical slice ideally now anchors at cottage→shed→quad→paddock→bore→return per MAP_BRIEF §7, not just "drive the quad around a paddock". You can either: (a) ship a stripped-down version (just the paddock + bore + quad) for the screenshot gate, then expand in Phase 2 via TS-3D-PHASE2-shed-cottage; (b) jump straight to a minimal cottage+shed setup. **Recommend (a)** — the screenshot gate is about the grass+dusk vision, not the daily loop. |
| **Sim smoke test available** | Run `Tara.RunSmokeTest 365` from the in-game console once the project compiles. Logs PASS/FAIL — verifies the sim is alive and invariants hold over a simulated year. Use it as a first sanity check before doing any Editor work. |

**Verify the sim is alive before doing any Editor work:** open the Editor, hit Play (PIE), and look at the Output Log. You should see `[TaraSim] Initialized. Year 1 Day 1 06:00 — herd 8 head`. If you see that line, the bridge is loading the FStation correctly and you can move to step 2 below.

**The original steps below are still valid for the screenshot-gate work.** Read on.

---

---

## Order to do this in

The order matters — some steps depend on the previous. Roughly 1–3 sessions of Editor work.

### 1. First-time setup (do once)

- [ ] Right-click `Tara_Station_3D.uproject` → **Generate Visual Studio project files**. This creates the `.sln` you'll use to build C++.
- [ ] Open the `.sln` in Visual Studio / Rider; build the **Development Editor** configuration. First build is slow (compiles Engine headers). Subsequent builds are fast.
- [ ] When the build succeeds, double-click `Tara_Station_3D.uproject` → opens in the Editor.

### 2. Create the level (`L_DevSandbox`)

- [ ] **File → New Level → Open World**. Save as `Content/Maps/L_DevSandbox`.
- [ ] In `Project Settings → Maps & Modes → Editor Startup Map`: set to `L_DevSandbox`. (Already set in `Config/DefaultEngine.ini` — verify it stuck.)

### 3. Landscape (terrain for the paddock)

- [ ] **Modes panel → Landscape → New Landscape**. Size: 4 sections × 1 component per section is plenty for one paddock; ~500m × 500m of playable area.
- [ ] Paint a rough flat terrain. We're not sculpting in Phase 1 — just need a walkable surface for the quad.
- [ ] (Optional, recommended) **Sculpt tool** → drop the elevation slightly in a 100m × 100m area to define the paddock's boundary. Use **Ramp** tool to soften edges so the quad can climb in/out.

### 4. Grass material (the signature)

The brief's #1 art priority: **pale gold to straw, glows when backlit, never green**.

- [ ] Right-click in `Content/Materials/` → **Material**. Name: `M_Grass_Master`.
- [ ] Open the material editor. Set:
  - **Base Color**: a sample-able golden-tan colour (~`#C8A75A`). Slightly desaturated.
  - **Roughness**: 0.85–0.95 (matte).
  - **Specular**: 0.1 (low — pure roughness response).
  - **Normal**: leave flat for Phase 1; gradient texture if you want subtle blade direction.
  - **Two-sided lighting**: enabled (so backlit grass glows).
  - **Subsurface scattering**: optional but adds the brief's "blows out near-white" — tint pale yellow with low intensity.
- [ ] Expose a **scalar parameter** named exactly `GrassFreshness` (case-sensitive — `PaddockActor.cpp` looks for this name). Default value 1.0. Use it to lerp between full pale-gold (1.0) and bare brown (0.0) on Base Color.
- [ ] **Save**. Apply this material to the Landscape (Landscape mode → Painting → drop `M_Grass_Master` onto the default layer).

### 5. Place a PaddockActor

- [ ] In the Content Browser, navigate to `C++ Classes/TaraGame/Actors/PaddockActor`.
- [ ] **Drag** `APaddockActor` from there into the Level. Place it at the centre of your landscape area.
- [ ] **In the Details panel**:
  - **PaddockId**: `A`
  - **Ground Mesh**: assign Engine's `Engine/BasicShapes/Plane` (or your own ground plane mesh).
  - Scale the Plane large enough to cover the paddock area (~5000 × 5000 = 50m × 50m at 1 unit = 1cm).
  - **Grass Material**: assign `M_Grass_Master`.

> If you prefer to use the Landscape as the visible grass surface instead of a flat Plane: that's also valid; the Phase 1 acceptance just needs **one bounded paddock of correctly-coloured gold grass**.

### 6. Place a BoreActor

- [ ] Drag `ABoreActor` from `C++ Classes/TaraGame/Actors/BoreActor` into the level. Place it near the centre of the paddock.
- [ ] In Details:
  - **BoreId**: `bore-1`
  - **WindmillMesh / TankMesh / TroughMesh**: assign Engine `BasicShapes` (Cylinder × 2 + Cube) so the silhouette is visible. The brief mentions the multi-blade Southern Cross windmill silhouette specifically; even as placeholder cylinders, getting the proportions roughly right pays off (tall thin windmill + wider tank + low long trough).

### 7. Place CattleActors (~8 head)

- [ ] Drag 8 × `ACattleActor` into the paddock. Scatter them.
- [ ] For each, assign Engine `BasicShapes/Cube` as the BodyMesh (or a stretched Sphere). Real cattle meshes come later.

### 8. QuadBikePawn — input setup

The pawn is C++ but needs Input Actions configured in the Editor.

- [ ] Create `Content/Input/IA_Throttle` (InputAction, value type Float, axis 1D).
- [ ] Create `Content/Input/IA_Steer` (InputAction, value type Float, axis 1D).
- [ ] Create `Content/Input/IA_TickDay` (InputAction, value type bool/digital).
- [ ] Create `Content/Input/IMC_QuadBike` (Input Mapping Context). Map:
  - `IA_Throttle`: `W` (modifier: scale +1), `S` (modifier: scale -1).
  - `IA_Steer`: `D` (+1), `A` (-1).
  - `IA_TickDay`: `T` (digital press).
- [ ] Create a Blueprint subclass of `AQuadBikePawn` → `BP_QuadBike` in `Content/Pawns/`.
- [ ] In `BP_QuadBike` Defaults panel:
  - **BodyMesh**: assign Engine `BasicShapes/Cube` for now.
  - **MappingContext**: `IMC_QuadBike`.
  - **ThrottleAction / SteerAction / TickDayAction**: assign the three IA assets.
- [ ] Drop `BP_QuadBike` into the level. Set it as the **Default Pawn** in your GameMode (next step).

### 9. GameMode

- [ ] Right-click in Content Browser → **Blueprint Class → GameModeBase** → `BP_TaraGameMode`.
- [ ] In the new GameMode's Defaults:
  - **Default Pawn Class**: `BP_QuadBike`.
- [ ] In `Project Settings → Maps & Modes → Default GameMode`: set to `BP_TaraGameMode`.

### 10. Sun + sky for the brief's dusk gradient

- [ ] **Place Actors panel → Lights → Directional Light** for the sun. Set its rotation to ~hour-of-day appropriate (rotation around Y axis: 30° = morning, 90° = midday, 150° = evening).
- [ ] **Place Actors panel → Sky → Sky Atmosphere** (for atmospheric scattering — the brief's deep blue daytime + dusk gradient).
- [ ] **Place Actors panel → Sky → Volumetric Cloud** — set very low density / disable for the brief's *cloudless* dusk. (Or leave for daytime cirrus; remove around hour 18+.)
- [ ] **Place Actors panel → Lights → Sky Light** (set Mobility = Movable; Source Type = Captured Scene).
- [ ] **Place Actors panel → Visual Effects → Exponential Height Fog** — light tint matching the warm horizon haze the brief calls for.
- [ ] **Optional, Phase 3 polish:** wire the Directional Light's rotation to `UTaraSimSubsystem::GetClock()` so the sun moves with in-game time. Phase 1 can have a static sun rotation.

### 11. HUD

- [ ] Right-click in Content Browser → **Widget Blueprint** → `WBP_TaraHUD`. Parent class: `UTaraHUDWidget`.
- [ ] In the Designer, drop 3 `TextBlock` widgets and rename them to `Text_DateTime`, `Text_Herd`, `Text_Paddock` (these names match the `BindWidget` properties in `TaraHUDWidget.h` — case-sensitive).
- [ ] Layout them in a corner (top-left typical). Style: monospace font, off-white colour `#E8D9B8`.
- [ ] In `BP_TaraGameMode` (or via the player controller / quad pawn's BeginPlay): **Create Widget → WBP_TaraHUD → Add to Viewport**. Easiest in a Blueprint subclass of `APlayerController` — `BP_TaraPlayerController` with a BeginPlay event that creates the HUD.

### 12. The screenshot

- [ ] In the Editor, set the Directional Light yaw to ~190°, pitch to ~-8° (low west-of-overhead sun).
- [ ] Push the quad to the centre of the paddock, facing west toward the sun.
- [ ] Press **Play in Editor** (PIE). Press `T` a few times to advance days and watch the grass material respond (`GrassFreshness` scalar driven by the sim's grass kg/ha).
- [ ] Frame the camera so the windmill silhouettes against the dusk gradient. **High-resolution screenshot** (Edit → Take High-Res Screenshot → 4× or 8×).

That's the Phase 1 gate.

---

## Common pitfalls

| Symptom | Fix |
|---|---|
| Grass material looks lush green | Defaults from Engine starter pack are aggressively green — make sure you authored your own Base Color, not used a sample material. |
| `GrassFreshness` scalar doesn't drive the material | Name mismatch. Must be exactly `GrassFreshness` (case-sensitive). Verify in PaddockActor.cpp `SetScalarParameterValue` call. |
| `T` key does nothing | Input Mapping Context not pushed to the local player. Verify `BP_QuadBike` Defaults has `MappingContext` set and the BeginPlay code path runs (UE_LOG to confirm). |
| Sim doesn't seem to advance | Open Output Log; UTaraSimSubsystem::Initialize logs "[TaraSim] Initialized…" — should appear once per PIE start. If not, the GameInstance subsystem didn't load — check `Config/DefaultEngine.ini` `GameInstanceClass` setting. |
| Save file conflicts | Saved file lives at `Tara_Station_3D/Saved/SaveGames/tara-save-3d-v1-phase0.json`. Delete to force New Game. |

---

## What landed in Phase 1 vs what's still TODO for the screenshot

| In code | In Editor (this doc) |
|---|---|
| ✅ TaraSimCore module (engine-agnostic sim) | — |
| ✅ EventBus + SeasonClock + Station + Paddock + CattleCohort + Herd + ConditionSystem | — |
| ✅ UTaraSimSubsystem (bridge) | — |
| ✅ PaddockActor / BoreActor / CattleActor / QuadBikePawn / TaraHUDWidget classes | — |
| ✅ Save/load via JSON to Saved/ folder | — |
| ✅ CI grep check enforcing engine-agnostic sim | — |
| — | ⏳ Level + Landscape |
| — | ⏳ M_Grass_Master material with `GrassFreshness` scalar |
| — | ⏳ Actors placed in level |
| — | ⏳ Input Actions + Input Mapping Context |
| — | ⏳ BP_QuadBike, BP_TaraGameMode |
| — | ⏳ Sun + Sky + Atmospheric Fog |
| — | ⏳ WBP_TaraHUD widget Blueprint |
| — | ⏳ Sun-tracks-clock wiring (Phase 1 optional; Phase 3 polish) |
