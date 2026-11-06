import { randomString } from '@/utils/random-string';
import type { Game } from '@/types/types';

export const fakeGames = async () => {
  return new Promise<Game[]>((resolve) => {
    fetch('gamelist.json')
      .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then((gameListData: Game[]) => {
        const games = gameListData.slice(0, 30);
        const sorted = games.sort((a, b) => a.name.localeCompare(b.name));
        resolve(sorted.map((game: Game) => {
            game['uid'] = randomString()
            return game
        }));
      })
      .catch(() => resolve([]));
  });
}
