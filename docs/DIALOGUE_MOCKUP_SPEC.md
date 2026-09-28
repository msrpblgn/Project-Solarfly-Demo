# Dialogue Behavior Mockup Specification (Milestone 1)

## Status

| Field | Value |
| ----- | ----- |
| Milestone | 1 of 8 — Aseprite behavior mockup |
| Result | **Milestone 1: USER VISUALLY APPROVED** |
| Runtime note | Runtime appearance remains subject to later user feedback. |
| Audit date | 2026-09-03 |
| Approval date | 2026-09-04 |
| Aseprite version | 1.3.18.3-x64 |
| Build command | `python tools/dialogue_mockup/build_dialogue_mockup.py` |
| Round-trip command | Aseprite batch scripts in `tools/dialogue_mockup/aseprite/` (invoked by build script) |
| Renderer reference outputs | `assets_src/ui/dialogue/dialogue_frame_*.png` (flattened, guides excluded) |
| Editable master | `assets_src/ui/dialogue/dialogue_behavior_mockup.aseprite` |

**Milestone 1 visual presentation is user-approved.** Milestones 2–4 implement panel, printer, and authored pacing (**automated checks passed**). Milestone 5 runtime bust portraits are **USER RUNTIME APPROVED** (mGBA: speaker switches, replacement, menu return). Milestone 6 adds the reusable scene runner and host integration. Dialogue source-language compiler, branching, and campaign remain later. Audio remains callback-only.

## Presentation size vs stored source size

| Role | Size | File |
| ---- | ---- | ---- |
| Stored source (Aksil) | 64 × 64 | `assets_src/sprites/aksil_front_sprite.png` |
| Stored source (Protagonist) | 66 × 66 | `assets_src/sprites/protagonist_front_sprite.png` |
| Normalized candidate | 64 × 64 | `protagonist_front_64x64_candidate.png`, `*_4bpp_candidate.png` |
| **Mockup presentation** | **128 × 128** | `aksil_front_128x128_presentation.png`, `protagonist_front_128x128_presentation.png` |

Presentation is a **fixed 2× nearest-neighbor** enlargement of each 64×64 4bpp candidate. Character pixels themselves are doubled. Transparency stays binary 0/255. Active and inactive palettes are enlarged independently; no zoom animation, smoothing, rotation, or interpolation.

**128×128 is mockup presentation only.** It does not prove that Milestone 0 OBJ reservations can display a 128×128 sprite at runtime. Later portrait-rendering work must reevaluate OBJ tile VRAM, OAM count, and whether a bust atlas or multi-OBJ join is required. See `docs/DIALOGUE_RESOURCE_MAP.md`.

## Source asset hashes (unchanged)

| File | SHA-256 |
| ---- | ------- |
| `assets_src/ui/Dialogue Ui.png` | `bf25ec599900e1443c725879ea4e200a65426249448d00f18f47e48be5a0072b` |
| `assets_src/sprites/aksil_front_sprite.png` | `2df139faf57bc5de6183ef46dae789cd02aa3b7de95bf445266cd31ccc83970d` |
| `assets_src/sprites/protagonist_front_sprite.png` | `5edfe6308c740594d0e8a33583b87cf7199eb73a08427c812a8f565d29de0863` |

## Canvas

| Property | Value |
| -------- | ----- |
| Size | 240 × 160 |
| Pixel aspect | 1:1 square pixels |
| Working mode | RGB sprite in Aseprite; PNG layers use RGBA |
| Anti-aliasing | None |
| Scaling | 2× NN for presentation canvases; 4× NN for review sheets/GIFs |

## Layer stack (top → bottom)

1. `GUIDES_do_not_export` — safe-area boxes, pause notes (**not exported**)
2. `ADVANCE_PROMPT` — continuation marker (visible/hidden states)
3. `TEXT` — engine 5×7 text
4. `NAMEPLATE` — provisional speaker tab
5. `FRAME_FRONT_MASK` — **pixel-identical** copy of `Dialogue Ui.png`
6. `PORTRAIT_RIGHT`
7. `PORTRAIT_LEFT`
8. `WORLD_PREVIEW_do_not_export` — flat green field preview (**non-runtime**)
9. `BACKGROUND_FILL_do_not_export` — darker fill (**non-runtime**)

Aseprite stores layers bottom-to-top internally; names match the list above.

## Frame tags

