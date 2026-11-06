<script setup lang="ts">
import { ref, computed, useTemplateRef, shallowRef, provide } from 'vue';
import { onClickOutside, refDebounced } from '@vueuse/core';
import { useFuse } from '@vueuse/integrations/useFuse';
import { invoke } from '@tauri-apps/api/core';
import { randomString } from '@/utils/random-string';
import { GameActionsProvider, GameExecutable, type Game } from '@/types/types';
import IconVerified from '@/components/IconVerified.vue';
import { isEmpty } from 'lodash-es';
import GameExecutables from '@/components/GameExecutables.vue';
import { GameActionsKey } from '@/constants/constants';
import { path } from '@tauri-apps/api';
import { emit } from '@tauri-apps/api/event';
import { useFetchGameList } from '@/composables/fetch-gamelist';
import { UseFuseOptions } from '@vueuse/integrations';
import Fuse from 'fuse.js';
import { useGlobalState } from '@/composables/app-state';
import { enrichGameExecutables } from '@/utils/game-overrides';

type DialogKey =
  | 'none'
  | 'rpc_message_1'
  | 'no_game_selected';

const {
  gameDB,
  fetchGameList,
} = useFetchGameList();

const { addLog, setActiveQuest, activeQuest } = useGlobalState();

const dialogRef = useTemplateRef<HTMLDialogElement>('dialogRef');
const searchResultContainerRef = useTemplateRef<HTMLElement>('searchResultContainerRef');
const dialogMessage = ref('');
const isDialogOpen = ref(false);
const dialogKey = ref<DialogKey>('none');
const isConnectedToRPC = ref(false);
const isConnecting = ref(false);
const copiedAppId = ref(false);

// Search functionality
const searchQuery = shallowRef('');
const debouncedSearchQuery = refDebounced(searchQuery, 150);
const searchResultsIsOpen = ref(false);
const isOnSearchResults = ref(false);

const currentlyPlaying = ref<string | null>(null);

onClickOutside(searchResultContainerRef, () => {
  searchResultsIsOpen.value = false;
});

const COPYRIGHT_SYMBOL = '\u00A9';
const TRADEMARK_SYMBOL = '\u2122';
const REGISTERED_SYMBOL = '\u00AE';
const ignoredSymbols = [COPYRIGHT_SYMBOL, TRADEMARK_SYMBOL, REGISTERED_SYMBOL];
const ignoredSymbolsRegex = new RegExp(`[${ignoredSymbols.join('')}]`, 'g');

const fuseOptions = computed<UseFuseOptions<Game>>(() => ({
  fuseOptions: {
    keys: [
      { name: 'name', weight: 0.7 },
      { name: 'aliases', weight: 0.2 },
      { name: 'executables.name', weight: 0.1 },
    ],
    getFn: (obj: any, pathStr: string[] | string) => {
      const value = Fuse.config.getFn(obj, pathStr);
      return typeof value === 'string'
        ? value.replace(ignoredSymbolsRegex, '')
        : value;
    },
    isCaseSensitive: false,
    threshold: 0.45,
    includeScore: true,
    includeMatches: false,
  },
  resultLimit: 12,
  matchAllWhenSearchEmpty: false,
}));

const { results: searchResults } = useFuse(debouncedSearchQuery, gameDB, fuseOptions);

// Selected games list
const gameList = ref<Game[]>([]);
const selectedGameId = ref<string | null | undefined>(null);

const selectedGame = computed(() => {
  if (!selectedGameId.value) return null;
  const found = gameList.value.find(g => g.uid === selectedGameId.value);
  if (found && (!found.executables || found.executables.length === 0)) {
    const enriched = enrichGameExecutables(found);
    found.executables = enriched.executables;
  }
  return found || null;
});

function closeSearchResults() {
  searchResultsIsOpen.value = false;
}
function openSearchResults() {
  searchResultsIsOpen.value = true;
}

function clearSearch() {
  searchQuery.value = '';
  searchResultsIsOpen.value = false;
}

function copyId(id: string) {
  navigator.clipboard.writeText(id);
  copiedAppId.value = true;
  setTimeout(() => {
    copiedAppId.value = false;
  }, 1500);
}

function addGameToList(game: Game) {
  const existing = gameList.value.find(g => g.id === game.id);
  const enriched = enrichGameExecutables(game);
  if (!existing) {
    const newGame = {
      uid: randomString(),
      ...enriched,
    };
    gameList.value.push(newGame);
    selectedGameId.value = newGame.uid;
    addLog('info', `Added ${game.name} to quest list.`);
  } else {
    existing.executables = enriched.executables;
    selectedGameId.value = existing.uid;
  }
  closeSearchResults();
  searchQuery.value = '';
}

