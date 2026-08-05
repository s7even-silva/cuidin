import { ref } from 'vue'
import { parsearRtttl } from '../lib/rtttl'

export function useRtttlAudioPreview() {
  const reproduciendo = ref(false)
  let contexto: AudioContext | null = null

  async function reproducir(rtttl: string) {
    const notas = parsearRtttl(rtttl)
    if (!contexto) contexto = new AudioContext()
    if (contexto.state === 'suspended') await contexto.resume()

    reproduciendo.value = true
    let inicio = contexto.currentTime

    for (const nota of notas) {
      const duracionSeg = nota.duracionMs / 1000
      if (nota.frecuencia > 0) {
        const osc = contexto.createOscillator()
        const ganancia = contexto.createGain()
        osc.type = 'square'
        osc.frequency.value = nota.frecuencia
        ganancia.gain.value = 0.15 // volumen moderado, es solo un preview
        osc.connect(ganancia)
        ganancia.connect(contexto.destination)
        osc.start(inicio)
        osc.stop(inicio + duracionSeg * 0.9) // pequeño gap entre notas, como el buzzer real
      }
      inicio += duracionSeg
    }

    const duracionTotalMs = (inicio - contexto.currentTime) * 1000
    setTimeout(() => {
      reproduciendo.value = false
    }, Math.max(0, duracionTotalMs))
  }

  return { reproduciendo, reproducir }
}