| Tag | Frame # | Purpose |
| --- | ------- | ------- |
| `frame_a` | 1 | Aksil active |
| `frame_b` | 2 | Protagonist active |
| `frame_c` | 3 | Left-side replacement technical test |

Motion previews are exported as GIF + timing JSON (not separate Aseprite tags in this milestone).

## Dialogue frame geometry

| Property | Value |
| -------- | ----- |
| Position | x=0, y=0 |
| Size | 240 × 160 |
| Opaque panel bounds | x=0, y=108, w=240, h=51 |
| Opaque y interval (half-open) | **[108, 159)** — inclusive rows 108–158 |
| First fully solid row | **y = 116** (rows 108–115 are the decorative arch with center holes) |
| Transparent bottom row | **y = 159** (fully transparent; portraits must not leak here) |
| Authority | `Dialogue Ui.png` (do not redraw) |

### Portrait leak-clip (separate from the supplied frame)

`FRAME_FRONT_MASK` remains a pixel-identical copy of `Dialogue Ui.png` and stays above both portrait layers. Because the opaque bounding interval is not solid in every pixel, portraits use an additional clip region:

| Region | Portrait pixels |
| ------ | --------------- |
| y < 116 | Visible bust (heads, shoulders, upper chest) |
| 116 ≤ y < 159 | Kept only where the supplied frame is opaque (behind the panel) |
| y = 159 | Never filled |

This makes the **solid panel edge** the torso cutoff. There is no floating crop line above the panel. Portrait pixels do not appear in the text area or through the transparent bottom row. The supplied frame is not painted over to hide leaks.

## Portrait presentation (128 × 128)

Old 64×64 on-screen anchors are superseded. Placement uses 2× anatomical landmarks, not doubled old coordinates.

### Local presentation landmarks (128×128 canvas)

| Character | Opaque bounds (x0,y0,x1,y1 half-open) | Head (top 40%) | Shoulder / upper-chest band | Notes |
| --------- | ------------------------------------- | -------------- | --------------------------- | ----- |
| Aksil | (38, 6, 84, 126) | y 6–51, x 40–83 | ~y 44–80 | Narrower body; left-padded like the 64px source |
| Protagonist | (38, 6, 106, 126) | y 6–51, x 38–81 | ~y 44–80 | Wider body; hair/shoulders occupy more of the canvas |

Chest cutoff landmarks (local row hidden by panel solid y=116):

| Pose | Anchor y | First hidden local row | Visible above panel |
| ---- | --------: | ---------------------: | ------------------- |
| Active | 36 | 80 | Head, neck, shoulders, upper chest |
| Inactive (rest) | 44 | 72 | Same, with 8 px less upper chest |

### Canvas anchors

| Slot | Active (x, y) | Inactive (x, y) |
| ---- | ------------- | --------------- |
| Left (Protagonist) | **−8, 36** | **−16, 44** |
| Right (Aksil) | **116, 36** | **124, 44** |

Inactive is 8 px down and 8 px outward from active. Outer shoulders may clip at screen edges; complete heads remain on-screen in both settled poses. Per-character x is the same because both 2× sprites share left opaque x=38; visual centers stay roughly mirrored around x=120.

The dialogue box and text area are not moved.

### Frame palette (RGBA)

| Role | RGBA |
| ---- | ---- |
| Transparent | `#00000000` |
| Black | `#000000FF` |
| Orange | `#FC9C00FF` |
| Gold | `#FDBF5AFF` |
| Light gold | `#FFD38CFF` |

## Text safe area (provisional — engine 5×7)

| Property | Value |
| -------- | ----- |
| Font | Embedded 5×7 from `src/core/video.c`, plus a **mockup-only** `?` glyph |
| Character advance | 6 px |
| Text start x | 12 |
| Line 1 y | 132 |
| Line 2 y | 144 |
| Max text end x | 221 |
| Usable width | 210 px |
| Max chars / line | 35 (`210 / 6`) |
| Text color | Black `#000000` |
| Lowercase | Engine supports limited punctuation; mockup strings are uppercase-only |
| Question mark | Present in Frame A (`YOU THOUGHT THAT WOULD WORK?`). The engine font in `src/core/video.c` still maps `?` to space; the mockup glyph is **not** a runtime font change. |

## Nameplate (provisional)

Derived from `Dialogue Ui.png` opaque top row **y = 108**. Placement attaches to the frame top and stays below visible **heads** (not full-body bottoms, which would collide with the 128px busts).

