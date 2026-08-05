<script setup lang="ts">
import { computed } from 'vue'
import { useBleStore } from '../../stores/ble'

const ble = useBleStore()

const puedeReconectar = computed(() => ble.estado === 'desconectado')
const conectando = computed(() => ble.estado === 'conectando')

function alClick() {
  if (puedeReconectar.value) {
    ble.reconectar()
  } else {
    ble.conectar()
  }
}
</script>

<template>
  <button :disabled="conectando" @click="alClick">
    {{ conectando ? 'Conectando…' : puedeReconectar ? 'Reconectar' : 'Conectar con Cuidín' }}
  </button>
  <p v-if="ble.ultimoError" class="error">{{ ble.ultimoError }}</p>
</template>

<style scoped>
button {
  padding: 0.6rem 1.2rem;
  font-size: 1rem;
  border-radius: 8px;
  border: none;
  background: #3ddc84;
  color: #0a0a0a;
  cursor: pointer;
}
button:disabled {
  opacity: 0.6;
  cursor: default;
}
.error {
  color: #e07a7a;
  font-size: 0.85rem;
}
</style>