const forceRerenderKey = ref(0);

function removeGameFromList(game: Game) {
  const gameId = game.uid;
  gameList.value = gameList.value.filter(g => g.uid !== gameId);
  if (selectedGame.value?.uid === gameId) {
    selectedGameId.value = gameList.value[0]?.uid || null;
    forceRerenderKey.value++;
  }
  addLog('info', `Removed ${game.name}.`);
}

function selectGame(game: Game) {
  if (game && (!game.executables || game.executables.length === 0)) {
    const enriched = enrichGameExecutables(game);
    game.executables = enriched.executables;
  }
  selectedGameId.value = game?.uid;
  searchResultsIsOpen.value = false;
}

function canCreateDummyGame(game: Game | null) {
  if (!game) return false;
  return !game.is_installed;
}

function canPlayGame(game: Game | null) {
  if (!game) return false;
  return (game.is_installed && !game.is_running) ?? false;
}

function isExecutableRunning(executable: GameExecutable) {
  return executable.is_running ?? false;
}

function isGameExecutableInstalled(executable: GameExecutable) {
  return executable.is_installed ?? false;
}

function isGameInstalled(game: Game | null) {
  if (!game) return false;
  return game.is_installed ?? false;
}

async function createDummyGame(game: Game | null, executable: GameExecutable) {
  if (!game) return;
  const gameUid = game.uid;
  const gameToInstall = gameList.value.find(g => g.uid === gameUid);
  const executableItem = gameToInstall?.executables.find(exe => exe.name === executable.name);
  if (gameToInstall && executableItem) {
    const steamSku = (gameToInstall as any).third_party_skus?.find((s: any) => s.distributor === 'steam');
    const steamAppId = steamSku?.id ? String(steamSku.id) : (gameToInstall.id === '1445982809792708628' ? '3393110' : '');

    const payload = {
      path: executable.path,
      executable_name: executable.filename,
      path_len: executable.segments,
      app_id: Number(gameToInstall.id),
      display_name: gameToInstall.name,
      steam_app_id: steamAppId,
    };
    await invoke('create_fake_game', payload);
    gameToInstall.is_installed = true;
    executableItem.is_installed = true;
    return true;
  }
}

async function installAndPlay({ game, executable }: { game: Game; executable: GameExecutable }) {
  if (!game) return;
  const gameCreated = await createDummyGame(game, executable);
  if (gameCreated) {
    playGame({ game, executable });
  } else {
    addLog('error', `Failed to create runner for ${game.name}.`);
  }
}

async function playGame({ game, executable }: { game: Game; executable: GameExecutable }) {
  if (!game) return;
  const gameUid = game.uid;
  try {
    addLog('info', `Starting quest simulation for ${game.name}...`);
    currentlyPlaying.value = game.id;
    const gameToPlay = gameList.value.find(g => g.uid === gameUid);
    const executableItem = gameToPlay?.executables.find(exe => exe.name === executable.name);
    if (gameToPlay && executableItem) {
      const payload = {
        name: game.name,
        path: executable.path,
        executable_name: executable.filename,
        path_len: executable.segments,
        app_id: Number(gameToPlay.id),
        exec_path: path.join(executable.path!, executable.filename!),
      };
      await invoke('run_background_process', payload);
      gameToPlay.is_running = true;
      executableItem.is_running = true;
      setActiveQuest({ id: game.id, name: game.name, executableName: executable.filename });
      addLog('info', `Running ${executable.filename}. Discord is detecting the game.`);
    }
  } catch (error) {
    console.error('Launch failed:', error);
    addLog('error', `Failed to start: ${error}`);
  }
}

async function stopPlaying({ game, executable }: { game: Game; executable: GameExecutable }) {
  if (!game) return;
  const gameUid = game.uid;
  currentlyPlaying.value = null;
  setActiveQuest(null);

  const gameToPlay = gameList.value.find(g => g.uid === gameUid);
  const executableItem = gameToPlay?.executables.find(exe => exe.name === executable.name);
  if (gameToPlay && executableItem) {
    try {
      await invoke('stop_process', {
        exec_name: executable.filename!,
      });
      addLog('info', `Stopped ${game.name}. Process terminated.`);
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      addLog('error', `Error stopping process: ${errorMessage}`);
    } finally {
      gameToPlay.is_running = false;
      executableItem.is_running = false;
    }
  }
}

