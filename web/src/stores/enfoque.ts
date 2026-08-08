import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { ConfigEnfoque, EstadoEnfoqueLive, PayloadEnfoque } from '../lib/bleProtocol'
import { useBleStore } from './ble'

export const useEnfoqueStore = defineStore('enfoque', () => {
  const ble = useBleStore()
  const config = ref<ConfigEnfoque | null>(null)
  const estado = ref<EstadoEnfoqueLive | null>(null)
  const guardando = ref(false)

  ble.suscribirseAEnfoque((json) => {
    const payload = json as PayloadEnfoque
    config.value = payload.config
    estado.value = payload.estado
    guardando.value = false
  })

  async function guardarConfig(cambios: Partial<ConfigEnfoque>) {
    guardando.value = true
    try {
      await ble.escribirComandoEnfoque({ accion: 'configurar', ...cambios })
      // La confirmación llega por notificación (arriba), que apaga
      // `guardando`. Si Cuidín no notifica por algún motivo, no nos
      // quedamos colgados esperando para siempre.
      setTimeout(() => {
        guardando.value = false
      }, 5000)
    } catch (err) {
      guardando.value = false
      throw err
    }
  }

  async function iniciarSesion() {
    await ble.escribirComandoEnfoque({ accion: 'iniciar' })
  }

  async function detener() {
    await ble.escribirComandoEnfoque({ accion: 'detener' })
  }

  return { config, estado, guardando, guardarConfig, iniciarSesion, detener }
})
