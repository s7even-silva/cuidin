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
    <p class="intro">
      Cuidín avisa cuando alguno de estos valores sale de rango. Ajusta los umbrales a tu espacio
      y rutina.
    </p>

    <div v-if="ble.estado !== 'conectado'" class="aviso aviso-info">
      Conéctate a Cuidín primero para ver y editar los umbrales actuales.
    </div>

    <form v-else @submit.prevent="guardar">
      <fieldset>
        <legend>General</legend>
        <label class="fila-switch">
          <span>Alarma habilitada</span>
          <input type="checkbox" v-model="form.activa" class="switch" />
        </label>
        <label class="campo">
          <span>Volumen ({{ form.volumen }}%)</span>
          <input type="range" min="0" max="100" v-model.number="form.volumen" />
        </label>
        <label class="campo">
          <span>Problemas simultáneos necesarios (no prioritarios)</span>
          <input type="number" min="1" max="5" v-model.number="form.umbralp" />
        </label>
      </fieldset>

      <fieldset>
        <legend>Temperatura y humedad</legend>
        <div class="par">
          <label class="campo"><span>Temp. mínima (°C)</span><input type="number" step="0.1" v-model.number="form.tmin" /></label>
          <label class="campo"><span>Temp. máxima (°C)</span><input type="number" step="0.1" v-model.number="form.tmax" /></label>
        </div>
        <div class="par">
          <label class="campo"><span>Humedad mínima (%)</span><input type="number" step="0.1" v-model.number="form.hmin" /></label>
          <label class="campo"><span>Humedad máxima (%)</span><input type="number" step="0.1" v-model.number="form.hmax" /></label>
        </div>
      </fieldset>

      <fieldset>
        <legend>Luz, sonido y presencia</legend>
        <div class="par">
          <label class="campo"><span>Umbral de poca luz (0-4095)</span><input type="number" min="0" max="4095" v-model.number="form.luz" /></label>
          <label class="campo"><span>Umbral de sonido fuerte</span><input type="number" min="0" v-model.number="form.sonido" /></label>
        </div>
        <label class="campo">
          <span>Distancia de detección de presencia (cm)</span>
          <input type="number" step="1" v-model.number="form.dist" />
        </label>
      </fieldset>

      <fieldset>
        <legend>Problemas prioritarios</legend>
        <p class="ayuda">Disparan la alarma por sí solos, sin necesitar combinarse con otros.</p>
        <div class="chips">
          <label
            v-for="p in PRIO_BITS"
            :key="p.bit"
            class="chip"
            :class="{ activo: tieneBit(form.prio, p.bit) }"
          >
            <input
              type="checkbox"
              :checked="tieneBit(form.prio, p.bit)"
              @change="alternarBit('prio', p.bit, ($event.target as HTMLInputElement).checked)"
            />
            {{ p.etiqueta }}
          </label>
        </div>
      </fieldset>

      <fieldset>
        <legend>Qué mostrar en la pantalla</legend>
        <div class="chips">
          <label
            v-for="m in MOSTRAR_BITS"
            :key="m.bit"
            class="chip"
            :class="{ activo: tieneBit(form.mostrar, m.bit) }"
          >
            <input
              type="checkbox"
              :checked="tieneBit(form.mostrar, m.bit)"
              @change="alternarBit('mostrar', m.bit, ($event.target as HTMLInputElement).checked)"
            />
            {{ m.etiqueta }}
          </label>
        </div>
      </fieldset>

      <fieldset>
        <legend>Sonido de alarma</legend>
        <label class="campo">
          <span>Patrón (0-3, usado si no hay sonido RTTTL seleccionado en la pestaña Sonidos)</span>
          <input type="number" min="0" max="3" v-model.number="form.patron" />
        </label>
      </fieldset>

      <button type="submit" class="btn btn-primary btn-guardar" :disabled="umbrales.guardando">
        {{ umbrales.guardando ? 'Guardando…' : 'Guardar cambios' }}
      </button>
    </form>
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
.ayuda {
  font-size: 0.85rem;
  margin: -0.2rem 0 0.7rem;
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

.fila-switch {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-weight: 600;
  margin: 0.4rem 0 0.8rem;
  cursor: pointer;
}
.switch {
  width: 2.5rem;
  height: 1.4rem;
  appearance: none;
  background: var(--border);
  border-radius: 999px;
  position: relative;
  cursor: pointer;
  transition: background-color 0.15s ease;
}
.switch::after {
  content: '';
  position: absolute;
  top: 2px;
  left: 2px;
  width: 1.1rem;
  height: 1.1rem;
  background: #fff;
  border-radius: 50%;
  transition: transform 0.15s ease;
  box-shadow: var(--shadow-sm);
}
.switch:checked {
  background: var(--accent-strong);
}
.switch:checked::after {
  transform: translateX(1.1rem);
}

.chips {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}
.chip {
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.85rem;
  font-weight: 600;
  padding: 0.4rem 0.8rem;
  border-radius: 999px;
  border: 1.5px solid var(--border);
  background: var(--bg-card);
  cursor: pointer;
  transition: background-color 0.15s ease, border-color 0.15s ease, color 0.15s ease;
}
.chip input {
  position: absolute;
  opacity: 0;
  pointer-events: none;
}
.chip.activo {
  background: var(--color-terracota-soft);
  border-color: var(--primary-strong);
  color: var(--primary);
}

.btn-guardar {
  width: 100%;
  margin-top: 0.4rem;
}
</style>
