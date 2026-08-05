// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= TAREA: PANTALLA (Core 1) =======================
void iniciarPantalla() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
}

void pantallaBienvenida() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 35);
  tft.println("Cuidin");
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.println("Tu compa de cada dia");
  tft.setCursor(10, 75);
  tft.println("Iniciando sensores...");
  delay(1500);
}

void dibujarLinea(int y, const char* etiqueta, String valor, bool esAlarma) {
  tft.setCursor(2, y);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(etiqueta);
  tft.setCursor(76, y); // corrido de 50 a 76: "Temperatura:" ya no cabria en 50
  tft.setTextColor(esAlarma ? ST77XX_RED : ST77XX_WHITE);
  tft.println(valor);
}

void dibujarPantalla(const Lecturas &copia, PosturaEstado postura, const Umbrales &u) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);

  tft.setCursor(2, 2);
  tft.setTextColor(copia.alarma_activa ? ST77XX_RED : ST77XX_GREEN);
  tft.println(copia.alarma_activa ? "** ALARMA **" : "Estado: OK");

  int y = 16;

  // La distancia ya no es un "problema" de alarma: ahora indica presencia.
  // Verde = persona detectada cerca (< u.dist_presencia_cm); blanco = lejos o
  // sin senal. Nunca se pinta de rojo, porque no dispara la alarma.
  if (u.mostrar_datos & MOSTRAR_PERSONA) {
    bool personaCerca = copia.distancia_cm > 0 && copia.distancia_cm < u.dist_presencia_cm;
    String textoPersona;
    if (copia.distancia_cm < 0) {
      textoPersona = "sin senal";
    } else if (personaCerca) {
      textoPersona = "Detectada (" + String(copia.distancia_cm, 0) + "cm)";
    } else {
      textoPersona = "Lejos (" + String(copia.distancia_cm, 0) + "cm)";
    }
    tft.setCursor(2, y);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Persona:");
    tft.setCursor(76, y);
    tft.setTextColor(personaCerca ? ST77XX_GREEN : ST77XX_WHITE);
    tft.println(textoPersona);
    y += 12;
  }

  if (u.mostrar_datos & MOSTRAR_TEMP) {
    dibujarLinea(y, "Temperatura:", isnan(copia.temperatura) ? "--" : String(copia.temperatura, 1) + " C",
                 !isnan(copia.temperatura) && (copia.temperatura < u.temp_min || copia.temperatura > u.temp_max));
    y += 12;
  }

  if (u.mostrar_datos & MOSTRAR_HUM) {
    dibujarLinea(y, "Humedad:", isnan(copia.humedad) ? "--" : String(copia.humedad, 1) + " %",
                 !isnan(copia.humedad) && (copia.humedad < u.hum_min || copia.humedad > u.hum_max));
    y += 12;
  }

  if (u.mostrar_datos & MOSTRAR_LUZ) {
    dibujarLinea(y, "Luz:", String(copia.luz_adc), copia.luz_adc < u.luz_oscuridad);
    y += 12;
  }

  if (u.mostrar_datos & MOSTRAR_SONIDO) {
    dibujarLinea(y, "Sonido:", String(copia.nivel_sonido), copia.nivel_sonido > u.sonido_alarma);
    y += 12;
  }

  if (u.mostrar_datos & MOSTRAR_POSTURA) {
    String textoPostura;
    switch (postura) {
      case POSTURA_OK:           textoPostura = "OK";         break;
      case POSTURA_MALA:         textoPostura = "MALA";       break;
      case POSTURA_SIN_PERSONA:  textoPostura = "Sin persona"; break;
      default:                   textoPostura = "N/D";        break;
    }
    dibujarLinea(y, "Postura:", textoPostura, postura == POSTURA_MALA);
    y += 14;
  }

  if (copia.alarma_activa) {
    tft.setCursor(2, y);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(copia.mensajeAlarma);
  }
}

#if DEBUG
void imprimirSerialDebug(const Lecturas &c, PosturaEstado postura) {
  const char* txtPost = (postura == POSTURA_OK) ? "OK" :
                        (postura == POSTURA_MALA) ? "MALA" :
                        (postura == POSTURA_SIN_PERSONA) ? "SIN_PERSONA" : "N/D";
  Serial.printf(
    "Dist=%.1fcm | Temp=%.1fC | Hum=%.1f%% | Luz=%d | Sonido=%d | Postura=%s | Alarma=%s | %s\n",
    c.distancia_cm, c.temperatura, c.humedad, c.luz_adc, c.nivel_sonido,
    txtPost, c.alarma_activa ? "SI" : "NO", c.mensajeAlarma);
}
#endif

