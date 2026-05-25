# Phase 2 — Curated 13-paddock layout Editor TODO

> **For:** executing `TS-3D-PHASE2-paddocks` after the C++ side ships.
> **Plan:** [backlog_bandit/plans/TS-3D-PHASE2-paddocks.md](../../backlog_bandit/plans/TS-3D-PHASE2-paddocks.md)
> **Pre-requisite reads:** [MAP_BRIEF.md §3 + §5](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md), [BRIDGE_API.md](BRIDGE_API.md)
> **Time:** ~3-5 Editor sessions (~30-60 min each).
> **What's already in place** (so you don't have to write any new C++): the existing `APaddockActor` Blueprint subclass, `Station::SeedDefaults` (currently has placeholder A/B/C paddocks; you'll edit this in C++ to use real names — small change), the `Paddock` entity in TaraSimCore already supports arbitrary string IDs.

---

## Order to do this in

The C++ change (real-name paddocks in SeedDefaults) lands first so the sim has the right IDs to bind Actors against. Then Landscape paint, then place actors, then bore mapping, then verification.

### 0. (5 min) — C++ side: replace A/B/C with real names in SeedDefaults

Outside the Editor. Open `Source/TaraSimCore/Private/Station.cpp`, find `FStation::SeedDefaults()`, replace the 3 paddock blocks (A/B/C) with the 13 real names per [MAP_BRIEF §5](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md). Replace the Adjacency map. Replace bore `ServesPaddockIds` with the right real-name targets. Bump the save schema constant in `Public/Station.h` (`v9-phase5-prereqs` → `v10-paddocks`).

**Suggested seed values** (Phase 1 vertical-slice paddock = Holding; numbers are first-pass estimates, tune in playtest):

```cpp
// Home cluster (south-centre)
{ Id: "Holding",       Name: "Holding",           AreaHa: 80,  StartingGrassKgPerHa: 2600 }
{ Id: "Tara-House",    Name: "Tara House",        AreaHa: 60,  StartingGrassKgPerHa: 2500 }
{ Id: "Bull",          Name: "Bull",              AreaHa: 150, StartingGrassKgPerHa: 2700 }
{ Id: "Mid-II",        Name: "Mid II",            AreaHa: 200, StartingGrassKgPerHa: 2400 }
// River spine
{ Id: "Telephone-N",   Name: "Telephone North",   AreaHa: 220, StartingGrassKgPerHa: 2900 } // best grass — river-side
{ Id: "Sandridge-S",   Name: "Sandridge South",   AreaHa: 280, StartingGrassKgPerHa: 2700 }
{ Id: "Pats-W",        Name: "Pat's West",        AreaHa: 240, StartingGrassKgPerHa: 2500 }
// Toward gorge (NW)
{ Id: "Hill-Pdk-N",    Name: "Hill Paddock North",AreaHa: 180, StartingGrassKgPerHa: 2200 }
{ Id: "Wallace-Rd-S",  Name: "Wallace Rd South",  AreaHa: 200, StartingGrassKgPerHa: 2300 }
// Toward spires (NE) — Limestone country, drier
{ Id: "Limestone-SW",  Name: "Limestone S-W",     AreaHa: 250, StartingGrassKgPerHa: 1900 }
{ Id: "Limestone-NE",  Name: "Limestone N-E",     AreaHa: 280, StartingGrassKgPerHa: 1800 }
// Toward monolith (E)
{ Id: "Pats-E",        Name: "Pat's East",        AreaHa: 300, StartingGrassKgPerHa: 2000 }
{ Id: "Wallace-Bore-II", Name: "Wallace Bore II", AreaHa: 320, StartingGrassKgPerHa: 1900 }
```

**Adjacency** — derive from the topology in [MAP_BRIEF §3](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md). Suggested set (24 edges total, undirected):

```
Holding ↔ Tara-House, Holding ↔ Bull, Holding ↔ Mid-II, Holding ↔ Telephone-N
Tara-House ↔ Bull, Tara-House ↔ Mid-II
Bull ↔ Mid-II
Mid-II ↔ Sandridge-S, Mid-II ↔ Pats-W
Telephone-N ↔ Sandridge-S, Telephone-N ↔ Hill-Pdk-N, Telephone-N ↔ Wallace-Rd-S
Sandridge-S ↔ Pats-W, Sandridge-S ↔ Limestone-SW
Pats-W ↔ Limestone-SW, Pats-W ↔ Pats-E
Hill-Pdk-N ↔ Wallace-Rd-S
Limestone-SW ↔ Limestone-NE
Limestone-NE ↔ Pats-E
Pats-E ↔ Wallace-Bore-II
Wallace-Rd-S ↔ Hill-Pdk-N
```

**Starting herd** in `Holding` paddock — change `Herd = FHerd(FCattleCohort(CohortCfg), TEXT("A"))` to `TEXT("Holding")`.

**Bore mapping** (suggested mapping using the real bore names from MAP_BRIEF §2):

```cpp
"Holding-Paddock-Nest" → serves {Holding, Tara-House}
"Wallace-Road-Nest"    → serves {Telephone-N, Wallace-Rd-S}
"Sandridge-Bore"       → serves {Sandridge-S, Mid-II}
"Pearcers-Bore"        → serves {Bull, Pats-W}
"Limestone-Bore"       → serves {Limestone-SW, Limestone-NE}
"Pats-Bore"            → serves {Pats-E, Wallace-Bore-II}
"Hill-Nest"            → serves {Hill-Pdk-N}
```

Build the C++ change in VS2022 (Ctrl+Shift+B) before continuing to Editor steps.

---

### 1. (15 min) — Verify the new sim seed in PIE

1. Open the Editor → hit Play (PIE).
2. Open the Output Log. Look for `[TaraSim] Initialized. Year 1 Day 1 06:00 — herd 8 head`. The init line itself doesn't show the paddock count, but you can verify via console:
3. Press the backtick `` ` `` to open the console → type `Tara.RunSmokeTest 30` → Enter.
4. Output should show `13 paddocks` in the smoke test report (the smoke test logs PaddockCount). If you see 13, the C++ seed is good. If 3, the build didn't pick up your edit — rebuild.

---

### 2. (30 min) — Extend the Landscape to cover the 13-paddock area

If you already have a `L_DevSandbox` (from PHASE1_EDITOR_TODO Phase 1), either extend its Landscape or start a new level `L_HubArea` from File → New Level → Open World.

1. **Landscape Mode → New Landscape**.
2. **Sizing** — Components: 8×8 sections, Resolution: 511×511. Approximate game-world size: ~3 km × ~3 km. Compresses the real ~15 km Tara lease into a satisfying few-minutes-by-quad scale per CORE_LOOP §4.
3. Place the Landscape so the south-centre is near origin (where Tara homestead will land).
4. **Sculpt** lightly — drop elevation slightly in the central spine (the future river channel), raise the NW corner (future gorge), keep the rest mostly flat.

---

### 3. (45-60 min per visit) — Paint paddock regions with Landscape Layers

There are 13 paddocks but UE5 performance prefers ≤8 active Landscape layers. **Option A (recommended for first pass):** group paddocks into 5 ground-material layers, distinguishing individual paddocks by trigger volumes only. **Option B (full granular):** one layer per paddock — works but heavier.

**Option A — 5 grouped layers:**

| Layer | Paddocks | Ground colour bias |
|---|---|---|
| `LL_HomeFlats` | Holding, Tara-House, Bull, Mid-II | Tan-gold |
| `LL_RiverFlats` | Telephone-N, Sandridge-S, Pats-W | Slightly greener tan |
| `LL_GorgeBound` | Hill-Pdk-N, Wallace-Rd-S | Rockier red-brown |
| `LL_Limestone` | Limestone-SW, Limestone-NE | Pale yellow-grey (real geology) |
| `LL_MonolithBound` | Pats-E, Wallace-Bore-II | Standard dry tan |

For each layer:
1. In **Landscape Mode → Paint tab**, click `+` next to "Layers" to add a layer.
2. Right-click the layer → **Create Layer Info** → Weight-Blended.
3. Paint the layer over the relevant paddocks following MAP_BRIEF §3 topology — home cluster south-centre, etc.

---

### 4. (45 min) — Place 13 `BP_Paddock` actor instances

You need 13 instances of the paddock actor (existing `BP_Paddock` from Phase 1, or `APaddockActor` C++ class directly). Each represents one sim-side `FPaddock` and pushes its `GrassFreshness` material parameter from sim state.

1. In the Content Browser, find `BP_Paddock`. If it doesn't exist as a Blueprint subclass, just place 13 instances of the C++ `APaddockActor` class.
2. Drop one instance at the centroid of each painted paddock area.
3. For each instance, in the **Details** panel set:
   - **Paddock Id** (must match exactly: `Holding`, `Tara-House`, `Bull`, `Mid-II`, `Telephone-N`, `Sandridge-S`, `Pats-W`, `Hill-Pdk-N`, `Wallace-Rd-S`, `Limestone-SW`, `Limestone-NE`, `Pats-E`, `Wallace-Bore-II`)
4. Optionally add a Box trigger volume covering the painted region — for paddock-name HUD.

**Tip:** drop one, set its `PaddockId`, then **Alt+drag** to duplicate; you only need to change the `PaddockId` on each duplicate.

---

### 5. (30 min) — Place 7 real-named `BP_Bore` actors

Use the existing `BP_Bore` (or `ABoreActor` C++) from Phase 1.

Suggested positions per MAP_BRIEF §2 watering points + the bore mapping in step 0:

| Bore Id | Paddocks served | Physical placement hint |
|---|---|---|
| `Holding-Paddock-Nest` | Holding, Tara-House | Between Holding + Tara-House paddock boundary |
| `Wallace-Road-Nest` | Telephone-N, Wallace-Rd-S | NW of central spine |
| `Sandridge-Bore` | Sandridge-S, Mid-II | Central — near river |
| `Pearcers-Bore` | Bull, Pats-W | Between Bull + Pat's W |
| `Limestone-Bore` | Limestone-SW, Limestone-NE | NE — Limestone paddock cluster |
| `Pats-Bore` | Pats-E, Wallace-Bore-II | Far E |
| `Hill-Nest` | Hill-Pdk-N | NW edge |

For each, set **Bore Id** in Details. The C++ sim ticks each one (M2 WaterSystem) — failure rolls, repair via `Subsystem.CheckBore(boreId)`, etc.

---

### 6. (15 min) — Author `WBP_PaddockNameHUD` widget

Tiny widget — shows "Holding" / "Tara House" / etc. when player enters a paddock's trigger volume.

1. **Right-click in Content/UI** → **Widget Blueprint** (parent: UUserWidget).
2. Name: `WBP_PaddockNameHUD`.
3. Add a single **TextBlock** centred at the bottom; **Bind** its `Text` to a function that calls `Subsystem.GetHerd().CurrentPaddockId` — or simpler, expose a `SetPaddockName(name)` BP function and call it from each `BP_Paddock`'s Box-trigger overlap.
4. Add to viewport on game-start (via the GameMode BP's BeginPlay).

---

### 7. (15 min) — PIE smoke verification

1. Run `Tara.RunSmokeTest 365` in console — should pass.
2. Spawn at homestead area (Holding paddock) — HUD shows "Holding".
3. Drive the quad through each cluster: Holding → Mid-II → Sandridge-S → Telephone-N → Hill-Pdk-N (NW) → back → Pat's W → Limestone-SW → Pat's E (E). HUD updates each crossing.
4. Confirm grass colour bias varies per Landscape layer.
5. Open the Output Log + watch for water events — `[TaraSim] BoreFailed: ...` will fire eventually as bores decay.

---

### 8. (10 min) — Done-log entry

Once all paddocks are placed + verified:

1. Take a screenshot of the level from above (top-down) showing the 13 paddock outlines + bore positions.
2. Update `backlog_bandit/done_log/<current-week>.md` (template in CLAUDE.md):
   - List of paddock IDs placed
   - Confirmation that smoke test passes with 13 paddocks
   - Any deviations from the suggested area/grass values (per real-Tara knowledge if you adjust)
3. Move `plans/TS-3D-PHASE2-paddocks.md` → `plans/done/TS-3D-PHASE2-paddocks.md` and update its frontmatter status.
4. Remove the BACKLOG row.
5. Commit + push backlog_bandit + Tara_Station_3D.

---

## Troubleshooting

- **HUD shows wrong paddock name** → check the `PaddockId` on each `BP_Paddock` instance matches the C++ seed exactly (case + hyphens matter).
- **Bores don't fail** → expected — failure roll is daily, condition starts ~85, drift slow. Run 90+ in-game days to see the first failure. Use console `Tara.RunSmokeTest 365` to verify the WaterSystem is firing.
- **Build error after editing SeedDefaults** → likely a syntax typo. Output panel in VS2022 shows the line. Common: missing `;` or wrong adjacency key.
- **Old save still loads with A/B/C** → save schema bump worked; check by deleting `Tara_Station_3D/Saved/SaveGames/*.json`. Fresh save will use real names.
