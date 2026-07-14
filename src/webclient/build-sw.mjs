// Stamp the service worker's CACHE_VERSION with a content hash of the precached
// assets, so the cache invalidates exactly when an asset changes — no manual
// bump, no chicken/egg with commit hashes (ADR 0006). Run via `npm run build`;
// the stamped sw.js is committed and CI verifies it is fresh.
import { readFileSync, writeFileSync } from "node:fs";
import { createHash } from "node:crypto";

const SW_PATH = new URL("./sw.js", import.meta.url);

// Assets whose bytes determine the cache version. sw.js cannot hash itself
// (circular), and "./" is an alias for index.html — both are excluded.
const HASHED_ASSETS = [
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

const hash = createHash("sha256");
for (const name of HASHED_ASSETS) {
  hash.update(name); // include the name so renames/reorders register
  hash.update(readFileSync(new URL(`./${name}`, import.meta.url)));
}
const version = "astroremote-" + hash.digest("hex").slice(0, 12);

const src = readFileSync(SW_PATH, "utf8");
const stamped = src.replace(
  /const CACHE_VERSION = "[^"]*";/,
  `const CACHE_VERSION = "${version}";`,
);
if (stamped === src && !src.includes(`"${version}"`)) {
  console.error("build-sw: could not find CACHE_VERSION line to stamp");
  process.exit(1);
}
writeFileSync(SW_PATH, stamped);
console.log(`build-sw: CACHE_VERSION = ${version}`);
