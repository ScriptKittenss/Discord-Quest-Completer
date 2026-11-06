import { type Game, type GameExecutable } from '@/types/types';

/**
 * Curated map of known executables for Discord games that have empty
 * or missing executables in Discord's official detectable.json registry.
 */
export const GAME_EXECUTABLE_OVERRIDES: Record<string, GameExecutable[]> = {
  // AION 2 (Application ID: 1445982809792708628, Steam App ID: 3393110)
  '1445982809792708628': [
    { name: 'AION2.exe', os: 'win32', is_launcher: false },
    { name: 'Binaries/Win64/AION2.exe', os: 'win32', is_launcher: false },
    { name: 'Aion2/Binaries/Win64/AION2.exe', os: 'win32', is_launcher: false },
    { name: 'aion2.exe', os: 'win32', is_launcher: false },
    { name: 'aion.bin', os: 'win32', is_launcher: false },
  ],
  // Aion Free-to-Play
  '451547384975982602': [
    { name: 'aion.bin', os: 'win32', is_launcher: false },
    { name: 'Aion.bin', os: 'win32', is_launcher: false },
    { name: 'AION.exe', os: 'win32', is_launcher: false },
  ],
  // Aion Original
  '451543892358201393': [
    { name: 'aion.bin', os: 'win32', is_launcher: false },
    { name: 'Aion.bin', os: 'win32', is_launcher: false },
    { name: 'AION.exe', os: 'win32', is_launcher: false },
  ],
};

/**
 * Ensures any game with missing or empty executables in Discord's database
 * has valid, runnable executables for process simulation so Discord Quests work.
 */
export function enrichGameExecutables(game: Game): Game {
  const existingExecutables = game.executables || [];
  
  // 1. Check explicit override by Application ID
  if (GAME_EXECUTABLE_OVERRIDES[game.id]) {
    const overrides = GAME_EXECUTABLE_OVERRIDES[game.id];
    // Merge without duplicates
    const merged = [...existingExecutables];
    for (const ov of overrides) {
      if (!merged.some(e => e.name.toLowerCase() === ov.name.toLowerCase())) {
        merged.push(ov);
      }
    }
    return {
      ...game,
      executables: merged,
    };
  }

  // 2. If game already has valid executables, return as is
  if (existingExecutables.length > 0) {
    return game;
  }

  // 3. Fallback: generate smart Windows executables for games with empty registry
  const rawName = game.name || 'game';
  const cleanName = rawName.replace(/[^a-zA-Z0-9]/g, '');
  const hyphenName = rawName.toLowerCase().replace(/[^a-z0-9]+/g, '-');
  const underscoreName = rawName.toLowerCase().replace(/[^a-z0-9]+/g, '_');

  const generated: GameExecutable[] = [
    { name: `${cleanName}.exe`, os: 'win32', is_launcher: false },
    { name: `${rawName}.exe`, os: 'win32', is_launcher: false },
    { name: `Binaries/Win64/${cleanName}.exe`, os: 'win32', is_launcher: false },
    { name: `${hyphenName}.exe`, os: 'win32', is_launcher: false },
    { name: `${underscoreName}.exe`, os: 'win32', is_launcher: false },
  ];

  return {
    ...game,
    executables: generated,
  };
}
