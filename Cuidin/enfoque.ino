// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= MODO ENFOQUE =======================
// Temporizador tipo Pomodoro configurable, independiente del sistema de
// alarmas de umbrales (taskAlarma sigue funcionando en paralelo sin
// cambios). Ver ConfigEnfoque/EstadoEnfoqueLive en cuidinQR.ino.

// ----- Persistencia NVS (namespace propio, separado de "cuidin") -----
void cargarConfigEnfoque() {
  Preferences p;
  p.begin("enfoque", true); // solo lectura
  enfoqueConfig.duracion_sesion_seg          = p.getUShort("sesion",    enfoqueConfig.duracion_sesion_seg);
  enfoqueConfig.duracion_descanso_corto_seg  = p.getUShort("descCorto", enfoqueConfig.duracion_descanso_corto_seg);
  enfoqueConfig.duracion_descanso_largo_seg  = p.getUShort("descLargo", enfoqueConfig.duracion_descanso_largo_seg);
  enfoqueConfig.sesiones_para_descanso_largo = p.getUChar("nSesiones",  enfoqueConfig.sesiones_para_descanso_largo);
  enfoqueConfig.debounce_presencia_seg       = p.getUChar("debounce",   enfoqueConfig.debounce_presencia_seg);
  enfoqueConfig.aviso_hito1_pct              = p.getUChar("hito1",      enfoqueConfig.aviso_hito1_pct);
  enfoqueConfig.aviso_hito2_pct              = p.getUChar("hito2",      enfoqueConfig.aviso_hito2_pct);
  enfoqueConfig.volumen                      = p.getUChar("volumen",    enfoqueConfig.volumen);
  enfoqueConfig.sonido_rtttl_id              = p.getString("rtttlId",   enfoqueConfig.sonido_rtttl_id);
  p.end();
}

void guardarConfigEnfoque() {
  Preferences p;
  p.begin("enfoque", false); // lectura/escritura
  p.putUShort("sesion",    enfoqueConfig.duracion_sesion_seg);
  p.putUShort("descCorto", enfoqueConfig.duracion_descanso_corto_seg);
  p.putUShort("descLargo", enfoqueConfig.duracion_descanso_largo_seg);
  p.putUChar("nSesiones",  enfoqueConfig.sesiones_para_descanso_largo);
  p.putUChar("debounce",   enfoqueConfig.debounce_presencia_seg);
  p.putUChar("hito1",      enfoqueConfig.aviso_hito1_pct);
  p.putUChar("hito2",      enfoqueConfig.aviso_hito2_pct);
  p.putUChar("volumen",    enfoqueConfig.volumen);
  p.putString("rtttlId",   enfoqueConfig.sonido_rtttl_id);
  p.end();
}

// ----- Heuristica de presencia combinada -----
// Presente si distancia O IA dicen presente; ausente solo si ambas
// coinciden en que no hay nadie. Ver justificacion en el plan: cada
// sensor puede fallar solo (angulo/material para el HC-SR04, luz/oclusion
// para la camara), asi que basta una senal positiva para no pausar.
bool evaluarPresenciaInstantanea(float distancia_cm, PosturaEstado postura) {
  bool distanciaDicePresente = (distancia_cm > 0 && distancia_cm < umbrales.dist_presencia_cm);
  bool iaDicePresente = (postura != POSTURA_SIN_PERSONA);
  return distanciaDicePresente || iaDicePresente;
}

// ----- Sonido de aviso de fin de etapa (sesion<->descanso) -----
// Si hay un RTTTL propio configurado, se usa (misma biblioteca que ya usa
// taskAlarma). Si no, cae a un patron simple y distintivo, distinto de los
// 4 patrones de "alarma" de audio.ino (esto es un cambio de fase, no un
// problema).
void reproducirFinEtapaEnfoque(const ConfigEnfoque &cfg) {
  float volumen = constrain(cfg.volumen, 0, 100) / 100.0f;
  String rtttl = buscarRTTTLPorId(cfg.sonido_rtttl_id);
  if (rtttl.length() > 0) {
    reproducirRTTTL(rtttl, volumen);
  } else {
    reproducirTono(600.0f, 150, volumen);
    delay(50);
    reproducirTono(900.0f, 150, volumen);
  }
}

