<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { useBleStore } from '../stores/ble'
import { useEnfoqueStore } from '../stores/enfoque'
import { useSonidosStore } from '../stores/sonidos'
import CuentaRegresiva from '../components/CuentaRegresiva.vue'
import type { ConfigEnfoque } from '../lib/bleProtocol'

const ble = useBleStore()
const enfoque = useEnfoqueStore()
const sonidos = useSonidosStore()

const textoEstado: Record<string, string> = {
  inactivo: 'Inactivo',
  en_sesion: 'En sesión',
  pausado_ausencia: 'Pausado — sin presencia',
  descanso_corto: 'Descanso corto',
  descanso_largo: 'Descanso largo',
}

// El formulario trabaja en minutos (más natural para el usuario); el
// struct del firmware persiste segundos, así que se convierte al guardar.
const form = reactive({
  sesionMin: 25,
  descCortoMin: 5,
  descLargoMin: 15,
  nSesiones: 4,
  debounce: 5,
  hito1: 80,
  hito2: 90,
  volumen: 80,
})

watch(
  () => enfoque.config,
  (nuevo) => {
    if (!nuevo) return
    form.sesionMin = Math.round(nuevo.sesion / 60)
    form.descCortoMin = Math.round(nuevo.descCorto / 60)
    form.descLargoMin = Math.round(nuevo.descLargo / 60)
    form.nSesiones = nuevo.nSesiones
    form.debounce = nuevo.debounce
    form.hito1 = nuevo.hito1
    form.hito2 = nuevo.hito2
    form.volumen = nuevo.volumen
  },
  { immediate: true },
)

async function guardarConfig() {
  const cambios: Partial<ConfigEnfoque> = {
    sesion: form.sesionMin * 60,
    descCorto: form.descCortoMin * 60,
    descLargo: form.descLargoMin * 60,
    nSesiones: form.nSesiones,
    debounce: form.debounce,
    hito1: form.hito1,
    hito2: form.hito2,
    volumen: form.volumen,
  }
  await enfoque.guardarConfig(cambios)
}

async function usarSonido(id: string) {
  await enfoque.guardarConfig({ rtttlId: id })
}

async function dejarDeUsarSonido() {
  await enfoque.guardarConfig({ rtttlId: '' })
}

const idSonidoActivo = computed(() => enfoque.config?.rtttlId ?? '')

const duracionEtapaActualSeg = computed(() => {
  const est = enfoque.estado
  const cfg = enfoque.config
  if (!est || !cfg) return 0
  switch (est.estado) {
    case 'en_sesion':
    case 'pausado_ausencia':
      return cfg.sesion
    case 'descanso_corto':
      return cfg.descCorto
    case 'descanso_largo':
      return cfg.descLargo
    default:
      return 0
  }
})

const segundosRestantes = computed(() => {
  const est = enfoque.estado
  if (!est) return 0
  return duracionEtapaActualSeg.value - est.transcurrido
})

const enPausa = computed(() => enfoque.estado?.estado === 'pausado_ausencia')
</script>

