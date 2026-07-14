# Flicker-free screens via off-screen canvases

Drawing directly to the display and clearing with `fillScreen` between frames
causes visible flicker — the screen briefly shows the cleared (black) state
before the new content lands. This was obvious on the Astro run screen, whose
stats refresh once per second.

Decision: screens that repaint frequently draw into an **off-screen M5Canvas**
and `pushSprite` once. A single blit is atomic, so no intermediate cleared
state is ever shown — no flicker.

The Astro run screen goes further and uses **two canvases pushed to
non-overlapping regions**:

- **top** — title + action menu (Pause/Resume, Stop), pushed only when the
  selection or run state changes;
- **bottom** — live stats + the colour-coded status bar, pushed once per second.

Because each `pushSprite` only touches its own rect, the per-second stats
refresh never repaints the menu above it.

## Considered alternatives

- **One full-screen canvas, redraw everything each tick.** Simpler, and still
  flicker-free (one atomic push). We kept two canvases as an optimisation — the
  static menu is not re-rendered every second — but a single canvas would be a
  reasonable simplification if the split ever becomes a burden.
- **Draw straight to the display (no canvas).** Rejected: this is the flicker
  source.

## Consequences

- Two 16bpp sprites on the M5StickC (80x160) cost ~26KB RAM total; fits with
  wide margin (RAM sat ~14%).
- `SelectableList::draw` can render into a canvas (see ADR 0004), so the menu
  region reuses the real component rather than duplicating its layout.
- Config screens still draw directly to the display and can flicker; only the
  Astro run screen adopts canvases so far.
