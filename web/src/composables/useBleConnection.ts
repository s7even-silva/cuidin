import { ref, shallowRef } from 'vue'
import {
  UUID_SERVICIO,
  UUID_CARACT_LEER,
  UUID_CARACT_ESCRIBIR,
  UUID_CARACT_SONIDOS,
} from '../lib/bleProtocol'

export type EstadoConexion = 'inactivo' | 'conectando' | 'conectado' | 'desconectado' | 'error'

const estado = ref<EstadoConexion>('inactivo')
const nombreDispositivo = ref<string | null>(null)
const ultimoError = ref<string | null>(null)

const dispositivo = shallowRef<BluetoothDevice | null>(null)
const caractLeer = shallowRef<BluetoothRemoteGATTCharacteristic | null>(null)
const caractEscribir = shallowRef<BluetoothRemoteGATTCharacteristic | null>(null)
const caractSonidos = shallowRef<BluetoothRemoteGATTCharacteristic | null>(null)

type ListenerJSON = (json: unknown) => void
const listenersLeer = new Set<ListenerJSON>()
const listenersSonidos = new Set<ListenerJSON>()

const decodificador = new TextDecoder('utf-8')
const codificador = new TextEncoder()

function parsearValorJSON(valor: DataView | undefined, listeners: Set<ListenerJSON>) {
  if (!valor) return
  const texto = decodificador.decode(valor)
  try {
    const json = JSON.parse(texto)
    listeners.forEach((fn) => fn(json))
  } catch {
    console.warn('Cuidín BLE: notificación con JSON inválido, se ignora:', texto)
  }
}

function parsearEventoJSON(event: Event, listeners: Set<ListenerJSON>) {
  const caract = event.target as BluetoothRemoteGATTCharacteristic
  parsearValorJSON(caract.value, listeners)
}

function alDesconectar() {
  estado.value = 'desconectado'
  caractLeer.value = null
  caractEscribir.value = null
  caractSonidos.value = null
}

async function conectar() {
  if (!navigator.bluetooth) {
    ultimoError.value = 'Este navegador no soporta Web Bluetooth.'
    estado.value = 'error'
    return
  }

  estado.value = 'conectando'
  ultimoError.value = null

  try {
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [UUID_SERVICIO] }],
    })
    dispositivo.value = device
    nombreDispositivo.value = device.name ?? 'Cuidín'
    device.addEventListener('gattserverdisconnected', alDesconectar)

    await conectarGatt(device)
  } catch (err) {
    // El usuario canceló el diálogo de selección, o falló la conexión.
    ultimoError.value = err instanceof Error ? err.message : String(err)
    estado.value = dispositivo.value ? 'desconectado' : 'inactivo'
  }
}

async function conectarGatt(device: BluetoothDevice) {
  if (!device.gatt) throw new Error('El dispositivo no expone GATT.')

  estado.value = 'conectando'
  const server = await device.gatt.connect()
  const servicio = await server.getPrimaryService(UUID_SERVICIO)

  caractLeer.value = await servicio.getCharacteristic(UUID_CARACT_LEER)
  caractEscribir.value = await servicio.getCharacteristic(UUID_CARACT_ESCRIBIR)
  caractSonidos.value = await servicio.getCharacteristic(UUID_CARACT_SONIDOS)

  caractLeer.value.addEventListener('characteristicvaluechanged', (e) =>
    parsearEventoJSON(e, listenersLeer),
  )
  await caractLeer.value.startNotifications()

  caractSonidos.value.addEventListener('characteristicvaluechanged', (e) =>
    parsearEventoJSON(e, listenersSonidos),
  )
  await caractSonidos.value.startNotifications()

  // Lectura inicial de ambas características (no hay que esperar a la
  // primera notificación para tener el estado actual).
  const valorLeer = await caractLeer.value.readValue()
  parsearValorJSON(valorLeer, listenersLeer)

  const valorSonidos = await caractSonidos.value.readValue()
  parsearValorJSON(valorSonidos, listenersSonidos)

  estado.value = 'conectado'
  ultimoError.value = null
}

async function reconectar() {
  if (!dispositivo.value) return
  try {
    await conectarGatt(dispositivo.value)
  } catch (err) {
    ultimoError.value = err instanceof Error ? err.message : String(err)
    estado.value = 'desconectado'
  }
}

function desconectar() {
  dispositivo.value?.gatt?.disconnect()
}

async function escribirUmbrales(cambios: Record<string, unknown>) {
  if (!caractEscribir.value) throw new Error('No hay conexión activa con Cuidín.')
  const payload = codificador.encode(JSON.stringify(cambios))
  await caractEscribir.value.writeValue(payload)
}

async function escribirComandoSonidos(comando: Record<string, unknown>) {
  if (!caractSonidos.value) throw new Error('No hay conexión activa con Cuidín.')
  const payload = codificador.encode(JSON.stringify(comando))
  await caractSonidos.value.writeValue(payload)
}

function suscribirseAUmbrales(fn: ListenerJSON) {
  listenersLeer.add(fn)
  return () => listenersLeer.delete(fn)
}

function suscribirseASonidos(fn: ListenerJSON) {
  listenersSonidos.add(fn)
  return () => listenersSonidos.delete(fn)
}

/**
 * Composable singleton: el estado de conexión BLE es compartido por toda la
 * app (una sola conexión activa a la vez), así que el estado vive a nivel de
 * módulo en vez de crearse por cada llamada al composable.
 */
export function useBleConnection() {
  return {
    estado,
    nombreDispositivo,
    ultimoError,
    conectar,
    reconectar,
    desconectar,
    escribirUmbrales,
    escribirComandoSonidos,
    suscribirseAUmbrales,
    suscribirseASonidos,
  }
}
