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
  <button class="btn btn-primary" :disabled="conectando" @click="alClick">
    {{ conectando ? 'Conectando…' : puedeReconectar ? 'Reconectar' : 'Conectar con Cuidín' }}
  </button>
  <p v-if="ble.ultimoError" class="error">{{ ble.ultimoError }}</p>
</template>

<style scoped>
.error {
  color: var(--color-danger);
  font-size: 0.85rem;
  margin-top: 0.5rem;
}
</style>
