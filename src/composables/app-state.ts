import { createGlobalState } from '@vueuse/core'
import { computed, ComputedRef, ref, Ref, ShallowRef, shallowRef } from 'vue'

export const Pages = {
    HOME: 'home',
    PLAYGROUND: 'playground',
} as const
export type Pages = typeof Pages[keyof typeof Pages]

export interface AppLogObject {
    type: 'info' | 'error' | 'warning' | 'debug';
    message: string;
    timestamp: Date;
}

export interface ActiveQuest {
    id: string;
    name: string;
    executableName?: string;
    startedAt: Date;
}

export interface UseGlobalStateReturn {
    page: ShallowRef<Pages>;
    count: ShallowRef<number>;
    doubleCount: ComputedRef<number>;
    setPage: (newPage: Pages) => void;
    increment: () => void;
    logs: ShallowRef<AppLogObject[]>;
    addLog: {
        (type: 'info' | 'error' | 'warning' | 'debug', newLog: string): void;
        (newLog: string): void;
    };
    clearLogs: () => void;
    activeQuest: Ref<ActiveQuest | null>;
    setActiveQuest: (quest: { id: string; name: string; executableName?: string } | null) => void;
}

export const useGlobalState = createGlobalState(
  () => {
    const page = shallowRef<Pages>(Pages.HOME)
    const logs = shallowRef<AppLogObject[]>([])
    const count = shallowRef(0)
    const activeQuest = ref<ActiveQuest | null>(null)

    const doubleCount = computed(() => count.value * 2)

    function increment() {
      count.value++
    }

    function setPage(newPage: Pages) {
      page.value = newPage
    }

    function setActiveQuest(quest: { id: string; name: string; executableName?: string } | null) {
      if (!quest) {
        activeQuest.value = null
      } else {
        activeQuest.value = {
          ...quest,
          startedAt: new Date(),
        }
      }
    }

    function addLog(type: string | 'info' | 'error' | 'warning' | 'debug', newLog?: string) {
      if (!newLog) {
        newLog = type;
        type = 'info';
      }
      const formattedLog = `${newLog}`;
      logs.value = [
        ...logs.value,
        { type: type as 'info' | 'error' | 'warning' | 'debug', message: formattedLog, timestamp: new Date() }
      ];
    }

    function clearLogs() {
      logs.value = []
    }

    return {
        page,
        count, 
        doubleCount,
        setPage, 
        increment,
        logs,
        addLog,
        clearLogs,
        activeQuest,
        setActiveQuest
    } as UseGlobalStateReturn
  }
)
