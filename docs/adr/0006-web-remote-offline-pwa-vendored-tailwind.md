# Web remote as an offline PWA with vendored Tailwind

The web client (`src/webclient/`) is the browser-side **remote link** interface:
a static Web Bluetooth page that duplicates the stick's buttons and now also
consumes the astro-status broadcast to show live **sequence** progress. It is
meant to be used in the field, on a phone, next to a camera — often with no
usable network. Two constraints drive this decision:

1. **It must work offline.** Astrophotography happens in dark fields; the page
   should load and run with no connectivity after the first visit.
2. **It cannot run in iOS Safari.** Apple does not implement Web Bluetooth in
   Safari at all. On iPhone the page only works in a Web-BLE browser such as
   **Bluefy**; on desktop/Android it works in Chrome/Edge.

Decision:

- **Ship as a PWA.** Add a `manifest.json` and a **cache-first service
  worker** that caches every asset (`index.html`, `ble.js`, `tailwind.css`,
  the manifest, icons) on first load. After that the app opens with no network
  and is installable / add-to-home-screen.
- **Vendor Tailwind as a pinned, pre-built stylesheet** rather than the CDN
  runtime build. Generate `tailwind.css` once with
  `npx @tailwindcss/cli@<pinned> --minify` (pinned at latest 4.x at build
  time), commit the output, and serve it same-origin. Utility classes stay in
  the markup unchanged.
- **Target any Web-BLE browser.** Detect `navigator.bluetooth`; when absent
  (e.g. iOS Safari) show a clear message pointing at Bluefy. iPhone is a
  first-class target *through Bluefy*, not Safari.

## Why

Offline + "always latest Tailwind" pull in opposite directions, and the CDN
build resolves them badly:

- `@tailwindcss/browser` (the CDN `<script>`) is a **runtime JIT compiler**
  (~300KB+ of JS that scans the DOM and generates CSS on every load), which
  Tailwind explicitly says not to ship to production. Caching it for offline
  means caching a compiler to avoid caching a stylesheet.
- A CDN URL like `@4` is a **moving target**: once the service worker caches
  it, the app is silently frozen at whatever version cached first — the worst
  of both "latest" and "pinned".

Vendoring a built CSS file gives a small, same-origin (non-opaque, cleanly
cacheable) stylesheet, faster first paint, and a **deterministic** version
that we bump on purpose by re-running the CLI.

## Considered alternatives

- **Keep the Tailwind CDN `<script>`, cache it in the service worker.**
  Rejected: ships a runtime compiler, freezes at first-cached version, slower
  first paint. Least code change, worst runtime properties.
- **Drop Tailwind, hand-write dark CSS with variables.** Fully self-contained,
  smallest payload, no toolchain — but discards the existing utility-class
  markup and the styling velocity Tailwind gives. Reasonable, not chosen
  (keeping Tailwind was an explicit preference).
- **No service worker, rely on the browser HTTP cache.** Rejected: not a
  reliable offline guarantee and not installable.
- **Treat iOS Safari as a target.** Impossible today — no Web Bluetooth in
  Safari. Documented rather than worked around.

## Tooling

This is the repo's **first JavaScript toolchain**; it is deliberately
right-sized to one static leaf, not a monorepo:

- A single **`src/webclient/package.json`** (colocated with the code, C++ root
  untouched) with devDeps `tailwindcss` + `@tailwindcss/cli` pinned to an
  explicit 4.x, a committed lockfile, and scripts: `build:css`
  (`tailwindcss -i input.css -o tailwind.css --minify`), `build:sw` (stamps the
  service worker's `CACHE_VERSION` — see below), `build` (both), and `test`
  (`node --test`, the built-in runner — zero extra deps).
- The **service-worker cache version is content-derived**, not manual:
  `build-sw.mjs` hashes the precached assets (sha256, first 12 hex) and stamps
  `CACHE_VERSION` into `sw.js`. This invalidates the offline cache exactly when
  an asset changes and never otherwise — avoiding both stale clients (forgot to
  bump) and needless churn. A commit hash was rejected: the SW is committed
  before its own commit exists (chicken/egg → always one stale) and would churn
  the cache on every commit regardless of whether web assets changed. The
  committed `tailwind.css` and stamped `sw.js` are both verified fresh in CI.
- **`node_modules/` is git-ignored** so it never reaches the checkout, and thus
  never the GitHub-Pages artifact (which uploads `src/webclient/` as-is).
- **Pure logic is extracted from `ble.js`** into an importable module
  (packet decode, timer interpolation, time/'bar-fraction formatting) so it is
  testable without a DOM, honouring the repo's tests-first rule.
- **Enforcement mirrors the native tests**: the `.githooks/pre-commit` hook
  runs `node --test` for the web client (guarded — skipped if `node` is absent,
  like the existing `pio` guard), and a small CI job runs them too so a
  `--no-verify` commit or an unhooked clone is still caught.

**Not Nx / not a monorepo.** Nx (or any monorepo tool) manages a graph of
several interdependent JS packages; here there is exactly one static leaf beside
a PlatformIO C++ build that Nx cannot see. The overhead (`nx.json`, project
graph, cache/daemon, generators) buys nothing and pushes against the zero-build
static deploy. Revisit only if the front end grows to ~3+ shared packages.

## Consequences

- The repo gains a tiny **build step for CSS only**: an `input.css` source
  (`@import "tailwindcss";` plus any `@theme` tokens) and the committed,
  generated `tailwind.css`. Deploy stays zero-build — GitHub Pages / Bluefy
  serve the static output. Bumping Tailwind = rerun the pinned CLI, recommit.
- A service worker means **cache invalidation is now our problem**: shipping
  new web-client assets requires bumping the SW cache version, or users keep
  the cached copy.
- The page is **dark-only** (no theme toggle) to suit night use; this pairs
  with the PWA `theme_color`/`background_color` in the manifest.
- iPhone users must be told to open the page in **Bluefy**; the in-page
  unsupported-browser message carries that instruction.
