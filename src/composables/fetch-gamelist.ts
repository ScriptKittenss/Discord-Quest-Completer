import { Game } from '@/types/types';
import { tryOnMounted, useAsyncState } from '@vueuse/core';
import { ref, shallowRef, watch } from 'vue';
import { message } from '@tauri-apps/plugin-dialog'; 
import { invoke } from '@tauri-apps/api/core';
import { useGlobalState } from './app-state';
import { enrichGameExecutables } from '@/utils/game-overrides';

export function useFetchGameList() {
    const { addLog } = useGlobalState();
    async function fetchGameListGHMirror() {
        addLog('Fetching game list from GitHub mirror...'); 
        const response = await invoke('fetch_gamelist_gh_mirror');
        return response as Game[] | unknown[] | undefined;
    }
    async function fetchGameListFromDiscord (){
        addLog('Fetching game list directly from discord...'); 
        const response = await invoke('fetch_gamelist_from_discord');
        return response as Game[] | unknown[] | undefined;
    };

    const { 
        state: gameListGHMirror,
        error: errorGH,
        isReady: isReadyGH,
        execute: executeGH,
        isLoading: isLoadingGH
    } = useAsyncState<Game[] | unknown[] | undefined>(fetchGameListGHMirror, [], {
            immediate: false,
            resetOnExecute: true,
        });
    const { 
        state: gameListFromDiscord, 
        error: errorDiscord,
        isReady: isReadyDiscord,
        execute: executeDiscord,
        isLoading: isLoadingDiscord
    } = useAsyncState(fetchGameListFromDiscord, [], {
        immediate: false,
        resetOnExecute: true,
    });
    const { 
        state: bundledGameList,
        error: errorBundled,
        isReady: isReadyBundled,
        execute: executeBundled,
        isLoading: isLoadingBundled
    } = useAsyncState(() => {
        // Native window.fetch: Tauri serves the bundled dist/ assets (including
        // public/gamelist.json) at the app origin, so a same-origin fetch loads
        // the list straight from the exe — no network needed. The HTTP-plugin
        // fetch used before is a real HTTP client and cannot fetch a local
        // path at all, which is why the bundled list never loaded in any
        // build. (tauri.conf.json connect-src now also allows 'self' so the
        // CSP doesn't block this same-origin fetch.)
        const result = window.fetch('/gamelist.json', { signal: AbortSignal.timeout(10000) }).then(res => {
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            return res.json();
        });
        addLog('Fetching bundled game list for fallback...');
        return result;
    }, [], {
        immediate: false,
        resetOnExecute: true,
    });

    const fetchError = ref<string | null>(null);

    const gameDB = shallowRef<Game[]>([]);

    const allFetchDone = ref(false);

    function isValidGameList(data: any): boolean {
        return Array.isArray(data) && data[0] && 'aliases' in data[0] && 'name' in data[0] && 'executables' in data[0];
    }

    watch(() => isReadyGH.value, async (newVal) => {
        addLog('debug','isReadyGH: ' + newVal); 
    });

    watch(() => isReadyDiscord.value, async (newVal) => {
        addLog('debug','isReadyDiscord: ' + newVal);
    })
    
    watch(() => isReadyBundled.value, async (newVal) => {
        addLog('debug','isReadyBundled: ' + newVal); 
    });

    let timeoutId: ReturnType<typeof setTimeout> | null = null;
    function markDone() {
        timeoutId = setTimeout(() => {
            allFetchDone.value = true;
        }, 200);
    }

    function useList(source: string, data: any) {
        gameDB.value = ((data as Game[]) || []).map(enrichGameExecutables);
        addLog(`Using game list from ${source}. ` + (data as Game[]).length + ' entries.');
    }

    // Background refresh: swap in the fresh mirror list when it arrives.
    // Best-effort only — the bundled list is already loaded by now, so a
    // mirror failure is not an error: log it as a warning and keep the
    // bundled list (e.g. when the PC's network/VPN can't reach the mirror).
    async function refreshFromMirror() {
        try {
            await executeGH();
        } catch {
            addLog('warning', 'GitHub mirror refresh failed, keeping bundled game list.');
            return;
        }
        if (errorGH.value) {
            addLog('warning', 'GitHub mirror refresh failed, keeping bundled game list.');
            return;
        }
        if (gameListGHMirror.value && gameListGHMirror.value?.length > 0 && isValidGameList(gameListGHMirror.value)) {
            useList('GitHub mirror', gameListGHMirror.value);
            addLog('Updated game list from GitHub mirror.');
        }
    }

    async function fetchGameList() { 
        allFetchDone.value = false;
        addLog('Fetching game list...');
        // Bundled list first: it is local, so the UI is usable in under a second.
        // The GitHub mirror then refreshes it in the background.
        try {
            await executeBundled();
        } catch {
            addLog('error', 'Error executing fetch for bundled game list.');
        }

        if (!errorBundled.value && bundledGameList.value && bundledGameList.value?.length > 0 && isValidGameList(bundledGameList.value)) {
            useList('bundled game list', bundledGameList.value);
            markDone();
            refreshFromMirror();
            return;
        }

        // silently log error for bundled, as it's the last resort.
        if (errorBundled.value) {
            addLog('error','Error fetching bundled game list');
        }

        // No usable bundled list — fetch the mirror (15s timeout on the Rust side).
        try {
            await executeGH();
        } catch {
            addLog('error', 'Error executing fetch for GitHub mirror.');
        }

        if (!errorGH.value && gameListGHMirror.value && gameListGHMirror.value?.length > 0 && isValidGameList(gameListGHMirror.value)) {
            useList('GitHub mirror', gameListGHMirror.value);
            markDone();
            return;
        }

        if (errorGH.value) { 
            fetchError.value = 'Error fetching game list from GitHub mirror.';
            addLog('error','Error fetching game list from GitHub mirror');
        }

        // Last resort: Discord directly.
        try {
            await executeDiscord();
        } catch {
            addLog('error', 'Error executing fetch for Discord.');
        }

        if (!errorDiscord.value && gameListFromDiscord.value && gameListFromDiscord.value?.length > 0 && isValidGameList(gameListFromDiscord.value)) {
            useList('Discord', gameListFromDiscord.value);
        } else {
            fetchError.value = 'Error fetching game list from Discord.';
            addLog('error','Error fetching game list from Discord:');
        }

        if (fetchError.value) {
            await message('There was an error fetching the latest game list.' + fetchError.value, {
                title: 'Game List Fetch Error',
                kind: 'error',
                buttons: {
                    ok: 'OK'
                }
            });
        }

        markDone();
    }

    watch(allFetchDone, (newVal) => {
        if (newVal && timeoutId) {
            clearTimeout(timeoutId);
        }
    });

    tryOnMounted(async () => {
        await fetchGameList();
    });


    return {
        gameListGHMirror,
        gameListFromDiscord,
        bundledGameList,
        fetchError,
        isReadyGH,
        isReadyDiscord,
        isReadyBundled,
        gameDB,
        fetchGameList,
        isLoadingGH,
        isLoadingDiscord,
        isLoadingBundled,
        allFetchDone
    }
}
