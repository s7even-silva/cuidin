<script setup lang="ts">
import { computed, reactive } from 'vue'
import { useBleStore } from '../stores/ble'
import { useSonidosStore } from '../stores/sonidos'
import { useUmbralesStore } from '../stores/umbrales'
import { useRtttlAudioPreview } from '../composables/useRtttlAudioPreview'
import { rtttlEsValido, LIMITE_CARACTERES_RTTTL } from '../lib/rtttl'
import { LIMITE_SONIDOS } from '../lib/bleProtocol'

const ble = useBleStore()
const sonidos = useSonidosStore()
const umbrales = useUmbralesStore()
const preview = useRtttlAudioPreview()

const form = reactive({ nombre: '', rtttl: '' })

const formValido = computed(() => form.nombre.trim().length > 0 && rtttlEsValido(form.rtttl))
const limiteAlcanzado = computed(() => sonidos.lista.length >= LIMITE_SONIDOS)

async function subir() {
  if (!formValido.value) return
  await sonidos.subir(form.nombre.trim(), form.rtttl.trim())
  form.nombre = ''
  form.rtttl = ''
}

function previsualizar(rtttl: string) {
  if (!rtttlEsValido(rtttl)) return
  preview.reproducir(rtttl)
}

async function probarEnCuidin(id: string) {
  await sonidos.probarEnDispositivo(id)
}

async function borrar(id: string) {
  await sonidos.borrar(id)
}

async function usarComoAlarma(id: string) {
  await umbrales.guardar({ sonidoRtttlId: id })
}

async function dejarDeUsar() {
  await umbrales.guardar({ sonidoRtttlId: '' })
}

const idActivo = computed(() => umbrales.actuales?.sonidoRtttlId ?? '')
</script>

<template>
  <section>
    <h1>Sonidos (RTTTL)</h1>

    <div v-if="ble.estado !== 'conectado'" class="aviso aviso-info">
      Conéctate a Cuidín primero para gestionar la biblioteca de sonidos.
    </div>

    <template v-else>
      <p class="intro">
        Melodías personalizadas que suenan cuando se dispara la alarma, en vez del patrón
        generado. Máximo {{ LIMITE_SONIDOS }} melodías, {{ LIMITE_CARACTERES_RTTTL }} caracteres
        cada una.
      </p>

      <fieldset>
        <legend>Nueva melodía</legend>
        <label class="campo">
          <span>Nombre</span>
          <input type="text" v-model="form.nombre" maxlength="40" placeholder="Ej: Urgente" />
        </label>
        <label class="campo">
          <span>RTTTL</span>
          <textarea
            v-model="form.rtttl"
            rows="3"
            :maxlength="LIMITE_CARACTERES_RTTTL"
            placeholder="urgente:d=4,o=5,b=140:c,e,g"
          />
        </label>
        <p v-if="form.rtttl && !rtttlEsValido(form.rtttl)" class="error">
          Formato inválido. Debe ser "nombre:defaults:notas".
        </p>
        <p v-if="limiteAlcanzado" class="error">
          Ya tienes {{ LIMITE_SONIDOS }} melodías guardadas. Borra alguna antes de subir otra.
        </p>
        <div class="acciones">
          <button
            type="button"
            class="btn btn-secondary"
            :disabled="!rtttlEsValido(form.rtttl)"
            @click="previsualizar(form.rtttl)"
          >
            {{ preview.reproduciendo ? 'Reproduciendo…' : 'Escuchar en el navegador' }}
          </button>
          <button
            type="button"
            class="btn btn-primary"
            :disabled="!formValido || limiteAlcanzado || sonidos.procesando"
            @click="subir"
          >
            {{ sonidos.procesando ? 'Guardando…' : 'Guardar en Cuidín' }}
          </button>
        </div>
      </fieldset>

      <fieldset>
        <legend>Biblioteca ({{ sonidos.lista.length }}/{{ LIMITE_SONIDOS }})</legend>
        <p v-if="sonidos.lista.length === 0" class="vacio">Todavía no hay melodías guardadas.</p>
        <ul v-else class="lista">
          <li v-for="s in sonidos.lista" :key="s.id" class="tarjeta" :class="{ activo: s.id === idActivo }">
            <div class="tarjeta-cabecera">
              <span class="nombre">{{ s.nombre }}</span>
              <span v-if="s.id === idActivo" class="etiqueta">En uso</span>
            </div>
            <div class="acciones">
              <button type="button" class="btn btn-secondary" @click="probarEnCuidin(s.id)">
                Probar en Cuidín
              </button>
              <button
                v-if="s.id !== idActivo"
                type="button"
                class="btn btn-secondary"
                @click="usarComoAlarma(s.id)"
              >
                Usar en la alarma
              </button>
              <button v-else type="button" class="btn btn-secondary" @click="dejarDeUsar">
                Dejar de usar
              </button>
              <button type="button" class="btn btn-danger" @click="borrar(s.id)">Borrar</button>
            </div>
          </li>
        </ul>
      </fieldset>
    </template>
  </section>
</template>

<style scoped>
.intro {
  margin-bottom: 1.25rem;
}
fieldset {
  margin-bottom: 1.1rem;
  border-radius: var(--radius-md);
}
.campo {
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
  margin: 0.6rem 0;
  font-size: 0.9rem;
  font-weight: 600;
}
textarea {
  width: 100%;
  font-family: ui-monospace, Menlo, Consolas, monospace;
  font-size: 0.88rem;
  resize: vertical;
}
.acciones {
  display: flex;
  gap: 0.5rem;
  margin-top: 0.6rem;
  flex-wrap: wrap;
}
.vacio {
  color: var(--text-muted);
  font-size: 0.9rem;
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
.error {
  color: var(--color-danger);
  font-size: 0.85rem;
}
</style>
