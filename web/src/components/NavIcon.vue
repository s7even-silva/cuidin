<script setup lang="ts">
defineProps<{
  seccion: 'conexion' | 'umbrales' | 'sonidos' | 'estado'
}>()
</script>

<template>
  <svg
    viewBox="0 0 24 24"
    width="20"
    height="20"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
    aria-hidden="true"
  >
    <path v-if="seccion === 'conexion'" d="M6.5 6.5 17.5 17.5M12 2v7.5l3-3M12 22v-7.5l-3 3M6.5 17.5 17.5 6.5" />
    <template v-else-if="seccion === 'umbrales'">
      <path d="M4 6h10M4 12h16M4 18h7" />
      <circle cx="17" cy="6" r="2" />
      <circle cx="9" cy="18" r="2" />
    </template>
    <template v-else-if="seccion === 'sonidos'">
      <path d="M9 18V6l10-2v12" />
      <circle cx="6.5" cy="18" r="2.5" />
      <circle cx="16.5" cy="16" r="2.5" />
    </template>
    <path v-else d="M3 12h4l2-7 4 14 2-7h6" />
  </svg>
</template>