function prepareExecutable(executable: GameExecutable): GameExecutable {
  const allSections = executable.name.split(/\\|\//);
  const filename = allSections.pop() || executable.name;
  const pathPart = allSections.join(path.sep());
  return {
    ...executable,
    path: pathPart,
    filename: filename,
    segments: allSections.length + 1,
  };
}

// Quick trigger for first executable or direct RPC emulation
function toggleQuest(game: Game) {
  if (!game) return;
  if (game.executables && game.executables.length > 0) {
    const targetExe = prepareExecutable(game.executables[0]);
    if (game.is_running) {
      stopPlaying({ game, executable: targetExe });
    } else {
      installAndPlay({ game, executable: targetExe });
    }
  } else {
    // For games with no local executables in Discord registry (e.g. AION 2), toggle Discord RPC directly
    if (game.is_running || isConnectedToRPC.value) {
      handleTestRPC(game);
    } else {
      continueRPCRisk(game);
    }
  }
}

async function handleTestRPC(game: Game | null) {
  const state = isConnectedToRPC.value ? 'disconnect' : 'connect';

  if (!game && state === 'connect') {
    showDialog('no_game_selected');
    return;
  }
  if (state === 'disconnect' || isConnecting.value) {
    emit('event_disconnect');
    isConnectedToRPC.value = false;
    if (game) game.is_running = false;
    currentlyPlaying.value = null;
    isConnecting.value = false;
    setActiveQuest(null);
    addLog('info', 'Disconnected from Discord RPC.');
    return;
  }
  showDialog('rpc_message_1');
}

async function continueRPCRisk(game: Game | null) {
  if (!game) return;
  const gameUid = game.uid;
  const gameToTest = gameList.value.find(g => g.uid === gameUid);
  if (gameToTest) {
    isConnecting.value = true;
    invoke('connect_to_discord_rpc_3', {
      activity_json: JSON.stringify({
        app_id: gameToTest.id,
      }),
      action: 'connect',
    }).then(() => {
      isConnectedToRPC.value = true;
      gameToTest.is_running = true;
      currentlyPlaying.value = gameToTest.id;
      isConnecting.value = false;
      setActiveQuest({ id: gameToTest.id, name: gameToTest.name });
      addLog('info', `Broadcasting custom RPC for ${gameToTest.name}.`);
    });
    hideDialog();
  }
}

function handleSearchBlur() {
  setTimeout(() => {
    if (!isOnSearchResults.value) {
      searchResultsIsOpen.value = false;
    }
  }, 200);
}

function showDialog(message: DialogKey) {
  isDialogOpen.value = true;
  dialogMessage.value = message;
  dialogKey.value = message;
  if (!isEmpty(message)) {
    dialogRef.value?.showModal();
  }
}

function hideDialog() {
  dialogRef.value?.close();
  dialogMessage.value = '';
  isDialogOpen.value = false;
}

provide<GameActionsProvider>(GameActionsKey, {
  canPlayGame,
  isGameInstalled,
  isExecutableRunning,
  isGameExecutableInstalled,
});
</script>

<template>
  <div class="h-full flex overflow-hidden bg-[#0F1014] text-[#ECEEF2]">
    <!-- Dialog -->
    <dialog
      id="dialog"
      class="fixed inset-0 bg-[#161822] border border-[#2B2E3D] rounded-xl text-zinc-200 z-50 max-w-md w-[calc(100%-2rem)] p-5 shadow-2xl backdrop:bg-black/60"
      style="left: 50%; top: 50%; transform: translate(-50%, -50%);"
      ref="dialogRef"
    >
      <div v-if="dialogKey === 'rpc_message_1'">
        <h3 class="font-bold text-white text-base mb-2">Notice</h3>
        <p class="text-xs text-zinc-300 leading-relaxed">
          This sends an activity payload to Discord directly using the game's registered Application ID.
        </p>
        <p class="text-xs text-amber-400 mt-2">
          Tip: For safest quest progression, the standard <strong>Play</strong> button is recommended because it lets Discord detect the process naturally.
        </p>
      </div>

      <div v-if="dialogKey === 'no_game_selected'">
        <h3 class="font-bold text-white text-base mb-1">No Game Selected</h3>
        <p class="text-xs text-zinc-400">Please select a game first.</p>
      </div>

      <div class="flex gap-2 justify-end mt-4">
        <button
          class="px-3.5 py-1.5 rounded-lg text-xs font-medium border border-[#323647] bg-[#1E202A] text-zinc-300 hover:text-white cursor-pointer"
          @click="hideDialog()"
        >
          Cancel
        </button>
        <button
          v-if="dialogKey === 'rpc_message_1'"
          class="px-3.5 py-1.5 rounded-lg text-xs font-medium bg-[#E12D46] hover:bg-[#cf223a] text-white cursor-pointer"
          @click="continueRPCRisk(selectedGame)"
        >
          Continue
        </button>
      </div>
    </dialog>

    <!-- LEFT SIDEBAR: Library & Search -->
    <aside class="w-72 shrink-0 border-r border-[#22242D] bg-[#14151C] flex flex-col h-full overflow-hidden">
      <!-- Search Input in Sidebar -->
      <div class="p-3 border-b border-[#22242D] relative" ref="searchResultContainerRef">
        <div class="relative">
          <svg class="absolute left-2.5 top-1/2 -translate-y-1/2 h-3.5 w-3.5 text-zinc-500" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
            <path stroke-linecap="round" stroke-linejoin="round" d="M21 21l-4.35-4.35M17 10.5a6.5 6.5 0 11-13 0 6.5 6.5 0 0113 0z" />
          </svg>
          <input
            v-model="searchQuery"
            type="text"
            placeholder="Search Discord games..."
            class="w-full pl-8 pr-7 py-1.5 bg-[#1C1E26] border border-[#2A2C38] rounded-md text-white placeholder-zinc-500 text-xs focus:outline-none focus:border-[#E12D46] transition-colors"
            @focus="openSearchResults"
            @blur="handleSearchBlur"
          />
          <button
            v-if="searchQuery"
            @click="clearSearch"
            class="absolute right-2 top-1/2 -translate-y-1/2 text-zinc-500 hover:text-white text-xs cursor-pointer"
          >
            ✕
          </button>
        </div>

        <!-- Floating Search Results Dropdown -->
        <div
          v-if="searchResultsIsOpen && searchResults.length > 0"
          @click="isOnSearchResults = true"
          class="absolute left-3 right-3 top-12 z-50 bg-[#1A1C24] border border-[#2D303E] rounded-lg shadow-xl max-h-72 overflow-y-auto"
          style="scrollbar-width: none; -ms-overflow-style: none;"
        >
          <div
            v-for="game in searchResults"
            :key="game.item.id"
            class="p-2.5 hover:bg-[#222530] border-b border-[#242632] last:border-b-0 transition-colors flex items-center justify-between gap-2"
          >
            <div class="min-w-0 flex-1">
              <div class="font-medium text-white text-xs truncate">{{ game.item.name }}</div>
              <div class="text-[10px] text-zinc-500 font-mono">ID: {{ game.item.id }}</div>
            </div>
            <button
              @click="addGameToList(game.item)"
              class="px-2 py-0.5 text-[11px] font-medium bg-[#E12D46] hover:bg-[#cf223a] text-white rounded cursor-pointer"
            >
              + Add
            </button>
          </div>
        </div>
      </div>

      <!-- Games List Header -->
      <div class="px-3 pt-3 pb-1.5 flex items-center justify-between text-xs text-zinc-500 font-medium">
        <span>MY QUESTS ({{ gameList.length }})</span>
        <button
          @click="fetchGameList()"
          class="text-[11px] text-zinc-400 hover:text-white cursor-pointer"
          title="Refresh Game Registry"
        >
          Refresh
        </button>
      </div>

      <!-- Games Scrollable Rail -->
      <div class="flex-grow overflow-y-auto p-2 space-y-1" style="scrollbar-width: none; -ms-overflow-style: none;">
        <div v-if="gameList.length === 0" class="py-10 text-center px-4 text-xs text-zinc-500 leading-relaxed">
          No games in your list.<br/>Type in the search box above to add any game for quests.
        </div>

        <div
          v-for="game in gameList"
          :key="game.uid"
          class="group p-2 rounded-md transition-colors cursor-pointer flex items-center justify-between gap-2 border"
          :class="selectedGame?.uid === game.uid
            ? 'bg-[#1F191D] border-[#E12D46]/60 text-white'
            : 'bg-transparent border-transparent hover:bg-[#1A1C24] text-zinc-300'"
          @click="selectGame(game)"
        >
          <div class="flex items-center gap-2 min-w-0 flex-1">
            <span
              class="w-6 h-6 rounded flex items-center justify-center font-bold text-[10px] shrink-0"
              :class="game.is_running ? 'bg-emerald-500/20 text-emerald-400' : 'bg-[#222430] text-zinc-400'"
            >
              {{ game.name.substring(0, 2).toUpperCase() }}
            </span>
            <div class="min-w-0 flex-1">
              <div class="font-medium text-xs truncate">{{ game.name }}</div>
              <div class="text-[10px] font-mono text-zinc-500 truncate flex items-center gap-1">
                <span v-if="game.is_running" class="text-emerald-400 font-medium">Playing</span>
                <span v-else>Ready</span>
              </div>
            </div>
          </div>

          <button
            @click.stop="removeGameFromList(game)"
            class="opacity-0 group-hover:opacity-100 text-zinc-500 hover:text-zinc-200 text-xs px-1 cursor-pointer transition-opacity"
            title="Remove"
            v-if="!game.is_running"
          >
            ✕
          </button>
        </div>
      </div>
    </aside>

    <!-- RIGHT MAIN STAGE: Active Game Dashboard -->
    <main class="flex-1 flex flex-col h-full overflow-hidden bg-[#0F1014] p-6" :key="forceRerenderKey">
      <div v-if="!selectedGame" class="m-auto text-center max-w-sm space-y-2">
        <h2 class="text-sm font-bold text-white">No Game Selected</h2>
        <p class="text-xs text-zinc-500 leading-relaxed">
          Select a game from the left sidebar to start quest emulation, or use the search box to find your Discord quest game.
        </p>
      </div>

      <div v-else class="max-w-2xl w-full mx-auto space-y-5 my-auto">
        <!-- Game Title & Details -->
        <div class="flex items-start justify-between gap-4 pb-4 border-b border-[#22242D]">
          <div>
            <div class="flex items-center gap-2">
              <h1 class="text-2xl font-extrabold text-white tracking-tight">{{ selectedGame.name }}</h1>
              <IconVerified class="w-4 h-4 text-zinc-400 shrink-0" />
            </div>
            <div class="flex items-center gap-2 mt-1 text-xs text-zinc-400 font-mono">
              <span>Application ID: {{ selectedGame.id }}</span>
              <button
                @click="copyId(selectedGame.id)"
                class="text-[#E12D46] hover:underline cursor-pointer"
              >
                {{ copiedAppId ? 'Copied!' : 'Copy' }}
              </button>
            </div>
          </div>

          <!-- Big Action Button -->
          <div class="shrink-0">
            <button
              class="px-6 py-2.5 rounded-lg text-xs font-bold tracking-wide transition-colors cursor-pointer flex items-center gap-2"
              :class="selectedGame.is_running
                ? 'bg-rose-600 hover:bg-rose-700 text-white'
                : 'bg-[#E12D46] hover:bg-[#cf223a] text-white shadow-sm'"
              @click="toggleQuest(selectedGame)"
            >
              <span v-if="selectedGame.is_running">■ STOP QUEST</span>
              <span v-else>▶ START QUEST</span>
            </button>
          </div>
        </div>

        <!-- Status Card -->
        <div class="p-3.5 rounded-lg border bg-[#14151C]" :class="selectedGame.is_running ? 'border-emerald-500/40 bg-[#121E19]' : 'border-[#22242D]'">
          <div class="flex items-center justify-between text-xs">
            <div class="flex items-center gap-2">
              <span class="w-2 h-2 rounded-full" :class="selectedGame.is_running ? 'bg-emerald-400 animate-pulse' : 'bg-zinc-600'"></span>
              <span v-if="selectedGame.is_running" class="font-medium text-emerald-300">
                Quest is actively progressing on Discord
              </span>
              <span v-else class="text-zinc-400">
                Ready to play. Click <strong>Start Quest</strong> to progress this game's quest on Discord.
              </span>
            </div>
            <span v-if="selectedGame.is_running" class="text-emerald-400 font-mono font-medium text-xs">
              {{ isConnectedToRPC ? 'Discord RPC Active' : 'Simulating Process' }}
            </span>
          </div>
        </div>

        <!-- Executables Options -->
        <div class="space-y-2">
          <div class="text-xs font-semibold text-zinc-400 uppercase tracking-wider">
            {{ selectedGame.executables?.length ? 'Game Executables' : 'Quest Emulation Mode' }}
          </div>
          <GameExecutables
            :game="selectedGame"
            @play="playGame"
            @stop="stopPlaying"
            @install_and_play="installAndPlay"
          />
        </div>

        <!-- Discord RPC Alternative Test Option -->
        <div class="pt-2 flex items-center justify-between text-xs text-zinc-500">
          <span>Need custom activity testing?</span>
          <button
            @click="handleTestRPC(selectedGame)"
            class="text-zinc-400 hover:text-white underline cursor-pointer"
          >
            {{ isConnectedToRPC ? 'Disconnect Custom RPC' : 'Test Custom Discord RPC' }}
          </button>
        </div>
      </div>
    </main>
  </div>
</template>

<style scoped></style>
