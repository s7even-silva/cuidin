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
    <p v-if="ble.estado !== 'conectado'" class="aviso">
      Conéctate a Cuidín primero para gestionar la biblioteca de sonidos.
    </p>

    <template v-else>
      <p>
        Melodías personalizadas que suenan cuando se dispara la alarma, en vez del patrón
        generado. Máximo {{ LIMITE_SONIDOS }} melodías, {{ LIMITE_CARACTERES_RTTTL }} caracteres
        cada una.
      </p>

      <fieldset>
        <legend>Nueva melodía</legend>
        <label>
          Nombre
          <input type="text" v-model="form.nombre" maxlength="40" placeholder="Ej: Urgente" />
        </label>
        <label>
          RTTTL
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
          <button type="button" :disabled="!rtttlEsValido(form.rtttl)" @click="previsualizar(form.rtttl)">
            {{ preview.reproduciendo ? 'Reproduciendo…' : 'Escuchar en el navegador' }}
          </button>
          <button type="button" :disabled="!formValido || limiteAlcanzado || sonidos.procesando" @click="subir">
            {{ sonidos.procesando ? 'Guardando…' : 'Guardar en Cuidín' }}
          </button>
        </div>
      </fieldset>

      <fieldset>
        <legend>Biblioteca ({{ sonidos.lista.length }}/{{ LIMITE_SONIDOS }})</legend>
        <p v-if="sonidos.lista.length === 0">Todavía no hay melodías guardadas.</p>
        <ul v-else class="lista">
          <li v-for="s in sonidos.lista" :key="s.id" :class="{ activo: s.id === idActivo }">
            <span class="nombre">{{ s.nombre }}</span>
            <span v-if="s.id === idActivo" class="etiqueta">En uso para la alarma</span>
            <div class="acciones">
              <button type="button" @click="probarEnCuidin(s.id)">Probar en Cuidín</button>
              <button v-if="s.id !== idActivo" type="button" @click="usarComoAlarma(s.id)">
                Usar en la alarma
              </button>
              <button v-else type="button" @click="dejarDeUsar">Dejar de usar</button>
              <button type="button" class="borrar" @click="borrar(s.id)">Borrar</button>
            </div>
          </li>
        </ul>
      </fieldset>
    </template>
  </section>
</template>

<style scoped>
fieldset {
  margin-bottom: 1rem;
  border-radius: 8px;
}
label {
  display: block;
  margin: 0.4rem 0;
}
textarea {
  width: 100%;
  font-family: monospace;
}
.acciones {
  display: flex;
  gap: 0.5rem;
  margin-top: 0.5rem;
  flex-wrap: wrap;
}
.lista {
  list-style: none;
  padding: 0;
  margin: 0;
}
.lista li {
  padding: 0.6rem 0;
  border-bottom: 1px solid #333;
}
.lista li.activo {
  color: #3ddc84;
}
.nombre {
  font-weight: 600;
  margin-right: 0.5rem;
}
.etiqueta {
  font-size: 0.8rem;
  opacity: 0.8;
}
.borrar {
  color: #e07a7a;
}
.error {
  color: #e07a7a;
  font-size: 0.85rem;
}
.aviso {
  color: #e0c341;
}
</style>
