<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  segundos: number
  activo: boolean
}>()

const texto = computed(() => {
  const s = Math.max(0, props.segundos)
  const min = Math.floor(s / 60)
  const seg = s % 60
  return `${String(min).padStart(2, '0')}:${String(seg).padStart(2, '0')}`
})
</script>

<template>
  <div class="cuenta" :class="{ inactivo: !activo }">{{ texto }}</div>
</template>

<style scoped>
.cuenta {
  font-family: var(--font-heading);
  font-size: 3rem;
  font-weight: 700;
  text-align: center;
  color: var(--primary);
  line-height: 1.1;
  letter-spacing: 0.02em;
}
.cuenta.inactivo {
  color: var(--text-muted);
}
</style>