| Speaker | Label | Rectangle (x, y, w, h) | Notes |
| ------- | ----- | ---------------------- | ----- |
| Aksil (Frame A) | `AKSIL` | 194, 106, 38, 11 | Canonical name; right attachment |
| Protagonist (Frame B) | `PROTAGONIST` | 8, 106, 74, 11 | **Development width-stress label only** |

Both left and right nameplates share **y = 106** (bottom row 116 is the first fully solid panel row). Faces remain clear of the plates. Hidden during Frame C.

### Frame A text

```text
YOU THOUGHT THAT WOULD WORK?
COME ON. ONE MORE TRY.
```

Guide-only pause marker after `WORK?` (not runtime).

### Frame B text

```text
IT ALMOST DID.
```

Line 2 intentionally empty.

## Advance prompt

| Property | Value |
| -------- | ----- |
| Rectangle | x=228–233, y=149–154 (6×6) |
| Layer | `ADVANCE_PROMPT` |
| Visible | Frames A and B |
| Hidden | Frame C |

## Focus exchange (5 unique frames)

Palette swap at **frame 2** (0-based). Travel is **8 px up and 8 px inward** from inactive to active, applied around the new bust anchors. Both portraits stay at 128×128 throughout. No overshoot, bounce, scale, or alpha fade. No duplicate endpoint holds inside the focus-motion tag. The panel, text baselines, and nameplate y stay stationary; the nameplate switches sides with the speaker.

| Frame | Left (x, y) | Right (x, y) | Palette state |
| ----: | ----------- | ------------ | ------------- |
| 0 | −16, 44 | 116, 36 | left inactive, right active |
| 1 | −14, 42 | 118, 38 | left inactive, right active |
| 2 | −12, 40 | 120, 40 | **swap** → left active, right inactive |
| 3 | −10, 38 | 122, 42 | left active, right inactive |
| 4 | −8, 36 | 124, 44 | left active, right inactive |

Incoming speaker rises and moves inward, revealing a little more upper chest above the panel. Outgoing speaker settles down and outward. This is a subtle rise, not an entrance from below.

## Protagonist normalization

| Property | Value |
| -------- | ----- |
| Source | 66 × 66 `protagonist_front_sprite.png` (unchanged) |
| Candidate | `protagonist_front_64x64_candidate.png` |
| Crop | `(1, 1, 65, 65)` — removes one transparent border row/column per side |
| Opaque pixels preserved | 968 / 968 |
| Method | Nearest-neighbor crop only; no interpolation |

## Portrait 4bpp candidates

Separate OBJ palette banks (Milestone 0). Max 16 colors each (index 0 transparent). No dithering. Binary alpha. Colors quantized to GBA BGR555.

Palette tables are recorded in `assets_src/ui/dialogue/build/mockup_config.json` under `palettes`.

Comparison sheets:

- `portrait_palette_comparison_1x.png`
- `portrait_palette_comparison_4x.png`

**Candidates are not promoted over source assets until user approval.**

## Behavior frames summary

### Frame A — Aksil active

- Protagonist left inactive (dim), Aksil right active
- Nameplate `AKSIL` on right
- Two text lines + advance prompt

### Frame B — Protagonist active

- Protagonist left active, Aksil right inactive (dim)
- Nameplate `PROTAGONIST` on left (dev label)
- One text line + advance prompt
- Text baselines identical to Frame A

### Frame C — technical replacement test

- **Not story canon**
- Aksil exiting left (linear sequenced handover still; Protagonist fully offscreen)
- Right slot empty; no nameplate, text, or prompt
- Portraits behind `FRAME_FRONT_MASK` with the leak-clip applied
- Documents need for an approved third portrait for persistent right-side opposition

## Motion previews

| Sequence | Frames | FPS | Notes |
| -------- | ------ | --- | ----- |
| Focus exchange | 5 | 60 | Integer positions; 8 px up/in; palette swap at frame 2 |
| Portrait enter (left) | 10 | 60 | Ease-out slide from x=**−128** to x=**−8** |
| Portrait exit (left) | 10 | 60 | Ease-out slide from x=**−8** to x=**−128** |
| Replacement (left) | 10 | 60 | **Sequenced linear**: Aksil exits fully, then Protagonist enters. Faces do not cross. |

Offscreen for a 128-wide presentation canvas: left **x = −128**, right **x = 240**. Stale x=−64 endpoints are removed.

Enlarged simultaneous replacement would pass faces through one another, so exit and entry are sequenced. The existing non-canonical test still uses only the supplied Aksil and Protagonist portraits.

