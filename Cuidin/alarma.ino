// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= TAREA: ALARMA (Core 0, prioridad mas alta) =======================
// "prioridad" ahora es un bit del bitmask u.prioritarios (ver PRIO_* arriba),
// editable desde la web - por defecto Poca luz y Postura mala, igual que antes.
struct Problema {
  bool activo;
  uint8_t bitPrioridad; // que bit de u.prioritarios corresponde a este problema
  const char* texto;
};

void taskAlarma(void *parametro) {
  for (;;) {
    Lecturas copia;
    PosturaEstado postura;
    Umbrales u;

    if (bloquearDatos()) {
      copia = datos;
      postura = posturaActual;
      liberarDatos();
    } else {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    if (bloquearUmbrales()) {
      u = umbrales;
      liberarUmbrales();
    }
    // si no se pudo tomar el mutex, usamos los valores de "u" que traiga
    // por defecto el struct (razonable, y evita bloquear esta tarea).

    // Interruptor general: si la alarma esta desactivada desde la web, no
    // se evalua nada y el sonido nunca suena (pero los datos en vivo por
    // BLE siguen actualizandose igual).
    if (!u.alarma_habilitada) {
      if (bloquearDatos()) {
        datos.alarma_activa = false;
        datos.mensajeAlarma[0] = '\0';
        liberarDatos();
      }
      notificarEstadoActual();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // La distancia NO esta aqui: solo se usa para detectar presencia
    // (ver dibujarPantalla), nunca dispara la alarma.
    Problema problemas[] = {
      { !isnan(copia.temperatura) && (copia.temperatura < u.temp_min || copia.temperatura > u.temp_max),
                                                                        PRIO_TEMP,    "Temp. fuera de rango" },
      { !isnan(copia.humedad) && (copia.humedad < u.hum_min || copia.humedad > u.hum_max),
                                                                        PRIO_HUM,     "Humedad fuera de rango" },
      { copia.nivel_sonido > u.sonido_alarma,                          PRIO_SONIDO,  "Sonido fuerte" },
      { copia.luz_adc < u.luz_oscuridad,                               PRIO_LUZ,     "Poca luz" },
      { postura == POSTURA_MALA,                                       PRIO_POSTURA, "Corrige tu postura" },
    };
    const int TOTAL_PROBLEMAS = sizeof(problemas) / sizeof(problemas[0]);

    int activos = 0;
    bool prioritarioActivo = false;
    char mensaje[MSG_LEN] = "";
    for (int i = 0; i < TOTAL_PROBLEMAS; i++) {
      if (!problemas[i].activo) continue;
      activos++;
      if (u.prioritarios & problemas[i].bitPrioridad) prioritarioActivo = true;
      if (mensaje[0] != '\0') {
        strncat(mensaje, " + ", MSG_LEN - strlen(mensaje) - 1);
      }
      strncat(mensaje, problemas[i].texto, MSG_LEN - strlen(mensaje) - 1);
    }

    // Los problemas marcados como prioritarios (bitmask u.prioritarios)
    // disparan la alarma solos. El resto sigue necesitando combinarse
    // (>= u.umbral_problemas).
    bool alarma = prioritarioActivo || (activos >= u.umbral_problemas);

    if (bloquearDatos()) {
      datos.alarma_activa = alarma;
      strncpy(datos.mensajeAlarma, alarma ? mensaje : "", MSG_LEN - 1);
      datos.mensajeAlarma[MSG_LEN - 1] = '\0';
      liberarDatos();
    }

    if (alarma) {
      reproducirAlarmaSonora(u.patron_sonido, u.volumen);
    }

    notificarEstadoActual(); // datos en vivo por BLE (caracteristica 0004)

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
