import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { Umbrales } from '../lib/bleProtocol'
import { useBleStore } from './ble'

export const useUmbralesStore = defineStore('umbrales', () => {
  const ble = useBleStore()
  const actuales = ref<Umbrales | null>(null)
  const guardando = ref(false)

  ble.suscribirseAUmbrales((json) => {
    actuales.value = json as Umbrales
    guardando.value = false
  })

  async function guardar(cambios: Partial<Umbrales>) {
    guardando.value = true
    try {
      await ble.escribirUmbrales(cambios)
      // La confirmación llega por notificación (ble.suscribirseAUmbrales arriba),
      // que apaga `guardando`. Si Cuidín no notifica por algún motivo, no nos
      // quedamos colgados esperando para siempre.
      setTimeout(() => {
        guardando.value = false
      }, 5000)
    } catch (err) {
      guardando.value = false
      throw err
    }
  }

  return { actuales, guardando, guardar }
})
