<script setup lang="ts">
defineProps<{
  razon: string
}>()
</script>

<template>
  <div class="aviso">
    <strong>Bluetooth no disponible</strong>
    <p>{{ razon }}</p>
  </div>
</template>

<style scoped>
.aviso {
  background: #3a1f1f;
  border: 1px solid #7a3b3b;
  color: #f5d5d5;
  border-radius: 8px;
  padding: 1rem;
}
.aviso p {
  margin: 0.5rem 0 0;
}
</style>
