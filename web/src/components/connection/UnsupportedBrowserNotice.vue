<script setup lang="ts">
defineProps<{
  razon: string
}>()
</script>

<template>
  <div class="aviso aviso-warning">
    <strong>Bluetooth no disponible</strong>
    <p>{{ razon }}</p>
  </div>
</template>
