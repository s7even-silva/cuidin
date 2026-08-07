<script setup lang="ts">
import cuidinMark from '../assets/cuidin-mark.png'
import { useBleSupport } from '../composables/useBleSupport'
import ConnectionBadge from '../components/connection/ConnectionBadge.vue'
import ConnectButton from '../components/connection/ConnectButton.vue'
import UnsupportedBrowserNotice from '../components/connection/UnsupportedBrowserNotice.vue'

const { soportado, razon } = useBleSupport()
</script>

<template>
  <section class="bienvenida">
    <img :src="cuidinMark" alt="" class="mascota" width="112" height="112" />
    <h1>Conexión con Cuidín</h1>
    <p>
      Cuidín debe estar cerca y encendido. Al conectar, el navegador te pedirá elegir el
      dispositivo llamado <strong>Cuidin</strong> en una ventana emergente.
    </p>

    <UnsupportedBrowserNotice v-if="!soportado && razon" :razon="razon" />

    <div v-else class="acciones">
      <ConnectionBadge />
      <ConnectButton />
    </div>
  </section>
</template>

<style scoped>
.bienvenida {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  padding-top: 1rem;
}
.mascota {
  margin-bottom: 0.75rem;
}
.acciones {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.9rem;
  margin-top: 0.5rem;
}
</style>
