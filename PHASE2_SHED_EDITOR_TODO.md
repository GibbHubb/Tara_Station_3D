# Phase 2 — Shed action menu Editor TODO

> **For:** finishing `TS-3D-TWO-SEASON-MODE` after the C++ side landed at commit `19c385f`.
> **What's already done in C++:** `FShedActionRow` struct, `UShedActionMenu` UUserWidget base, `FOnTaraSeasonChanged` bridge delegate, bankruptcy override, season filter logic, **default rows fallback** so PIE works without `DT_ShedActions.uasset` existing yet.
> **What this doc covers:** click-by-click for the 6 Editor-side steps from [plan §7](../../backlog_bandit/plans/TS-3D-TWO-SEASON-MODE.md#7-steps-execution-order).
> **Time estimate:** ~45-60 min of Editor work the first time, ~15 min if you've done this kind of widget before.

---

## Step 4 (MUST do first — the widget) — Create `WBP_ShedActionMenu`

1. **Open the Editor**: double-click `Tara_Station_3D.uproject`. Wait for it to load.
2. **Build the C++** if you haven't already after the `19c385f` commit: top menu → **Tools → Refresh Visual Studio Code Project** (or Visual Studio Project), then in your IDE build `Tara_Station_3DEditor — Development Editor`. Re-open the Editor when the build finishes.
3. **Make sure the new C++ class is visible**: in the Content Browser, click the **Settings** gear (top-right of the browser) → tick **Show C++ Classes**. You should now see `C++ Classes / TaraGame / UI / ShedActionMenu` in the source tree.
4. **Create the Content/UI folder** if it doesn't exist: right-click in the Content root → **New Folder** → name it `UI`.
5. **Create the widget asset**:
   - Right-click in `Content/UI/` → **User Interface → Widget Blueprint**.
   - When the parent-class picker pops, type `ShedActionMenu` and pick `UShedActionMenu` (the C++ class).
   - Name the asset `WBP_ShedActionMenu` and open it.
6. **Lay out the widget** in the Designer:
   - Drag a **Canvas Panel** (or just use the root) → drop a **Vertical Box** in the centre. Resize to ~600×600.
   - Inside the Vertical Box, top-to-bottom:
     - **TextBlock** — set its **Is Variable** ✓ + rename to **`SeasonLabel`** (must match the `BindWidget` name exactly). Font size 24, centered.
     - **TextBlock** — set Is Variable ✓ + rename to **`BankruptcyMessage`**. Text: *"The property's in administration — see the boss."* Font size 18, set initial **Visibility = Collapsed**. (Optional widget — the C++ has `BindWidgetOptional` for it.)
     - **Vertical Box** — set Is Variable ✓ + rename to **`ActionList`**. This is where the C++ spawns buttons.
     - **Button** with a TextBlock child reading "Close" — set Is Variable ✓ on the button, then go to **Events** panel (right side) and click the `+` next to **On Clicked**. In the resulting BP graph, drag from the exec pin → search for **Close** (the UFUNCTION on UShedActionMenu) → connect.
   - **Compile + Save** the widget (top-left of the BP editor).

   > **Variable names matter.** The C++ has `UPROPERTY(meta = (BindWidget))` for `SeasonLabel`, `ActionList`, and `BindWidgetOptional` for `BankruptcyMessage`. If the names don't match exactly, you'll get compile errors like `BindWidget property 'SeasonLabel' not bound`.

---

## Step 2 + 5 — Import `DT_ShedActions` from the CSV (or skip entirely)

**You have 3 options for the action roster:**

### Option A — Skip (recommended for first PIE test)
Leave `ActionsTable` unassigned on the widget. The C++ falls back to `ShedActionMenu_DefaultRows()` which has the same 11 rows the CSV has. You'll see "using 11 default rows" in the log. **PIE works immediately.** Author the DT later when you want designers to edit it.

### Option B — Import the CSV as a one-shot
The repo already has `Content/UI/DT_ShedActions.csv` written for you (added in the same commit as this doc). To import:

1. In the Content Browser, navigate to `Content/UI/`.
2. Right-click → **Import to /Game/UI/** (or drag the CSV from Windows Explorer into the Content Browser).
3. UE5 pops the **DataTable Options** dialog:
   - **Row Type**: `FShedActionRow` (search for it; it's a USTRUCT in TaraGame).
   - **Ignore Extra Fields**: ✓ (CSV has an extra leading `---` row-name column).
   - Click **Apply** + **OK**.
4. UE5 creates `DT_ShedActions.uasset` next to the CSV. **Save it.**

### Option C — Author rows manually in the DataTable editor
If you want to start from scratch:
1. Right-click in `Content/UI/` → **Miscellaneous → Data Table** → pick `FShedActionRow` for the Row Struct.
2. Name it `DT_ShedActions`. Open it.
3. Add rows one by one. Names must be **valid FNames** (no spaces; use kebab-case). Pick `Season` from the dropdown (`Any` / `Wet` / `Dry`).

**Either way: when DT_ShedActions exists, open `WBP_ShedActionMenu` → in the Defaults panel (top-right) find `Actions Table` → assign `DT_ShedActions`. Save.**

> **Step 5 — Take-a-Vehicle row:** already in the CSV + defaults. `ActionId = "take-vehicle"`, `Season = Any`. The C++ base treats this id specially (bypasses bankruptcy + season filter, returns early in `HandleActionButtonClicked`). To pop your **vehicle-pick widget**, open `WBP_ShedActionMenu` → Graph view → override the function `HandleActionButtonClicked` (in the **Functions** dropdown, search for it). In the override: get the row whose ActionId matches `take-vehicle`, then call your `WBP_VehiclePick`'s `Create Widget` + `Add to Viewport`. The C++ base does NOT spawn the vehicle-pick widget itself — that's deliberate, since the vehicle-pick widget belongs to `TS-3D-PHASE2-shed-cottage`.

---

## Step 7 — Wire `BP_ShedHub` overlap → show widget

If `BP_ShedHub` doesn't exist yet (it's part of `TS-3D-PHASE2-shed-cottage` which isn't shipped yet), do this on whatever Actor the player overlaps when entering the shed. For a quick-and-dirty Phase 1 test, even a Trigger Box in the level works.

1. Select the shed actor in the level → in the Blueprint editor or Details panel, add a **Box Collision** child component if it doesn't have one. Size it to the shed footprint.
2. Click the Box Collision component → **Events** panel → click `+` on **On Component Begin Overlap**.
3. In the BP graph:
   - From the **Begin Overlap** exec pin, drag → search **Cast To FirstPersonCharacter** (or your player class). This filters out non-player overlaps.
   - From the Cast success pin → drag → **Get Game Instance** → **Get Subsystem (Tara Sim Subsystem)**.
   - Independent of the cast, drag → **Create Widget** → pick `WBP_ShedActionMenu`. (You don't need to pass the subsystem — the widget pulls it itself.)
   - From the Create Widget return value → **Add to Viewport** (Z-order 10 is fine).
   - Optional: also set **Show Mouse Cursor** = true on the player controller while the widget is open.
4. **Dismiss on Close button**: already wired in step 4 above — the widget's Close button calls `Close()` → `RemoveFromParent`.
5. Compile + Save.

> **Bankruptcy override note**: the menu handles bankruptcy itself — if `Subsystem->IsBankrupted()` is true, only the Take-a-Vehicle button is shown + the BankruptcyMessage TextBlock is visible. No special BP logic needed.

---

## Step 8 — PIE verify at day 200 (dry) + day 350 (wet) + bankruptcy

You need a way to fast-forward the SeasonClock. Two options:

### Option A — Console TickDay button (simplest)
Add a temporary debug Blueprint (or use an in-Editor utility actor) that calls `Subsystem->TickDay()` repeatedly. E.g. bind the `T` key to fire one `TickDay`. Walk into the shed, mash `T` until `DayOfYear` (visible in your HUD widget) is at the target day.

### Option B — One-time clock-set in the level BP
In the level Blueprint, on **BeginPlay** add an exec pin that does:
- Get subsystem → call `GetStationMutable()` → set `Clock.DayOfYear = 200` (or 350) → call the autosave. Restart PIE between season tests.

### Verification checklist

For each test, **screenshot the menu** + name the screenshot accordingly (`shed-dry-d200.png`, `shed-wet-d350.png`, `shed-bankrupt.png`):

- [ ] **Day 200 (Dry)** — walk into shed:
  - SeasonLabel reads "Dry season — cattle work"
  - **Enabled**: take-vehicle, muster, yard-work, order-supplement, sell-cattle, check-bore
  - **Greyed** (visible, ~30% opacity, tooltip on hover): repair-fence, grade-road, buy-machine, install-sensor, bush-mechanic
- [ ] **Day 350 (Wet)** — walk into shed:
  - SeasonLabel reads "Wet season — build mode"
  - **Enabled**: take-vehicle, repair-fence, grade-road, buy-machine, install-sensor, bush-mechanic
  - **Greyed**: muster, yard-work, order-supplement, sell-cattle, check-bore
- [ ] **Bankruptcy override** — open the level BP, set `Subsystem->GetStationMutable()->GetPlayer().Cash = -100000` + role to Owner + tick day 65 times. Check `IsBankrupted()` is true. Walk into shed:
  - All buttons except take-vehicle are **collapsed (hidden, not greyed)**
  - BankruptcyMessage is visible
  - Take-a-vehicle button stays enabled
- [ ] **Season-flip refresh** — open menu at day 334 (Dry). With menu open, run `Subsystem->TickDay()` from a debug key. Menu should re-evaluate to Wet (the OnSeasonChanged delegate fires). Verify SeasonLabel + button states flip live. (If they don't: the widget might need a manual close+reopen — the C++ has the subscription wired but UMG sometimes needs a `ForceLayoutPrepass` call after `SetIsEnabled` flips. Note in done_log if you hit this.)

---

## Step 9 — Done-log entry

Once all 4 PIE checks pass:

1. Save the screenshots in `backlog_bandit/decks/` (or wherever Max stores screenshots).
2. Open `backlog_bandit/done_log/2026-W21.md`.
3. Add an entry at the top following the format in [CLAUDE.md](../../backlog_bandit/CLAUDE.md#complete-id):
   ```
   ## 2026-MM-DD · [TS-3D-TWO-SEASON-MODE] Tara_Station — wet/dry shed-action-menu shipped
   **Done:** 2-4 sentences on the final action roster, what worked smoothly, what needed tweaking.
   **Files:** WBP_ShedActionMenu.uasset, DT_ShedActions.uasset (or "skipped — using C++ defaults"), Source/TaraGame/UI/ShedActionMenu.{h,cpp}, BP_ShedHub overlap wiring.
   **Plan:** [plans/done/TS-3D-TWO-SEASON-MODE.md](../plans/done/TS-3D-TWO-SEASON-MODE.md)
   **Notes:** any UMG quirks; whether the season-flip refresh worked live or needed close+reopen; final action set after playtest.
   ```
4. Move `plans/TS-3D-TWO-SEASON-MODE.md` → `plans/done/TS-3D-TWO-SEASON-MODE.md`. Update frontmatter `[A]` → `Done` + append done date.
5. Remove the BACKLOG row.
6. Commit + push `backlog_bandit`:
   ```
   git add -A
   git commit -m "chore(backlog): complete TS-3D-TWO-SEASON-MODE — wet/dry shed-action-menu shipped"
   git push origin main
   ```

---

## Troubleshooting

- **"BindWidget property not bound" compile error in WBP_ShedActionMenu** → variable name in the Designer doesn't match the C++ exactly. Match `SeasonLabel` (UTextBlock), `ActionList` (UVerticalBox), `BankruptcyMessage` (UTextBlock; this one is optional so missing is fine).
- **Buttons spawn but have no label** → make sure the loop in `BuildButtons` is using the latest `Row.Label.IsEmpty()` check; if your Designer pre-fills text, that text gets replaced. To pre-style the button (border, font), set it as the default on a custom `UButton` subclass and assign that subclass in the C++ `NewObject<UButton>` call instead of the default.
- **OnSeasonChanged never fires in PIE** → the bridge synthesises this from `OnDayEnded`. You need at least one `TickDay()` call to cross a season boundary (day 334→335 or 59→60). Verify in the log: every flip prints `[TaraSim] SeasonChanged: Wet -> Dry` (or vice versa).
- **Take-a-Vehicle button does nothing** → expected. The C++ base intentionally returns early for this row id. Implement the vehicle-pick behaviour in the BP override of `HandleActionButtonClicked`.
- **Reflection dispatch logs "bridge function 'X' takes parameters"** → C++ only auto-dispatches no-args functions. For `SellCattle(count)`, `BuyWorkMachine(type)`, etc., override `HandleActionButtonClicked` in BP and call the bridge action directly with the right arg.

---

When all 9 plan steps are ticked and the screenshots are in the done-log, `complete TS-3D-TWO-SEASON-MODE` to close it out.