<template>
  <section>
    <h1>Modo Enfoque</h1>
    <p class="intro">
      Temporizador tipo Pomodoro: avisa cuando ya llevas suficiente tiempo sentado y toca
      descansar. Sigue funcionando aunque cierres esta página — el conteo vive en Cuidín.
    </p>

    <div v-if="ble.estado !== 'conectado'" class="aviso aviso-info">
      Conéctate a Cuidín primero para usar el Modo Enfoque.
    </div>

    <template v-else-if="enfoque.estado && enfoque.config">
      <div class="resumen" :class="{ alarma: enPausa }">
        <span class="punto" />
        <span>{{ textoEstado[enfoque.estado.estado] ?? enfoque.estado.estado }}</span>
      </div>

      <CuentaRegresiva
        :segundos="segundosRestantes"
        :activo="enfoque.estado.estado !== 'inactivo'"
      />

      <p class="ciclo">Ciclo {{ enfoque.estado.ciclo }} / {{ enfoque.config.nSesiones }}</p>

      <div class="acciones-principales">
        <button
          v-if="enfoque.estado.estado === 'inactivo'"
          type="button"
          class="btn btn-primary btn-ancho"
          @click="enfoque.iniciarSesion"
        >
          Iniciar sesión
        </button>
        <button v-else type="button" class="btn btn-danger btn-ancho" @click="enfoque.detener">
          Detener
        </button>
      </div>

      <form @submit.prevent="guardarConfig">
        <fieldset>
          <legend>Duraciones</legend>
          <div class="par">
            <label class="campo"><span>Sesión (min)</span><input type="number" min="1" v-model.number="form.sesionMin" /></label>
            <label class="campo"><span>Descanso corto (min)</span><input type="number" min="1" v-model.number="form.descCortoMin" /></label>
          </div>
          <div class="par">
            <label class="campo"><span>Descanso largo (min)</span><input type="number" min="1" v-model.number="form.descLargoMin" /></label>
            <label class="campo"><span>Sesiones antes del descanso largo</span><input type="number" min="1" v-model.number="form.nSesiones" /></label>
          </div>
        </fieldset>

        <fieldset>
          <legend>Avisos</legend>
          <div class="par">
            <label class="campo"><span>Primer aviso (%)</span><input type="number" min="1" max="99" v-model.number="form.hito1" /></label>
            <label class="campo"><span>Segundo aviso (%)</span><input type="number" min="1" max="99" v-model.number="form.hito2" /></label>
          </div>
          <label class="campo">
            <span>Debounce de presencia (seg)</span>
            <input type="number" min="1" v-model.number="form.debounce" />
          </label>
          <label class="campo">
            <span>Volumen ({{ form.volumen }}%)</span>
            <input type="range" min="0" max="100" v-model.number="form.volumen" />
          </label>
        </fieldset>

        <fieldset>
          <legend>Sonido de fin de etapa</legend>
          <p class="ayuda">
            Si no eliges ninguno, Cuidín usa un patrón de dos tonos generado.
          </p>
          <p v-if="sonidos.lista.length === 0" class="vacio">
            Todavía no hay melodías guardadas (ve a la pestaña Sonidos para crear una).
          </p>
          <ul v-else class="lista">
            <li
              v-for="s in sonidos.lista"
              :key="s.id"
              class="tarjeta"
              :class="{ activo: s.id === idSonidoActivo }"
            >
              <div class="tarjeta-cabecera">
                <span class="nombre">{{ s.nombre }}</span>
                <span v-if="s.id === idSonidoActivo" class="etiqueta">En uso</span>
              </div>
              <div class="acciones">
                <button
                  v-if="s.id !== idSonidoActivo"
                  type="button"
                  class="btn btn-secondary"
                  @click="usarSonido(s.id)"
                >
                  Usar
                </button>
                <button v-else type="button" class="btn btn-secondary" @click="dejarDeUsarSonido">
                  Dejar de usar
                </button>
              </div>
            </li>
          </ul>
        </fieldset>

        <button type="submit" class="btn btn-primary btn-guardar" :disabled="enfoque.guardando">
          {{ enfoque.guardando ? 'Guardando…' : 'Guardar configuración' }}
        </button>
      </form>
    </template>
  </section>
</template>

<style scoped>
.intro {
  margin-bottom: 1.25rem;
}
.resumen {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  font-weight: 700;
  padding: 0.8rem 1rem;
  border-radius: var(--radius-md);
  background: var(--color-sage-soft);
  color: var(--accent);
  margin-bottom: 1.25rem;
}
.resumen.alarma {
  background: var(--color-danger-bg);
  color: var(--color-danger);
}
.punto {
  width: 0.6rem;
  height: 0.6rem;
  border-radius: 50%;
  background: currentColor;
  flex-shrink: 0;
}
.ciclo {
  text-align: center;
  color: var(--text-muted);
  font-weight: 600;
  margin: 0.5rem 0 0;
}
.acciones-principales {
  display: flex;
  justify-content: center;
  margin: 1.25rem 0 1.75rem;
}
.btn-ancho {
  min-width: 12rem;
}

fieldset {
  margin-bottom: 1.1rem;
  border-radius: var(--radius-md);
}
.ayuda {
  font-size: 0.85rem;
  margin: -0.2rem 0 0.7rem;
}
.vacio {
  color: var(--text-muted);
  font-size: 0.9rem;
}
.campo {
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
  margin: 0.6rem 0;
  font-size: 0.9rem;
  font-weight: 600;
  color: var(--text);
}
.campo input[type='number'],
.campo input[type='range'] {
  font-weight: 400;
}
.par {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 0 0.9rem;
}
@media (max-width: 480px) {
  .par {
    grid-template-columns: 1fr;
  }
}

.lista {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 0.6rem;
}
.tarjeta {
  border: 1.5px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 0.75rem 0.9rem;
  background: var(--bg-card-muted);
}
.tarjeta.activo {
  border-color: var(--accent-strong);
  background: var(--color-sage-soft);
}
.tarjeta-cabecera {
  display: flex;
  align-items: center;
  gap: 0.6rem;
  margin-bottom: 0.5rem;
}
.nombre {
  font-weight: 700;
}
.etiqueta {
  font-size: 0.75rem;
  font-weight: 700;
  color: var(--accent-on);
  background: var(--accent-strong);
  padding: 0.15rem 0.55rem;
  border-radius: 999px;
}
.acciones {
  display: flex;
  gap: 0.5rem;
  flex-wrap: wrap;
}

.btn-guardar {
  width: 100%;
  margin-top: 0.4rem;
}
</style>
