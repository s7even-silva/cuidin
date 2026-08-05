// Reimplementación en TS del parser de Cuidin/rtttl.ino — debe traducir cada
// RTTTL exactamente igual que el firmware, para que el preview en el
// navegador suene como sonará realmente en el buzzer de Cuidín.

export interface NotaRtttl {
  /** Hz, 0 = pausa (sin tono) */
  frecuencia: number
  duracionMs: number
}

const FRECUENCIAS_OCTAVA_4: Record<string, number> = {
  c: 262,
  'c#': 277,
  d: 294,
  'd#': 311,
  e: 330,
  f: 349,
  'f#': 370,
  g: 392,
  'g#': 415,
  a: 440,
  'a#': 466,
  b: 494,
}

const ORDEN_NOTAS = ['c', 'c#', 'd', 'd#', 'e', 'f', 'f#', 'g', 'g#', 'a', 'a#', 'b']

export const LIMITE_CARACTERES_RTTTL = 200

export function rtttlEsValido(rtttl: string): boolean {
  if (rtttl.length === 0 || rtttl.length > LIMITE_CARACTERES_RTTTL) return false
  const primerosDosPuntos = rtttl.indexOf(':')
  if (primerosDosPuntos < 0) return false
  const segundosDosPuntos = rtttl.indexOf(':', primerosDosPuntos + 1)
  if (segundosDosPuntos < 0) return false
  return segundosDosPuntos + 1 < rtttl.length
}

function frecuenciaDeNota(indice: number, octava: number): number {
  if (indice < 0) return 0
  let frecuencia = FRECUENCIAS_OCTAVA_4[ORDEN_NOTAS[indice]]
  const desplazamiento = octava - 4
  if (desplazamiento > 0) {
    frecuencia *= Math.pow(2, desplazamiento)
  } else if (desplazamiento < 0) {
    frecuencia /= Math.pow(2, -desplazamiento)
  }
  return Math.round(frecuencia)
}

function indiceNota(letra: string): number {
  switch (letra) {
    case 'c':
      return 0
    case 'd':
      return 2
    case 'e':
      return 4
    case 'f':
      return 5
    case 'g':
      return 7
    case 'a':
      return 9
    case 'b':
      return 11
    default:
      return -1 // 'p' (pausa) u otro carácter no reconocido
  }
}

/** Lanza si el RTTTL no es válido — usar rtttlEsValido() antes si se quiere evitar la excepción. */
export function parsearRtttl(rtttl: string): NotaRtttl[] {
  if (!rtttlEsValido(rtttl)) {
    throw new Error('RTTTL inválido: falta el formato "nombre:defaults:notas".')
  }

  const primerosDosPuntos = rtttl.indexOf(':')
  const segundosDosPuntos = rtttl.indexOf(':', primerosDosPuntos + 1)

  const defaults = rtttl.slice(primerosDosPuntos + 1, segundosDosPuntos)
  let duracionDefecto = 4
  let octavaDefecto = 5
  let bpm = 63
  for (const parteRaw of defaults.split(',')) {
    const parte = parteRaw.trim()
    if (parte.startsWith('d=')) duracionDefecto = parseInt(parte.slice(2), 10) || duracionDefecto
    else if (parte.startsWith('o=')) octavaDefecto = parseInt(parte.slice(2), 10) || octavaDefecto
    else if (parte.startsWith('b=')) bpm = parseInt(parte.slice(2), 10) || bpm
  }
  if (duracionDefecto <= 0) duracionDefecto = 4
  if (octavaDefecto <= 0) octavaDefecto = 5
  if (bpm <= 0) bpm = 63

  const msPorRedonda = (60000 / bpm) * 4

  const notasTexto = rtttl.slice(segundosDosPuntos + 1)
  const notas: NotaRtttl[] = []

  for (const notaRaw of notasTexto.split(',')) {
    const nota = notaRaw.trim()
    if (nota.length === 0) continue

    let p = 0

    let duracion = duracionDefecto
    const inicioDuracion = p
    while (p < nota.length && /\d/.test(nota[p])) p++
    if (p > inicioDuracion) duracion = parseInt(nota.slice(inicioDuracion, p), 10)

    if (p >= nota.length) continue
    const letra = nota[p].toLowerCase()
    p++

    let sostenido = false
    if (p < nota.length && nota[p] === '#') {
      sostenido = true
      p++
    }

    let octava = octavaDefecto
    const inicioOctava = p
    while (p < nota.length && /\d/.test(nota[p])) p++
    if (p > inicioOctava) octava = parseInt(nota.slice(inicioOctava, p), 10)

    const punteada = p < nota.length && nota[p] === '.'

    let indice = indiceNota(letra)
    if (indice >= 0 && sostenido) indice++
    if (indice > 11) indice -= 12

    const frecuencia = frecuenciaDeNota(indice, octava)
    let duracionMs = msPorRedonda / duracion
    if (punteada) duracionMs *= 1.5

    notas.push({ frecuencia, duracionMs })
  }

  return notas
}