### Timing policy

| Field | Value |
| ----- | ----- |
| Runtime / logical FPS | **60** exact (`1000/60 ≈ 16.666… ms` per logical frame) |
| Logical motion length | 35 frames → `35 × 1000/60 ≈ 583.333 ms` |
| GIF delay unit | 10 ms |
| Quantization | Cumulative timestamps are rounded to the nearest 10 ms; encoded delays are differences of those quantized ends |
| Identical-frame collapse | Allowed; collapsed holds accumulate their display time |
| Native GIF speed | `1.0×` intended timeline |
| 4× review GIF speed | `12.0×` slower (`12 × 1000/60 = 200 ms` per logical frame) |
| Frame strip | Always shows all **35** logical frames |
| Sentinel pixels | **Forbidden** — preview pixels must match flattened composition |

Timing manifest: `assets_src/ui/dialogue/build/dialogue_motion_timing.json`

GIF outputs:

- `dialogue_motion_preview.gif` — native 1× preview at intended 60 fps timeline
- `dialogue_motion_preview_4x.gif` — 4× nearest-neighbor, **labeled 12× slower than 60 fps**
- `dialogue_focus_exchange_preview.gif` — native focus-only chest-rise preview
- `dialogue_focus_exchange_preview_4x.gif` — same, labeled 12× slower
- `dialogue_mockup_before_after_4x.png` — old 64px vs new 128px bust at identical 1× screen scale, then 4× NN

Focus position strip: `assets_src/ui/dialogue/build/dialogue_focus_strip.png`

Full motion frame strip (35 logical frames with coordinates): `assets_src/ui/dialogue/build/dialogue_motion_frame_strip.png`

The GIF is a human-review artifact. The timing manifest is the source of truth for logical frame count, intended duration, encoded mapping, and measured duration. Encoded GIF frame count may be smaller than 35 when consecutive images are identical.

## Deliverables

| File | Purpose |
| ---- | ------- |
| `dialogue_behavior_mockup.aseprite` | Editable layered master |
| `dialogue_frame_a_aksil_active.png` | Programmer reference — Frame A |
| `dialogue_frame_b_protagonist_active.png` | Programmer reference — Frame B |
| `dialogue_frame_c_replacement.png` | Programmer reference — Frame C (sequenced mid-exit) |
| `dialogue_motion_preview.gif` | Full motion review at intended speed |
| `dialogue_motion_preview_4x.gif` | 4× motion review, labeled 12× slower |
| `dialogue_focus_exchange_preview.gif` | Focus-only chest-rise preview |
| `dialogue_focus_exchange_preview_4x.gif` | Focus review, labeled 12× slower |
| `dialogue_mockup_review_sheet_4x.png` | Static 4× nearest-neighbor review sheet |
| `dialogue_mockup_before_after_4x.png` | Old 64px vs new 128px at identical screen scale |
| `portrait_palette_comparison_1x.png` | Palette review |
| `portrait_palette_comparison_4x.png` | Palette review 4× |
| `protagonist_front_64x64_candidate.png` | Normalized 64×64 source candidate |
| `aksil_front_4bpp_candidate.png` | Active 4bpp candidate (64×64) |
| `protagonist_front_4bpp_candidate.png` | Active 4bpp candidate (64×64) |
| `aksil_front_128x128_presentation.png` | Derived 2× presentation (do not overwrite sources) |
| `protagonist_front_128x128_presentation.png` | Derived 2× presentation (do not overwrite sources) |

## Provisional vs requires approval

| Item | Status |
| ---- | ------ |
| `Dialogue Ui.png` silhouette | Requires user approval |
| **128×128 bust size, torso cutoff, lower rest, speaker rise** | **Judge this revision** |
| 4bpp palette reductions | Previously accepted; retained |
| Active vs inactive contrast | Previously accepted; retained |
| Protagonist 64×64 normalization | Previously accepted; retained |
| Nameplate shape / attachment | Provisional — requires approval |
| Text position / two-line capacity | Provisional — requires approval |
| Advance prompt design | Provisional — requires approval |
| Enter / exit / sequenced replacement motion | Requires user approval |
| `PROTAGONIST` nameplate label | Development-only width stress |

## Milestone 0 reservation note

No Milestone 0 VRAM reservations were changed. Dialogue remains Mode 0 overlay planning only. The 128×128 mockup presentation is not a runtime allocation change.
