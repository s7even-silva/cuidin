import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { ConfigEnfoque, EstadoEnfoqueLive, PayloadEnfoque } from '../lib/bleProtocol'
import { useBleStore } from './ble'

export const useEnfoqueStore = defineStore('enfoque', () => {
  const ble = useBleStore()
  const config = ref<ConfigEnfoque | null>(null)
  const estado = ref<EstadoEnfoqueLive | null>(null)
  const guardando = ref(false)

  function configCambio(a: ConfigEnfoque | null, b: ConfigEnfoque) {
    if (!a) return true
    return (
      a.sesion !== b.sesion ||
      a.descCorto !== b.descCorto ||
      a.descLargo !== b.descLargo ||
      a.nSesiones !== b.nSesiones ||
      a.debounce !== b.debounce ||
      a.hito1 !== b.hito1 ||
      a.hito2 !== b.hito2 ||
      a.volumen !== b.volumen ||
      a.rtttlId !== b.rtttlId
    )
  }

  ble.suscribirseAEnfoque((json) => {
    const payload = json as PayloadEnfoque
    // CARACT_ENFOQUE se renotifica cada segundo (para que "estado" avance en
    // vivo), pero "config" casi nunca cambia. Si reasignáramos config.value
    // en cada notificación, el watch de FocusView.vue (que sincroniza el
    // formulario) se dispararía cada segundo y pisaría lo que el usuario
    // esté escribiendo. Solo se reasigna (nueva referencia) cuando algún
    // campo realmente cambió, para no disparar ese watch de más.
    if (configCambio(config.value, payload.config)) {
      config.value = payload.config
    }
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
