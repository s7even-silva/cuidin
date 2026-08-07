import { ref, watchEffect } from 'vue'

export type Tema = 'claro' | 'oscuro'

const CLAVE_STORAGE = 'cuidin-tema'

function temaGuardado(): Tema | null {
  const valor = localStorage.getItem(CLAVE_STORAGE)
  return valor === 'claro' || valor === 'oscuro' ? valor : null
}

// El tema por defecto es SIEMPRE claro (identidad de marca cálida), sin
// importar la preferencia del sistema operativo — a diferencia de antes,
// que seguía prefers-color-scheme automáticamente. El usuario puede
// cambiar a oscuro explícitamente con el botón, y esa elección se recuerda.
const tema = ref<Tema>(temaGuardado() ?? 'claro')

watchEffect(() => {
  document.documentElement.setAttribute('data-theme', tema.value)
  localStorage.setItem(CLAVE_STORAGE, tema.value)
})

function alternarTema() {
  tema.value = tema.value === 'claro' ? 'oscuro' : 'claro'
}

export function useTheme() {
  return { tema, alternarTema }
}
