<script setup lang="ts">
import { computed } from 'vue'
import { useBleStore } from '../../stores/ble'

const ble = useBleStore()

const info = computed(() => {
  switch (ble.estado) {
    case 'conectado':
      return { color: '#3ddc84', texto: `Conectado${ble.nombreDispositivo ? ' a ' + ble.nombreDispositivo : ''}` }
    case 'conectando':
      return { color: '#e0c341', texto: 'Conectando…' }
    case 'desconectado':
      return { color: '#e07a41', texto: 'Desconectado' }
    case 'error':
      return { color: '#e04141', texto: 'Error de conexión' }
    default:
      return { color: '#777', texto: 'Sin conectar' }
  }
})
</script>

<template>
  <div class="badge">
    <span class="punto" :style="{ backgroundColor: info.color }" />
    <span>{{ info.texto }}</span>
  </div>
</template>

<style scoped>
.badge {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.9rem;
}
.punto {
  width: 0.6rem;
  height: 0.6rem;
  border-radius: 50%;
  display: inline-block;
}
</style>
