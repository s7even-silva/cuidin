// Espejo de docs/protocol.md — si cambias algo aquí, cambia también el
// documento y Cuidin/ble.ino.

export const NOMBRE_DISPOSITIVO = 'Cuidin'

export const UUID_SERVICIO = '6d5a1000-0001-4c1a-8b1a-2f6a9c8e1a01'
export const UUID_CARACT_LEER = '6d5a1000-0002-4c1a-8b1a-2f6a9c8e1a01'
export const UUID_CARACT_ESCRIBIR = '6d5a1000-0003-4c1a-8b1a-2f6a9c8e1a01'
export const UUID_CARACT_SONIDOS = '6d5a1000-0004-4c1a-8b1a-2f6a9c8e1a01'

export interface Umbrales {
  dist: number
  tmin: number
  tmax: number
  hmin: number
  hmax: number
  sonido: number
  luz: number
  umbralp: number
  volumen: number
  activa: boolean
  prio: number
  mostrar: number
  patron: number
  sonidoRtttlId: string
  modoCamara: boolean
}

// Bitmask del campo `prio` — qué problemas disparan la alarma por sí solos.
export const PRIO_TEMP = 1 << 0
export const PRIO_HUM = 1 << 1
export const PRIO_SONIDO = 1 << 2
export const PRIO_LUZ = 1 << 3
export const PRIO_POSTURA = 1 << 4

export const PRIO_BITS = [
  { bit: PRIO_TEMP, etiqueta: 'Temperatura fuera de rango' },
  { bit: PRIO_HUM, etiqueta: 'Humedad fuera de rango' },
  { bit: PRIO_SONIDO, etiqueta: 'Sonido fuerte' },
  { bit: PRIO_LUZ, etiqueta: 'Poca luz' },
  { bit: PRIO_POSTURA, etiqueta: 'Postura mala' },
] as const

// Bitmask del campo `mostrar` — qué filas se ven en la vista de datos del TFT.
export const MOSTRAR_PERSONA = 1 << 0
export const MOSTRAR_TEMP = 1 << 1
export const MOSTRAR_HUM = 1 << 2
export const MOSTRAR_LUZ = 1 << 3
export const MOSTRAR_SONIDO = 1 << 4
export const MOSTRAR_POSTURA = 1 << 5

export const MOSTRAR_BITS = [
  { bit: MOSTRAR_PERSONA, etiqueta: 'Presencia (distancia)' },
  { bit: MOSTRAR_TEMP, etiqueta: 'Temperatura' },
  { bit: MOSTRAR_HUM, etiqueta: 'Humedad' },
  { bit: MOSTRAR_LUZ, etiqueta: 'Luz' },
  { bit: MOSTRAR_SONIDO, etiqueta: 'Nivel de sonido' },
  { bit: MOSTRAR_POSTURA, etiqueta: 'Postura' },
] as const

export interface SonidoMeta {
  id: string
  nombre: string
}

export interface IndiceSonidos {
  sonidos: SonidoMeta[]
}

export type ComandoSonidos =
  | { accion: 'subir'; nombre: string; rtttl: string }
  | { accion: 'borrar'; id: string }
  | { accion: 'probar'; id: string }

export const LIMITE_SONIDOS = 8
// LIMITE_CARACTERES_RTTTL vive en lib/rtttl.ts, junto al parser que lo usa.
