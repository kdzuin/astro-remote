# SelectableList::draw is templated on the render target

`SelectableList::draw` takes its render target as a template parameter
(`template <typename Target> void draw(Target&)`) rather than a concrete
`lgfx::LovyanGFX*` base pointer. This lets the same code draw to the live
display, an off-screen `M5Canvas`, or the native test's display stub — none of
which share a common base the native build can see.

The driver is host-testability: the `native` unit-test environment has no M5GFX
at all (it links a tiny `DisplayStub` with the same method names). A signature
typed to `LovyanGFX*` would force M5GFX into the test build; a duck-typed
template binds structurally to whatever exposes `width/fillScreen/drawString/…`.

## Consequences

- The draw body lives in the header/`.tpp` (templates must), which is already
  the case for this class.
- Any target must implement the LovyanGFX-style drawing API surface used here;
  a missing method is a compile error at the call site, not a link error.
- Off-screen canvases are how flicker is avoided (draw into a sprite, push once)
  — e.g. the Astro run screen renders its menu into a canvas via this same
  `draw`, with `clearFirst=false` since the canvas is cleared per frame.
