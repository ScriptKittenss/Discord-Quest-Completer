<script setup lang="ts">
const props = defineProps<{
  title: string;
  description?: string;
  imageUrl?: string;
  launchId?: string;
}>();

const emit = defineEmits(['launch']);

function launchGame() {
  emit('launch', props.launchId);
}
</script>

<template>
  <div class="group bg-ink-800 border border-ink-700/60 rounded-2xl overflow-hidden transition-all duration-200 hover:border-brand-600/50 hover:shadow-[0_8px_30px_rgba(217,43,64,0.15)] hover:-translate-y-0.5">
    <div class="relative h-44 bg-ink-900">
      <img v-if="imageUrl" :src="imageUrl" alt="Game cover" class="w-full h-full object-cover" />
      <div v-else class="flex items-center justify-center w-full h-full">
        <img src="/logo.png" alt="" class="h-12 w-12 opacity-30 rounded-xl" />
      </div>
    </div>
    <div class="p-4">
      <h3 class="text-[15px] font-semibold text-white mb-1 truncate">{{ title }}</h3>
      <p v-if="description" class="text-[13px] text-zinc-400 mb-4 line-clamp-2">{{ description }}</p>
      <button
        @click="launchGame"
        class="w-full bg-brand-600 hover:bg-brand-500 active:bg-brand-700 text-white text-sm font-semibold py-2 px-4 rounded-xl transition-colors shadow-[0_2px_12px_rgba(217,43,64,0.35)]"
      >
        Launch
      </button>
    </div>
  </div>
</template>
