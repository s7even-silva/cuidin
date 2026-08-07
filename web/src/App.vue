<script setup lang="ts">
import cuidinMark from './assets/cuidin-mark.png'
import NavIcon from './components/NavIcon.vue'
import { useBleStore } from './stores/ble'
import { useUmbralesStore } from './stores/umbrales'
import { useSonidosStore } from './stores/sonidos'
import { useEstadoStore } from './stores/estado'
import { useTheme } from './composables/useTheme'

const ble = useBleStore()
const { tema, alternarTema } = useTheme()

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

const secciones = [
  { to: '/conexion', etiqueta: 'Conexión', icono: 'conexion' as const },
  { to: '/umbrales', etiqueta: 'Umbrales', icono: 'umbrales' as const },
  { to: '/sonidos', etiqueta: 'Sonidos', icono: 'sonidos' as const },
  { to: '/estado', etiqueta: 'Estado', icono: 'estado' as const },
]
</script>

<template>
  <header class="cabecera">
    <div class="marca">
      <img :src="cuidinMark" alt="" class="logo" width="40" height="40" />
      <span class="nombre">Cuidín</span>
    </div>

    <nav class="nav nav-superior">
      <RouterLink v-for="s in secciones" :key="s.to" :to="s.to">
        <NavIcon :seccion="s.icono" />
        <span>{{ s.etiqueta }}</span>
      </RouterLink>
    </nav>

    <span
      class="indicador"
      :style="{ '--dot': puntoColor[ble.estado] ?? puntoColor.inactivo }"
      :title="`Estado de conexión: ${ble.estado}`"
    />

    <button
      type="button"
      class="boton-tema"
      :aria-label="tema === 'claro' ? 'Cambiar a modo oscuro' : 'Cambiar a modo claro'"
      :title="tema === 'claro' ? 'Cambiar a modo oscuro' : 'Cambiar a modo claro'"
      @click="alternarTema"
    >
      <svg v-if="tema === 'claro'" viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
        <circle cx="12" cy="12" r="4.2" />
        <path d="M12 2.5v2.4M12 19.1v2.4M4.6 4.6l1.7 1.7M17.7 17.7l1.7 1.7M2.5 12h2.4M19.1 12h2.4M4.6 19.4l1.7-1.7M17.7 6.3l1.7-1.7" />
      </svg>
      <svg v-else viewBox="0 0 24 24" width="20" height="20" fill="currentColor">
        <path d="M20.3 14.7A8.4 8.4 0 0 1 9.3 3.7a8.5 8.5 0 1 0 11 11z" />
      </svg>
    </button>
  </header>

  <main>
    <RouterView />
  </main>

  <nav class="nav nav-inferior" aria-label="Navegación principal">
    <RouterLink v-for="s in secciones" :key="s.to" :to="s.to" class="nav-inferior-item">
      <NavIcon :seccion="s.icono" />
      <span class="nav-inferior-texto">{{ s.etiqueta }}</span>
    </RouterLink>
  </nav>
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
}

.nav-superior {
  gap: 0.25rem;
  flex: 1;
  overflow-x: auto;
}

.nav-superior a {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  color: var(--text-muted);
  text-decoration: none;
  font-weight: 600;
  font-size: 0.92rem;
  padding: 0.45rem 0.85rem;
  border-radius: 999px;
  white-space: nowrap;
  transition: background-color 0.15s ease, color 0.15s ease;
}

.nav-superior a:hover {
  background: var(--color-sage-soft);
  color: var(--accent);
}

.nav-superior a.router-link-active {
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

.boton-tema {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.25rem;
  height: 2.25rem;
  border-radius: 50%;
  border: 1.5px solid var(--border);
  background: var(--bg-card-muted);
  color: var(--text);
  flex-shrink: 0;
  transition: background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease;
}
.boton-tema:hover {
  border-color: var(--accent-strong);
  color: var(--accent);
}

main {
  max-width: 640px;
  margin: 0 auto;
  padding: 1.75rem 1.25rem 3rem;
}

/* Tablet en adelante: la columna de lectura crece un poco para no dejar
   tanto espacio vacío a los lados, sin llegar al ancho completo de desktop
   (los formularios siguen siendo más legibles angostos). */
@media (min-width: 769px) {
  main {
    max-width: 720px;
  }
}
@media (min-width: 1024px) {
  main {
    max-width: 800px;
    padding-top: 2.25rem;
  }
}

.nav-inferior {
  display: none;
}

/* Tablet y móvil: la nav superior se oculta y aparece la barra inferior
   fija, más fácil de alcanzar con el pulgar. */
@media (max-width: 768px) {
  .nav-superior {
    display: none;
  }

  .cabecera {
    gap: 0.75rem;
    padding: 0.7rem 1rem;
  }
  .nombre {
    display: none;
  }

  main {
    padding-bottom: calc(1.5rem + 64px + env(safe-area-inset-bottom));
  }

  .nav-inferior {
    display: flex;
    position: fixed;
    bottom: 0;
    left: 0;
    right: 0;
    background: var(--bg-card);
    border-top: 1px solid var(--border);
    padding: 0.4rem 0.25rem calc(0.4rem + env(safe-area-inset-bottom));
    z-index: 10;
  }

  .nav-inferior-item {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 0.2rem;
    padding: 0.35rem 0.2rem;
    color: var(--text-muted);
    text-decoration: none;
    border-radius: var(--radius-sm);
    min-height: 44px;
    justify-content: center;
  }
  .nav-inferior-item.router-link-active {
    color: var(--accent);
  }
  .nav-inferior-texto {
    font-size: 0.68rem;
    font-weight: 700;
  }
}
</style>
