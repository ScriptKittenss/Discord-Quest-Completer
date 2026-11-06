// Ensures public/gamelist.json exists before build (Vite copies public/ into dist).
// The game list is 11MB and not committed to git — it is downloaded fresh from
// Discord's API (same source the release workflow uses). Never fails the build:
// if the download fails, the app falls back to fetching at runtime.
import { existsSync, mkdirSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const dest = join(root, 'public', 'gamelist.json');

if (existsSync(dest)) {
  console.log('gamelist.json already present, skipping download.');
  process.exit(0);
}

console.log('Downloading game list from Discord API...');
try {
  const res = await fetch('https://discord.com/api/v10/applications/detectable', {
    signal: AbortSignal.timeout(60000),
  });
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  const data = await res.text();
  if (!data || data.length < 100) throw new Error('downloaded data looks empty');
  mkdirSync(dirname(dest), { recursive: true });
  writeFileSync(dest, data);
  console.log(`Saved game list (${(data.length / 1024 / 1024).toFixed(1)} MB).`);
} catch (err) {
  console.warn(`Could not download game list (${err.message}). Continuing without it — the app will fetch at runtime.`);
}
