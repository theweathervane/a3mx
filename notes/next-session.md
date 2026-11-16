# Next session: Blanka's shoulder cut in WIDE FORCED

Goal: draw a fighter's full art into the 48 px margins in WIDE FORCED. The walls stay where they are,
and nothing changes with WIDE off (docs/WIDESCREEN.md "Known issues" / "What to ensure").

## Found so far (static, 2026-11-16)

- The sprite drawers' dispatcher is **0x0880E150**. It picks one of four sibling grid drawers by the
  flag in t0 (`a3 & 0xFF`) and by `src + 164`:

  | flag | src+164 | drawer      | left cull in WIDE FORCED |
  |------|---------|-------------|--------------------------|
  | 0    | != 0    | 0x088112A8  | `-w < x` (unpatched)     |
  | 0    | == 0    | 0x08810D84  | `-w < x` (unpatched)     |
  | != 0 | != 0    | 0x08811BCC  | `-96 < x` (patched at 0x08811E28: `negu t0,a0` -> `addiu t0,zero,-0x60`) |
  | != 0 | == 0    | 0x088117D0  | `-w < x` (unpatched)     |

- Each drawer walks a grid of cells (bytes, 0xFF = empty) and draws a cell as a quad (0x089E1950)
  only if `x < DS+20` and `-w < x`. DS is the draw state at 0x08AD9320.
  - The cell width `w` is DS+16 (`DS+12/+16/+18` = 16/32/32 times the float scale 0x08E759E8/EC),
    or 0x26 in 088112A8's scaled branch (src+87).
  - **The right limit is fine:** DS+20 is set by 0x0880B928 from 480.0f in WIDE FORCED (patch word
    0x0880B978).
  - **The left limit is the suspect:** the patch widened it in 0x08811BCC only. Pieces in the left
    margin are dropped by the other three drawers. That fits the symptom: the cut is on the left,
    near the 4:3 edge, and it flickers as the cell grid moves with the fighter.
- All 35 patch words are decoded in `work/boot.widescreen.json` (orig -> new). The 11 at
  0x088EC0D4-0x088EC788 are `-192 -> -240` (the camera centre), not a cull.

## Still to confirm (needs a run on the Windows PC; the Linux shell here has no cmake)

1. Which drawer draws the fighters. Build with A3MX_WATCH, then Free Battle with Blanka at the left
   wall:
   `a3mx --watch 08AD9320-08AD9380 --watch-pc 088115E8 --watch-pc 088110C4 --watch-pc 08811A2C --watch-pc 08811E24`
   (the loads of `w` right before each drawer's left test). The PCs that report hits are the drawers
   in use.
2. Quick proof without decompiling: in WIDE FORCED, make that drawer's left test `-96 < x` as in
   0x08811BCC. If the shoulder comes back, decompile that drawer (docs/DECOMPILING.md) with the test as
   `rt_widescreen ? -96 : -w`, or better `-(w + margin)` from the planned `Wide` struct.
3. Parity: `wide_fight` and `arcade_fight`, then the new scenario (Free Battle, Blanka vs Blanka,
   both walk to the left wall, then to the right). Also check Zangief and Sagat.
