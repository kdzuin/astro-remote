// AstroRemote service worker — cache-first for offline use (ADR 0006).
//
// Bump CACHE_VERSION whenever any precached asset changes, or clients keep
// serving the old copy. Old caches are purged on activate.
const CACHE_VERSION = "astroremote-v1";

// Explicit precache list — every asset the app needs offline. Kept explicit
// (not a glob) so build artifacts like package.json / input.css / node_modules
// are never cached.
const PRECACHE = [
  "./",
  "index.html",
  "ble.js",
  "astro-status.js",
  "tailwind.css",
  "manifest.json",
  "icon.svg",
  "icon-192.png",
  "icon-512.png",
  "icon-180.png",
];

self.addEventListener("install", (event) => {
  event.waitUntil(
    caches
      .open(CACHE_VERSION)
      .then((cache) => cache.addAll(PRECACHE))
      .then(() => self.skipWaiting()),
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches
      .keys()
      .then((keys) =>
        Promise.all(
          keys.filter((k) => k !== CACHE_VERSION).map((k) => caches.delete(k)),
        ),
      )
      .then(() => self.clients.claim()),
  );
});

self.addEventListener("fetch", (event) => {
  const req = event.request;
  // Only handle same-origin GETs; let everything else hit the network.
  if (req.method !== "GET" || new URL(req.url).origin !== self.location.origin) {
    return;
  }
  event.respondWith(
    caches.match(req).then((cached) => {
      if (cached) return cached;
      return fetch(req)
        .then((resp) => {
          // Runtime-cache successful same-origin responses so navigations to
          // unlisted paths still work offline after first visit.
          if (resp.ok) {
            const copy = resp.clone();
            caches.open(CACHE_VERSION).then((c) => c.put(req, copy));
          }
          return resp;
        })
        .catch(() => cached);
    }),
  );
});
