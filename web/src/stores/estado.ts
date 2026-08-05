import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { Estado } from '../lib/bleProtocol'
import { useBleStore } from './ble'

export const useEstadoStore = defineStore('estado', () => {
  const ble = useBleStore()
  const actual = ref<Estado | null>(null)

  ble.suscribirseAEstado((json) => {
    actual.value = json as Estado
  })

  return { actual }
})
