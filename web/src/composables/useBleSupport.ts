import { computed } from 'vue'

export function useBleSupport() {
  const soportado = computed(() => {
    return typeof navigator !== 'undefined' && 'bluetooth' in navigator
  })

  const contextoSeguro = computed(() => {
    return typeof window !== 'undefined' && window.isSecureContext
  })

  const razon = computed(() => {
    if (!contextoSeguro.value) {
      return 'Esta página necesita cargarse por HTTPS (o localhost) para usar Bluetooth.'
    }
    if (!soportado.value) {
      return 'Este navegador no soporta Web Bluetooth. Usa Chrome, Edge u Opera (escritorio o Android). Safari y Firefox no lo soportan.'
    }
    return null
  })

  return {
    soportado: computed(() => soportado.value && contextoSeguro.value),
    razon,
  }
}
