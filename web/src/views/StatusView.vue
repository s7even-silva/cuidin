<script setup lang="ts">
import { computed } from 'vue'
import { useBleStore } from '../stores/ble'
import { useEstadoStore } from '../stores/estado'
import { useUmbralesStore } from '../stores/umbrales'

const ble = useBleStore()
const estado = useEstadoStore()
const umbrales = useUmbralesStore()

const textoPostura: Record<string, string> = {
  ok: 'OK',
  mala: 'Mala',
  sin_persona: 'Sin persona',
  desconocida: 'Desconocida',
}

interface Tile {
  etiqueta: string
  valor: string
  fueraDeRango: boolean
}

const tiles = computed<Tile[]>(() => {
  const e = estado.actual
  const u = umbrales.actuales
  if (!e) return []

  return [
    {
      etiqueta: 'Temperatura',
      valor: e.temperatura != null ? e.temperatura.toFixed(1) + ' °C' : '—',
      fueraDeRango: !!u && e.temperatura != null && (e.temperatura < u.tmin || e.temperatura > u.tmax),
    },
    {
      etiqueta: 'Humedad',
      valor: e.humedad != null ? e.humedad.toFixed(1) + ' %' : '—',
      fueraDeRango: !!u && e.humedad != null && (e.humedad < u.hmin || e.humedad > u.hmax),
    },
    {
      etiqueta: 'Luz (ADC)',
      valor: String(e.luz),
      fueraDeRango: !!u && e.luz < u.luz,
    },
    {
      etiqueta: 'Sonido',
      valor: String(e.sonido),
      fueraDeRango: !!u && e.sonido > u.sonido,
    },
    {
      etiqueta: 'Presencia',
      valor: e.distancia >= 0 ? e.distancia.toFixed(0) + ' cm' : 'sin señal',
      fueraDeRango: false,
    },
    {
      etiqueta: 'Postura',
      valor: textoPostura[e.postura] ?? e.postura,
      fueraDeRango: e.postura === 'mala',
    },
  ]
})
</script>

<template>
  <section>
    <h1>Estado en vivo</h1>

    <div v-if="ble.estado !== 'conectado'" class="aviso aviso-info">
      Conéctate a Cuidín primero para ver las lecturas en vivo de los sensores.
    </div>
    <p v-else-if="!estado.actual">Esperando la primera lectura…</p>

    <template v-else>
      <div class="resumen" :class="{ alarma: estado.actual.alarma }">
        <span class="punto" />
        <span>{{ estado.actual.alarma ? estado.actual.mensaje || 'Alarma activa' : 'Todo en orden' }}</span>
      </div>

      <div class="grilla">
        <div v-for="t in tiles" :key="t.etiqueta" class="tile" :class="{ alerta: t.fueraDeRango }">
          <span class="tile-etiqueta">{{ t.etiqueta }}</span>
          <span class="tile-valor">{{ t.valor }}</span>
        </div>
      </div>
    </template>
  </section>
</template>

<style scoped>
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

.grilla {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 0.75rem;
}

.tile {
  display: flex;
  flex-direction: column;
  gap: 0.3rem;
  padding: 0.9rem 1rem;
  border-radius: var(--radius-md);
  background: var(--bg-card);
  border: 1.5px solid var(--border);
}
.tile-etiqueta {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--text-muted);
}
.tile-valor {
  font-family: var(--font-heading);
  font-size: 1.4rem;
  font-weight: 700;
  color: var(--text);
}
.tile.alerta {
  border-color: var(--color-danger);
  background: var(--color-danger-bg);
}
.tile.alerta .tile-valor {
  color: var(--color-danger);
}
</style>
