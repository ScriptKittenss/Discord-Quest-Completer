<script setup lang="ts">
import { Pages, useGlobalState } from '@/composables/app-state';
import { onMounted, onUnmounted, ref } from 'vue';

const appState = useGlobalState();
const { page, setPage, activeQuest } = appState;

const elapsedSeconds = ref(0);
let timerInterval: any = null;

function updateTimer() {
  if (activeQuest.value?.startedAt) {
    const diff = Math.floor((Date.now() - new Date(activeQuest.value.startedAt).getTime()) / 1000);
    elapsedSeconds.value = Math.max(0, diff);
  } else {
    elapsedSeconds.value = 0;
  }
}

function formatElapsed(sec: number) {
  const m = Math.floor(sec / 60);
  const s = sec % 60;
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

onMounted(() => {
  timerInterval = setInterval(updateTimer, 1000);
});

onUnmounted(() => {
  if (timerInterval) clearInterval(timerInterval);
});
</script>

<template>
  <div class="flex flex-col h-dvh overflow-hidden bg-[#0F1014] text-[#ECEEF2]" style="font-family: Arial, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;">
    <!-- Compact Top Bar -->
    <header class="shrink-0 border-b border-[#22242D] bg-[#14151C] px-4 py-2">
      <div class="flex items-center justify-between gap-4">
        <!-- Brand -->
        <div class="flex items-center gap-2.5 cursor-pointer" @click="setPage(Pages.HOME)">
          <img
            src="/logo.png"
            alt="Script Kittens"
            class="h-7 w-7 rounded-md"
          />
          <div class="flex items-baseline gap-1.5">
            <span class="font-bold text-sm tracking-tight text-white">
              Script Kittens
            </span>
            <span class="text-[11px] text-zinc-400 font-normal">
              Quest Complete
            </span>
          </div>
        </div>

        <!-- Center Quest Live Status -->
        <div v-if="activeQuest" class="flex items-center gap-2 text-xs text-zinc-300 bg-[#1C1E26] border border-[#2B2E3A] px-2.5 py-0.5 rounded-md">
          <span class="w-2 h-2 rounded-full bg-emerald-500"></span>
          <span>Playing: <strong class="text-white">{{ activeQuest.name }}</strong></span>
          <span class="text-zinc-400 font-mono">({{ formatElapsed(elapsedSeconds) }})</span>
        </div>

        <!-- Right: Tabs & Socials -->
        <div class="flex items-center gap-2.5">
          <nav class="flex items-center bg-[#1A1C23] border border-[#272A35] rounded-md p-0.5">
            <button
              class="px-2.5 py-1 rounded text-xs font-medium transition-colors cursor-pointer"
              :class="page === Pages.HOME
                ? 'bg-[#E12D46] text-white'
                : 'text-zinc-400 hover:text-white'"
              @click.prevent="setPage(Pages.HOME)"
            >
              Quests
            </button>
            <button
              class="px-2.5 py-1 rounded text-xs font-medium transition-colors cursor-pointer"
              :class="page === Pages.PLAYGROUND
                ? 'bg-[#E12D46] text-white'
                : 'text-zinc-400 hover:text-white'"
              @click.prevent="setPage(Pages.PLAYGROUND)"
            >
              Console
            </button>
          </nav>

          <div class="flex items-center gap-1 pl-1.5 border-l border-[#272A35]">
            <a
              href="https://discord.gg/2v9F7bp94F"
              target="_blank"
              rel="noopener"
              class="p-1 rounded text-zinc-400 hover:text-white hover:bg-[#1E202A] transition-colors"
              title="Discord Server"
            >
              <svg class="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 24 24">
                <path d="M20.317 4.37a19.791 19.791 0 0 0-4.885-1.515.074.074 0 0 0-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 0 0-5.487 0 12.64 12.64 0 0 0-.617-1.25.077.077 0 0 0-.079-.037A19.736 19.736 0 0 0 3.677 4.37a.07.07 0 0 0-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 0 0 .031.057 19.9 19.9 0 0 0 5.993 3.03.078.078 0 0 0 .084-.028c.462-.63.874-1.295 1.226-1.994.021-.041.001-.09-.041-.106a13.107 13.107 0 0 1-1.872-.892.077.077 0 0 1-.008-.128 10.2 10.2 0 0 0 .372-.292.074.074 0 0 1 .077-.01c3.929 1.793 8.18 1.793 12.061 0a.074.074 0 0 1 .078.01c.12.098.246.198.373.292a.077.077 0 0 1-.006.127 12.299 12.299 0 0 1-1.873.894.077.077 0 0 0-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 0 0 .084.028 19.839 19.839 0 0 0 6.002-3.03.077.077 0 0 0 .032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 0 0-.031-.028zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z" />
              </svg>
            </a>
            <a
              href="https://www.youtube.com/@1shot1moanz"
              target="_blank"
              rel="noopener"
              class="p-1 rounded text-zinc-400 hover:text-white hover:bg-[#1E202A] transition-colors"
              title="YouTube"
            >
              <svg class="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 24 24">
                <path d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z" />
              </svg>
            </a>
          </div>
        </div>
      </div>
    </header>

    <!-- Main Workspace (Zero visible scrollbars) -->
    <main class="flex-grow overflow-y-auto" style="scrollbar-width: none; -ms-overflow-style: none;">
      <slot></slot>
    </main>

    <!-- Bottom Bar -->
    <footer class="shrink-0 border-t border-[#22242D] bg-[#14151C] px-4 py-1.5 flex items-center justify-between text-[11px] text-zinc-500">
      <div>Script Kittens Quest Complete</div>
      <div class="flex items-center gap-3 text-zinc-400">
        <a href="https://github.com/ScriptKittenss/Discord-Quest-Completer" target="_blank" rel="noopener" class="hover:text-zinc-200">GitHub</a>
        <span>•</span>
        <a href="https://discord.gg/2v9F7bp94F" target="_blank" rel="noopener" class="hover:text-zinc-200">Discord</a>
      </div>
    </footer>
  </div>
</template>

<style scoped>
main::-webkit-scrollbar {
  display: none !important;
  width: 0 !important;
  height: 0 !important;
}
</style>
