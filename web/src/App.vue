<script setup lang="ts">
import cuidinMark from './assets/cuidin-mark.png'
import { useBleStore } from './stores/ble'
import { useUmbralesStore } from './stores/umbrales'
import { useSonidosStore } from './stores/sonidos'
import { useEstadoStore } from './stores/estado'

const ble = useBleStore()

// Los stores de datos deben existir desde el arranque (no solo al visitar
// su vista), porque cada uno se suscribe a notificaciones/lecturas BLE en
// su propia creación. Si se instanciaran de forma perezosa al navegar a la
// vista, la lectura inicial que ocurre justo al conectar (en ConnectView)
// se perdería para cualquier store que el usuario aún no hubiera visitado.
useUmbralesStore()
useSonidosStore()
useEstadoStore()

const puntoColor: Record<string, string> = {
  conectado: 'var(--accent-strong)',
  conectando: 'var(--color-warning)',
  desconectado: 'var(--color-danger)',
  error: 'var(--color-danger)',
  inactivo: 'var(--border)',
}
</script>

<template>
  <header class="cabecera">
    <div class="marca">
      <img :src="cuidinMark" alt="" class="logo" width="40" height="40" />
      <span class="nombre">Cuidín</span>
    </div>

    <nav class="nav">
      <RouterLink to="/conexion">Conexión</RouterLink>
      <RouterLink to="/umbrales">Umbrales</RouterLink>
      <RouterLink to="/sonidos">Sonidos</RouterLink>
      <RouterLink to="/estado">Estado</RouterLink>
    </nav>

    <span
      class="indicador"
      :style="{ '--dot': puntoColor[ble.estado] ?? puntoColor.inactivo }"
      :title="`Estado de conexión: ${ble.estado}`"
    />
  </header>
  <main>
    <RouterView />
  </main>
</template>

<style scoped>
.cabecera {
  display: flex;
  align-items: center;
  gap: 1.5rem;
  padding: 0.85rem 1.5rem;
  background: var(--bg-card);
  border-bottom: 1px solid var(--border);
  position: sticky;
  top: 0;
  z-index: 10;
}

.marca {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  flex-shrink: 0;
}

.logo {
  display: block;
  border-radius: 50%;
}

.nombre {
  font-family: var(--font-heading);
  font-weight: 700;
  font-size: 1.15rem;
  color: var(--primary);
}

.nav {
  display: flex;
  gap: 0.25rem;
  flex: 1;
  overflow-x: auto;
}

.nav a {
  color: var(--text-muted);
  text-decoration: none;
  font-weight: 600;
  font-size: 0.92rem;
  padding: 0.45rem 0.85rem;
  border-radius: 999px;
  white-space: nowrap;
  transition: background-color 0.15s ease, color 0.15s ease;
}

.nav a:hover {
  background: var(--color-sage-soft);
  color: var(--accent);
}

.nav a.router-link-active {
  background: var(--accent);
  color: var(--accent-on);
}

.indicador {
  width: 0.65rem;
  height: 0.65rem;
  border-radius: 50%;
  background: var(--dot);
  flex-shrink: 0;
  box-shadow: 0 0 0 3px color-mix(in srgb, var(--dot) 25%, transparent);
}

main {
  max-width: 640px;
  margin: 0 auto;
  padding: 1.75rem 1.25rem 3rem;
}

@media (max-width: 640px) {
  .cabecera {
    gap: 0.85rem;
    padding: 0.7rem 1rem;
  }
  .nombre {
    display: none;
  }
  .nav {
    gap: 0.1rem;
  }
  .nav a {
    padding: 0.4rem 0.6rem;
    font-size: 0.85rem;
  }
}
</style>
