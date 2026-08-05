// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= TAREA: SENSORES (Core 0) =======================
void taskSensores(void *parametro) {
  unsigned long ultimoDHT = 0;

  for (;;) {
    float distancia = medirDistanciaCm();
    int luz = analogRead(LDR_PIN);

    bool hayLecturaDHT = false;
    float temp = NAN, hum = NAN;
    if (millis() - ultimoDHT >= INTERVALO_DHT_MS) {
      hum = dht.readHumidity();
      temp = dht.readTemperature();
      hayLecturaDHT = !isnan(hum) && !isnan(temp);
      ultimoDHT = millis();
    }

    if (bloquearDatos()) {
      datos.distancia_cm = distancia;
      datos.luz_adc = luz;
      if (hayLecturaDHT) {
        datos.temperatura = temp;
        datos.humedad = hum;
      }
      liberarDatos();
    }

    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

// Medicion del HC-SR04 por sondeo (sin interrupciones). Usa pulseInLong,
// que se apoya en micros() y no desactiva interrupciones: mas estable que
// pulseIn en el ESP32 con FreeRTOS. Promedia hasta 3 disparos validos.
float medirDistanciaCm() {
  float lecturas[3];
  int validas = 0;
  for (int k = 0; k < 3; k++) {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    unsigned long duracion = pulseInLong(ECHO_PIN, HIGH, 30000);
    if (duracion > 0) lecturas[validas++] = duracion * 0.0343 / 2.0;
    delay(10); // el HC-SR04 necesita ~10ms entre disparos
  }
  if (validas == 0) return -1;
  float suma = 0;
  for (int i = 0; i < validas; i++) suma += lecturas[i];
  return suma / validas;
}

