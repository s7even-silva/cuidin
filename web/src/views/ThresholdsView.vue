<script setup lang="ts">
import { reactive, watch } from 'vue'
import { useBleStore } from '../stores/ble'
import { useUmbralesStore } from '../stores/umbrales'
import { PRIO_BITS, MOSTRAR_BITS, type Umbrales } from '../lib/bleProtocol'

const ble = useBleStore()
const umbrales = useUmbralesStore()

// Copia editable local: se sincroniza con lo recibido de Cuidín, pero el
// usuario edita esta copia y solo se envía al guardar explícitamente.
const form = reactive<Umbrales>({
  dist: 120,
  tmin: 18,
  tmax: 30,
  hmin: 30,
  hmax: 90,
  sonido: 3000,
  luz: 800,
  umbralp: 2,
  volumen: 80,
  activa: true,
  prio: 0,
  mostrar: 0,
  patron: 0,
  sonidoRtttlId: '',
  modoCamara: false,
})

watch(
  () => umbrales.actuales,
  (nuevo) => {
    if (nuevo) Object.assign(form, nuevo)
  },
  { immediate: true },
)

function tieneBit(valor: number, bit: number) {
  return (valor & bit) !== 0
}

function alternarBit(campo: 'prio' | 'mostrar', bit: number, activo: boolean) {
  form[campo] = activo ? form[campo] | bit : form[campo] & ~bit
}

async function guardar() {
  await umbrales.guardar({ ...form })
}
</script>

<template>
  <section>
    <h1>Umbrales de alarma</h1>
    <p v-if="ble.estado !== 'conectado'" class="aviso">
      Conéctate a Cuidín primero para ver y editar los umbrales actuales.
    </p>

    <form v-else @submit.prevent="guardar">
      <fieldset>
        <legend>General</legend>
        <label><input type="checkbox" v-model="form.activa" /> Alarma habilitada</label>
        <label>
          Volumen (%)
          <input type="range" min="0" max="100" v-model.number="form.volumen" />
          {{ form.volumen }}
        </label>
        <label>
          Problemas simultáneos necesarios (no prioritarios)
          <input type="number" min="1" max="5" v-model.number="form.umbralp" />
        </label>
      </fieldset>

      <fieldset>
        <legend>Temperatura (°C)</legend>
        <label>Mínima <input type="number" step="0.1" v-model.number="form.tmin" /></label>
        <label>Máxima <input type="number" step="0.1" v-model.number="form.tmax" /></label>
      </fieldset>

      <fieldset>
        <legend>Humedad (%)</legend>
        <label>Mínima <input type="number" step="0.1" v-model.number="form.hmin" /></label>
        <label>Máxima <input type="number" step="0.1" v-model.number="form.hmax" /></label>
      </fieldset>

      <fieldset>
        <legend>Luz y sonido</legend>
        <label>Umbral de poca luz (0-4095) <input type="number" min="0" max="4095" v-model.number="form.luz" /></label>
        <label>Umbral de sonido fuerte <input type="number" min="0" v-model.number="form.sonido" /></label>
      </fieldset>

      <fieldset>
        <legend>Presencia</legend>
        <label>Distancia de detección (cm) <input type="number" step="1" v-model.number="form.dist" /></label>
      </fieldset>

      <fieldset>
        <legend>Problemas prioritarios (disparan la alarma solos)</legend>
        <label v-for="p in PRIO_BITS" :key="p.bit">
          <input
            type="checkbox"
            :checked="tieneBit(form.prio, p.bit)"
            @change="alternarBit('prio', p.bit, ($event.target as HTMLInputElement).checked)"
          />
          {{ p.etiqueta }}
        </label>
      </fieldset>

      <fieldset>
        <legend>Qué mostrar en la pantalla</legend>
        <label v-for="m in MOSTRAR_BITS" :key="m.bit">
          <input
            type="checkbox"
            :checked="tieneBit(form.mostrar, m.bit)"
            @change="alternarBit('mostrar', m.bit, ($event.target as HTMLInputElement).checked)"
          />
          {{ m.etiqueta }}
        </label>
      </fieldset>

      <fieldset>
        <legend>Sonido de alarma</legend>
        <label>
          Patrón (0-3, usado si no hay sonido RTTTL seleccionado)
          <input type="number" min="0" max="3" v-model.number="form.patron" />
        </label>
      </fieldset>

      <button type="submit" :disabled="umbrales.guardando">
        {{ umbrales.guardando ? 'Guardando…' : 'Guardar cambios' }}
      </button>
    </form>
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
.aviso {
  color: #e0c341;
}
</style>
