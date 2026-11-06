<template>
  <div class="space-y-3 text-xs text-zinc-400">
    <div v-if="isMac" class="p-2.5 rounded-lg bg-amber-500/10 border border-amber-500/20 text-amber-300">
      A small window opens while playing so Discord on macOS can detect the game.
    </div>

    <div v-if="filteredExecutables.length === 0" class="p-3.5 rounded-lg bg-[#161822] border border-[#272A3B] text-zinc-300 space-y-1">
      <div class="flex items-center gap-2 font-medium text-xs text-white">
        <span class="w-2 h-2 rounded-full bg-indigo-400"></span>
        <span>Direct Discord RPC Mode</span>
      </div>
      <p class="text-[11px] text-zinc-400 leading-relaxed">
        Discord tracks this title via direct Rich Presence rather than a local .exe process. Click <strong>Start Quest</strong> to broadcast quest activity directly to Discord.
      </p>
    </div>

    <!-- Executables List -->
    <div v-else class="space-y-2">
      <div
        v-for="executable in filteredExecutables"
        :key="executable.name"
        class="flex items-center justify-between gap-3 p-2.5 rounded-lg border transition-colors"
        :class="gameActions?.isExecutableRunning(executable)
          ? 'bg-[#15231D] border-emerald-500/40'
          : 'bg-[#181920] border-[#252733] hover:border-[#333646]'"
      >
        <div class="min-w-0 flex-1">
          <div class="flex items-center gap-2">
            <span class="text-[10px] font-mono uppercase px-1.5 py-0.5 rounded bg-[#222430] text-zinc-400">
              {{ executable.os === 'win32' ? 'Win' : executable.os === 'darwin' ? 'Mac' : 'Linux' }}
            </span>
            <span class="font-medium text-white truncate" :title="executable.name">
              {{ getFilename(executable) }}
            </span>
          </div>
          <div class="text-[11px] text-zinc-500 font-mono truncate mt-0.5">
            {{ executable.name }}
          </div>
        </div>

        <button
          class="shrink-0 px-3 py-1.5 rounded-md text-xs font-medium transition-colors cursor-pointer"
          :class="gameActions?.isExecutableRunning(executable)
            ? 'bg-rose-600 hover:bg-rose-700 text-white'
            : 'bg-[#E12D46] hover:bg-[#cf223a] text-white'"
          @click="handleLaunch(executable)"
        >
          {{ gameActions?.isExecutableRunning(executable) ? 'Stop' : 'Play' }}
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { GameActionsKey, getCurrentOS, isMacOS } from '@/constants/constants';
import { GameActionsProvider, type Game, type GameExecutable } from '@/types/types';
import { path } from '@tauri-apps/api';
import { computed, inject } from 'vue';

const props = defineProps<{
  game: Game;
}>();

const emit = defineEmits<{
  play: [{ game: Game; executable: GameExecutable }];
  stop: [{ game: Game; executable: GameExecutable }];
  install_and_play: [{ game: Game; executable: GameExecutable }];
}>();

const gameActions = inject<GameActionsProvider>(GameActionsKey);

function isValidPath(name: string) {
  const illegalChars = ['>', '<', ':', '"', '|', '?', '*'];
  return !illegalChars.some(char => name.includes(char));
}

const validExecutables = computed(() =>
  props.game.executables.filter(executable => isValidPath(executable.name))
);

const currentPlatform = getCurrentOS();
const isMac = isMacOS();

const filteredExecutables = computed(() => {
  const platformMatches = validExecutables.value.filter(
    executable => executable.os === currentPlatform
  );

  if (platformMatches.length > 0) {
    return platformMatches;
  }

  return validExecutables.value;
});

function splitExecutableName(executable: GameExecutable) {
  const allSections = executable.name.split(/\\|\//);
  const last = executable.name.split(/\\|\//).pop();
  const name = last?.split('.').slice(0, -1).join('.') || last;
  return [...allSections.slice(0, -1), name];
}

function getExecutablePath(executable: GameExecutable) {
  const allSections = executable.name.split(/\\|\//);
  return allSections.slice(0, -1).join(path.sep());
}

function getFilename(executable: GameExecutable) {
  return executable.name.split(/\\|\//).pop() || executable.name;
}

function handleLaunch(executable: GameExecutable) {
  if (executable.is_running) {
    emit('stop', {
      game: props.game,
      executable: {
        path: getExecutablePath(executable),
        segments: splitExecutableName(executable).length,
        filename: getFilename(executable),
        ...executable,
      },
    });
  } else {
    if (!gameActions?.isGameExecutableInstalled(executable)) {
      emit('install_and_play', {
        game: props.game,
        executable: {
          path: getExecutablePath(executable),
          segments: splitExecutableName(executable).length,
          filename: getFilename(executable),
          ...executable,
        },
      });
    } else {
      emit('play', {
        game: props.game,
        executable: {
          path: getExecutablePath(executable),
          segments: splitExecutableName(executable).length,
          filename: getFilename(executable),
          ...executable,
        },
      });
    }
  }
}
</script>
