<script setup lang="ts">
import { computed } from 'vue'
import { useBleStore } from '../../stores/ble'

const ble = useBleStore()

const info = computed(() => {
  switch (ble.estado) {
    case 'conectado':
      return {
        tono: 'conectado',
        texto: `Conectado${ble.nombreDispositivo ? ' a ' + ble.nombreDispositivo : ''}`,
      }
    case 'conectando':
      return { tono: 'conectando', texto: 'Conectando…' }
    case 'desconectado':
      return { tono: 'desconectado', texto: 'Desconectado' }
    case 'error':
      return { tono: 'error', texto: 'Error de conexión' }
    default:
      return { tono: 'inactivo', texto: 'Sin conectar' }
  }
})
</script>

<template>
  <div class="badge" :data-tono="info.tono">
    <span class="punto" />
    <span>{{ info.texto }}</span>
  </div>
</template>

<style scoped>
.badge {
  display: inline-flex;
  align-items: center;
  gap: 0.55rem;
  font-size: 0.92rem;
  font-weight: 600;
  padding: 0.45rem 0.9rem;
  border-radius: 999px;
  background: var(--bg-card-muted);
  color: var(--text);
}
.punto {
  width: 0.6rem;
  height: 0.6rem;
  border-radius: 50%;
  background: var(--border);
  flex-shrink: 0;
}
.badge[data-tono='conectado'] .punto {
  background: var(--accent-strong);
  box-shadow: 0 0 0 3px color-mix(in srgb, var(--accent-strong) 25%, transparent);
}
.badge[data-tono='conectando'] .punto {
  background: var(--color-warning);
  animation: parpadeo 1s ease-in-out infinite;
}
.badge[data-tono='desconectado'] .punto,
.badge[data-tono='error'] .punto {
  background: var(--color-danger);
}

@keyframes parpadeo {
  50% {
    opacity: 0.35;
  }
}

@media (prefers-reduced-motion: reduce) {
  .badge[data-tono='conectando'] .punto {
    animation: none;
  }
}
</style>
