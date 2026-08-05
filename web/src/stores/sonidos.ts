import { ref } from 'vue'
import { defineStore } from 'pinia'
import type { IndiceSonidos, SonidoMeta } from '../lib/bleProtocol'
import { useBleStore } from './ble'

export const useSonidosStore = defineStore('sonidos', () => {
  const ble = useBleStore()
  const lista = ref<SonidoMeta[]>([])
  const procesando = ref(false)

  ble.suscribirseASonidos((json) => {
    const indice = json as IndiceSonidos
    lista.value = indice.sonidos ?? []
    procesando.value = false
  })

  async function subir(nombre: string, rtttl: string) {
    procesando.value = true
    try {
      await ble.escribirComandoSonidos({ accion: 'subir', nombre, rtttl })
      setTimeout(() => {
        procesando.value = false
      }, 5000)
    } catch (err) {
      procesando.value = false
      throw err
    }
  }

  async function borrar(id: string) {
    procesando.value = true
    try {
      await ble.escribirComandoSonidos({ accion: 'borrar', id })
      setTimeout(() => {
        procesando.value = false
      }, 5000)
    } catch (err) {
      procesando.value = false
      throw err
    }
  }

  async function probarEnDispositivo(id: string) {
    // No hay notificación de confirmación para "probar" (no cambia estado
    // persistido), así que no tocamos `procesando`.
    await ble.escribirComandoSonidos({ accion: 'probar', id })
  }

  return { lista, procesando, subir, borrar, probarEnDispositivo }
})
