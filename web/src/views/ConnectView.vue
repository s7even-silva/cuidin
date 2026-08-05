<script setup lang="ts">
import { useBleSupport } from '../composables/useBleSupport'
import ConnectionBadge from '../components/connection/ConnectionBadge.vue'
import ConnectButton from '../components/connection/ConnectButton.vue'
import UnsupportedBrowserNotice from '../components/connection/UnsupportedBrowserNotice.vue'

const { soportado, razon } = useBleSupport()
</script>

<template>
  <section>
    <h1>Conexión con Cuidín</h1>
    <p>
      Cuidín debe estar cerca y encendido. Al conectar, el navegador te pedirá elegir el
      dispositivo llamado <strong>Cuidin</strong> en una ventana emergente.
    </p>

    <UnsupportedBrowserNotice v-if="!soportado && razon" :razon="razon" />

    <template v-else>
      <ConnectionBadge />
      <ConnectButton />
    </template>
  </section>
</template>
