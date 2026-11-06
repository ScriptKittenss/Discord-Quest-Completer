// Generates all Tauri app icons from public/logo.png (cat photo).
// Run: node scripts/generate-icons.mjs
// The binary icons are build artifacts (gitignored) — logo.png is the source of truth.
import sharp from 'sharp';
import pngToIco from 'png-to-ico';
import { Icns, IcnsImage } from '@fiahfy/icns';
import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const iconsDir = join(root, 'src-tauri', 'icons');
const logo = join(root, 'public', 'logo.png');
mkdirSync(iconsDir, { recursive: true });

const png = (size, name) =>
  sharp(logo).resize(size, size).png().toFile(join(iconsDir, name)).then(() => console.log('wrote', name));

await png(32, '32x32.png');
await png(64, '64x64.png');
await png(128, '128x128.png');
await png(256, '128x128@2x.png');
await png(512, 'icon.png');
for (const [size, name] of [[30,'Square30x30Logo.png'],[44,'Square44x44Logo.png'],[71,'Square71x71Logo.png'],[89,'Square89x89Logo.png'],[107,'Square107x107Logo.png'],[142,'Square142x142Logo.png'],[150,'Square150x150Logo.png'],[284,'Square284x284Logo.png'],[310,'Square310x310Logo.png'],[50,'StoreLogo.png']]) {
  await png(size, name);
}

// .ico for Windows
const icoBuf = await pngToIco(join(iconsDir, 'icon.png'));
writeFileSync(join(iconsDir, 'icon.ico'), icoBuf);
console.log('wrote icon.ico');

// .ico for the dummy-game runner (embedded via src-win/embed_icon.rc)
writeFileSync(join(root, 'src-win', 'icon.ico'), icoBuf);
console.log('wrote src-win/icon.ico');

// .icns for macOS
const icns = new Icns();
for (const [size, osType] of [[16,'icp4'],[32,'icp5'],[64,'icp6'],[128,'ic07'],[256,'ic08'],[512,'ic09']]) {
  const buf = await sharp(logo).resize(size, size).png().toBuffer();
  icns.append(IcnsImage.fromPNG(buf, osType));
}
writeFileSync(join(iconsDir, 'icon.icns'), icns.data);
console.log('wrote icon.icns');

console.log('done');
