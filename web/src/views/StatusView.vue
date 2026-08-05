<script setup lang="ts">
import { computed } from 'vue'
import { useBleStore } from '../stores/ble'
import { useEstadoStore } from '../stores/estado'

const ble = useBleStore()
const estado = useEstadoStore()

const textoPostura: Record<string, string> = {
  ok: 'OK',
  mala: 'Mala',
  sin_persona: 'Sin persona',
  desconocida: 'Desconocida',
}

const formateado = computed(() => estado.actual)
</script>

<template>
  <section>
    <h1>Estado en vivo</h1>
    <p v-if="ble.estado !== 'conectado'" class="aviso">
      Conéctate a Cuidín primero para ver las lecturas en vivo de los sensores.
    </p>
    <p v-else-if="!formateado">Esperando la primera lectura…</p>

    <dl v-else class="datos">
      <dt>Estado</dt>
      <dd :class="{ alarma: formateado.alarma }">
        {{ formateado.alarma ? 'ALARMA — ' + formateado.mensaje : 'OK' }}
      </dd>

      <dt>Distancia</dt>
      <dd>{{ formateado.distancia >= 0 ? formateado.distancia.toFixed(0) + ' cm' : 'sin señal' }}</dd>

      <dt>Temperatura</dt>
      <dd>{{ formateado.temperatura != null ? formateado.temperatura.toFixed(1) + ' °C' : '—' }}</dd>

      <dt>Humedad</dt>
      <dd>{{ formateado.humedad != null ? formateado.humedad.toFixed(1) + ' %' : '—' }}</dd>

      <dt>Luz (ADC)</dt>
      <dd>{{ formateado.luz }}</dd>

      <dt>Sonido</dt>
      <dd>{{ formateado.sonido }}</dd>

      <dt>Postura</dt>
      <dd>{{ textoPostura[formateado.postura] ?? formateado.postura }}</dd>
    </dl>
  </section>
</template>

<style scoped>
.datos {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 0.4rem 1rem;
}
.datos dt {
  opacity: 0.7;
}
.datos dd {
  margin: 0;
}
.alarma {
  color: #e07a7a;
  font-weight: 600;
}
.aviso {
  color: #e0c341;
}
</style>