// ======================= CARAS FELIZ / MOLESTO =======================
// Ya no se rellena un circulo de color de fondo: solo se dibujan los
// rasgos (ojos, boca, cejas) directamente sobre la pantalla en negro.
void dibujarBoca(int cx, int cyBase, int ancho, int alto, bool sonrisa, uint16_t color) {
  const int N = 9;
  int prevX = 0, prevY = 0;
  for (int i = 0; i < N; i++) {
    float t = (float)i / (N - 1);
    float curva = 1.0 - (2 * t - 1) * (2 * t - 1);
    int x = cx - ancho / 2 + (int)(t * ancho);
    int y = sonrisa ? cyBase + (int)(alto * curva) : cyBase - (int)(alto * curva);
    if (i > 0) {
      tft.drawLine(prevX, prevY, x, y, color);
      tft.drawLine(prevX, prevY + 1, x, y + 1, color);
    }
    prevX = x;
    prevY = y;
  }
}

void dibujarCeja(int cxOjo, int cyOjo, int rOjo, bool esIzquierda, uint16_t color) {
  int x1, y1, x2, y2;
  if (esIzquierda) {
    x1 = cxOjo - rOjo * 2; y1 = cyOjo - rOjo * 2;
    x2 = cxOjo + rOjo;     y2 = cyOjo - rOjo * 3;
  } else {
    x1 = cxOjo + rOjo * 2; y1 = cyOjo - rOjo * 2;
    x2 = cxOjo - rOjo;     y2 = cyOjo - rOjo * 3;
  }
  tft.drawLine(x1, y1, x2, y2, color);
  tft.drawLine(x1, y1 + 1, x2, y2 + 1, color);
}

void dibujarCaraFeliz() {
  tft.fillScreen(ST77XX_BLACK);
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;
  int r = min(tft.width(), tft.height()) / 2 - MARGEN_CARA;

  int rOjo = r / 6;
  int cyOjos = cy - r / 4;
  tft.fillCircle(cx - r / 2, cyOjos, rOjo, COLOR_RASGOS_FELIZ);
  tft.fillCircle(cx + r / 2, cyOjos, rOjo, COLOR_RASGOS_FELIZ);

  dibujarBoca(cx, cy + r / 6, r, r / CURVATURA_BOCA_DIVISOR, true, COLOR_RASGOS_FELIZ);
}

void dibujarCaraMolesta() {
  tft.fillScreen(ST77XX_BLACK);
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;
  int r = min(tft.width(), tft.height()) / 2 - MARGEN_CARA;

  int rOjo = r / 6;
  int cyOjos = cy - r / 4;
  tft.fillCircle(cx - r / 2, cyOjos, rOjo, COLOR_RASGOS_MOLESTO);
  tft.fillCircle(cx + r / 2, cyOjos, rOjo, COLOR_RASGOS_MOLESTO);

  dibujarCeja(cx - r / 2, cyOjos, rOjo, true, COLOR_RASGOS_MOLESTO);
  dibujarCeja(cx + r / 2, cyOjos, rOjo, false, COLOR_RASGOS_MOLESTO);

  dibujarBoca(cx, cy + r / 3, r, r / CURVATURA_BOCA_DIVISOR, false, COLOR_RASGOS_MOLESTO);
}

enum VistaPantalla { VISTA_DATOS, VISTA_CARA };

void taskPantalla(void *parametro) {
  VistaPantalla vista = VISTA_DATOS;
  unsigned long momentoCambioVista = millis();
  bool parpadeo = false;

  for (;;) {
    Lecturas copia;
    PosturaEstado postura;
    Umbrales u;

    if (bloquearUmbrales()) {
      u = umbrales;
      liberarUmbrales();
    }

    if (bloquearDatos()) {
      copia = datos;
      postura = posturaActual;
      liberarDatos();

      parpadeo = !parpadeo;
      digitalWrite(LED_FELIZ_PIN, copia.alarma_activa ? LOW : HIGH);
      digitalWrite(LED_MOLESTO_PIN, copia.alarma_activa && parpadeo ? HIGH : LOW);

#if DEBUG
      // Modo debug: SIEMPRE datos en pantalla + volcado por Serial, nunca la cara.
      dibujarPantalla(copia, postura, u);
      imprimirSerialDebug(copia, postura);
#else
      unsigned long ahora = millis();

      if (u.mostrar_datos == 0) {
        // Nada seleccionado para mostrar: se queda solo en la cara.
        if (copia.alarma_activa) dibujarCaraMolesta(); else dibujarCaraFeliz();
      } else {
        unsigned long duracionVistaActual =
            (vista == VISTA_DATOS) ? TIEMPO_VISTA_DATOS_MS : TIEMPO_VISTA_CARA_MS;

        if (ahora - momentoCambioVista >= duracionVistaActual) {
          vista = (vista == VISTA_DATOS) ? VISTA_CARA : VISTA_DATOS;
          momentoCambioVista = ahora;
        }

        if (vista == VISTA_DATOS) {
          dibujarPantalla(copia, postura, u);
        } else if (copia.alarma_activa) {
          dibujarCaraMolesta();
        } else {
          dibujarCaraFeliz();
        }
      }
#endif
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