// ======================= TAREA: MODO ENFOQUE (Core 0, prioridad mas baja) =======================
void taskEnfoque(void *parametro) {
  int segundosPresenciaDistinta = 0;
  bool presenciaEstable = true; // arranca asumiendo que hay alguien presente

  for (;;) {
    float distancia;
    PosturaEstado postura;
    if (bloquearDatos()) {
      distancia = datos.distancia_cm;
      postura = posturaActual;
      liberarDatos();
    } else {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    ConfigEnfoque cfg;
    if (bloquearEnfoque()) {
      cfg = enfoqueConfig;
      liberarEnfoque();
    }

    bool presenciaInstantanea = evaluarPresenciaInstantanea(distancia, postura);
    if (presenciaInstantanea != presenciaEstable) {
      segundosPresenciaDistinta++;
      if (segundosPresenciaDistinta >= cfg.debounce_presencia_seg) {
        presenciaEstable = presenciaInstantanea;
        segundosPresenciaDistinta = 0;
      }
    } else {
      segundosPresenciaDistinta = 0;
    }

    bool dispararAviso80 = false, dispararAviso90 = false, dispararFinEtapa = false;

    if (bloquearEnfoque()) {
      EstadoEnfoqueLive &e = enfoqueEstado;

      switch (e.estado) {
        case ENFOQUE_EN_SESION:
          if (!presenciaEstable) {
            e.estado = ENFOQUE_PAUSADO_AUSENCIA;
          } else {
            e.segundos_transcurridos++;
            uint16_t pct = (uint16_t)((uint32_t)e.segundos_transcurridos * 100 / cfg.duracion_sesion_seg);
            if (!e.aviso80_emitido && pct >= cfg.aviso_hito1_pct) {
              e.aviso80_emitido = true;
              dispararAviso80 = true;
            }
            if (!e.aviso90_emitido && pct >= cfg.aviso_hito2_pct) {
              e.aviso90_emitido = true;
              dispararAviso90 = true;
            }
            if (e.segundos_transcurridos >= cfg.duracion_sesion_seg) {
              e.ciclos_completados++;
              bool descansoLargo = (e.ciclos_completados % cfg.sesiones_para_descanso_largo) == 0;
              e.estado = descansoLargo ? ENFOQUE_DESCANSO_LARGO : ENFOQUE_DESCANSO_CORTO;
              e.segundos_transcurridos = 0;
              e.aviso80_emitido = false;
              e.aviso90_emitido = false;
              dispararFinEtapa = true;
            }
          }
          break;

        case ENFOQUE_PAUSADO_AUSENCIA:
          if (presenciaEstable) {
            e.estado = ENFOQUE_EN_SESION; // segundos_transcurridos se conserva, no se descuenta
          }
          break;

        case ENFOQUE_DESCANSO_CORTO:
        case ENFOQUE_DESCANSO_LARGO: {
          e.segundos_transcurridos++;
          uint16_t duracion = (e.estado == ENFOQUE_DESCANSO_CORTO)
                                   ? cfg.duracion_descanso_corto_seg
                                   : cfg.duracion_descanso_largo_seg;
          if (e.segundos_transcurridos >= duracion) {
            e.estado = ENFOQUE_EN_SESION;
            e.segundos_transcurridos = 0;
            e.aviso80_emitido = false;
            e.aviso90_emitido = false;
            dispararFinEtapa = true;
          }
          break;
        }

        case ENFOQUE_INACTIVO:
        default:
          break; // no hace nada; solo sale por comando BLE
      }
      liberarEnfoque();
    }

    // Llamadas de audio FUERA del mutex (bloqueantes, no deben retenerlo).
    // iniciarReproduccion(SONIDO_ENFOQUE) evita que debeSeguirReproduciendo()
    // (audio.ino) las corte por depender de datos.alarma_activa, que no
    // tiene relacion con los avisos del Modo Enfoque.
    if (dispararAviso80 || dispararAviso90) {
      iniciarReproduccion(SONIDO_ENFOQUE);
      reproducirTono(1000.0f, 120, constrain(cfg.volumen, 0, 100) / 100.0f);
      terminarReproduccion();
    }
    if (dispararFinEtapa) {
      iniciarReproduccion(SONIDO_ENFOQUE);
      reproducirFinEtapaEnfoque(cfg);
      terminarReproduccion();
    }

    notificarEstadoEnfoque();

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
