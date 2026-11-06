<template>
  <div class="h-full flex flex-col p-6 max-w-4xl mx-auto w-full space-y-4 overflow-hidden">
    <!-- Header -->
    <div class="flex items-center justify-between pb-3 border-b border-[#22242D] shrink-0">
      <div>
        <h1 class="text-lg font-bold text-white">Developer Console</h1>
        <p class="text-xs text-zinc-400 mt-0.5">Real-time process daemon logs and Discord RPC debugger.</p>
      </div>

      <div class="flex items-center gap-2">
        <button
          class="px-3 py-1.5 rounded text-xs font-medium border border-[#2B2E3D] bg-[#161822] text-zinc-300 hover:text-white cursor-pointer transition-colors"
          @click="discordTest"
        >
          {{ isConnected ? 'Disconnect Test RPC' : 'Test Discord RPC' }}
        </button>
      </div>
    </div>

    <!-- Live Terminal Log Stream -->
    <div class="flex-1 flex flex-col min-h-0 bg-[#13141B] border border-[#22242D] rounded-xl overflow-hidden">
      <!-- Terminal Top bar -->
      <div class="px-4 py-2 border-b border-[#22242D] bg-[#171922] flex items-center justify-between text-xs text-zinc-400 shrink-0">
        <span class="font-mono text-zinc-300">engine.log ({{ logs.length }} events)</span>

        <div class="flex items-center gap-2">
          <button
            class="text-[11px] text-zinc-400 hover:text-white cursor-pointer"
            @click="copyAllLogs"
          >
            {{ copiedLogs ? 'Copied' : 'Copy' }}
          </button>
          <span>•</span>
          <button
            class="text-[11px] text-zinc-400 hover:text-white cursor-pointer"
            @click="clearLogs"
          >
            Clear
          </button>
        </div>
      </div>

      <!-- Logs output (No visible scrollbar) -->
      <div class="flex-1 p-3.5 font-mono text-xs leading-relaxed overflow-y-auto space-y-1.5 bg-[#0D0E13]" style="scrollbar-width: none; -ms-overflow-style: none;">
        <div v-if="logs.length === 0" class="py-12 text-center text-zinc-600">
          No logs yet. Actions taken in the quest tab will show up here.
        </div>

        <div
          v-for="(log, index) in logs"
          :key="index"
          class="flex items-start gap-2 hover:bg-white/[0.02] px-1 py-0.5 rounded"
        >
          <span class="text-zinc-600 shrink-0 select-none">[{{ formatLogTime(log.timestamp) }}]</span>
          <span
            class="shrink-0 uppercase font-semibold text-[10px]"
            :class="{
              'text-sky-400': log.type === 'info',
              'text-rose-400': log.type === 'error',
              'text-amber-400': log.type === 'warning',
              'text-emerald-400': log.type === 'debug'
            }"
          >
            {{ log.type }}
          </span>
          <span class="text-zinc-300 break-all">{{ log.message }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { invoke } from '@tauri-apps/api/core';
import { emit } from '@tauri-apps/api/event';
import { useGlobalState } from '@/composables/app-state';

const ActivityKind = {
  Playing: 0,
  Listening: 2,
  Watching: 3,
  Competing: 5,
} as const;

const isConnected = ref(false);
const testAppId = ref('1361728268088381706');
const copiedLogs = ref(false);

const { logs, addLog, clearLogs } = useGlobalState();

function formatLogTime(date: Date) {
  const d = new Date(date);
  return d.toTimeString().split(' ')[0];
}

function copyAllLogs() {
  const text = logs.value
    .map(l => `[${formatLogTime(l.timestamp)}] [${l.type.toUpperCase()}] ${l.message}`)
    .join('\n');
  navigator.clipboard.writeText(text);
  copiedLogs.value = true;
  setTimeout(() => {
    copiedLogs.value = false;
  }, 1500);
}

function discordTest() {
  const appIdCode = testAppId.value || '1361728268088381706';

  if (isConnected.value) {
    emit('event_disconnect');
    isConnected.value = false;
    addLog('info', 'Disconnected test RPC session.');
    return;
  }

  invoke('connect_to_discord_rpc_3', {
    activity_json: JSON.stringify({
      app_id: appIdCode,
      details: 'Script Kittens Quest Runner',
      state: 'Simulating Quest',
      activity_kind: ActivityKind.Playing,
      timestamp: Math.floor(Date.now() / 1000),
    }),
    action: 'connect',
  });
  isConnected.value = true;
  addLog('info', `Sent Discord activity packet for App ID ${appIdCode}.`);
}
</script>

<style scoped></style>