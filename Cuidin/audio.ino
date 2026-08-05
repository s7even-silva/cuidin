// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= SD (sonidos de alarma) =======================
// Pines fijos por hardware en este modulo (SD_MMC 1-bit, no se configuran
// por software): CLK=39, CMD=38, D0=40. No los usa nada mas del proyecto.
// Formato esperado de los archivos: WAV PCM de 16 bits, mono o estereo,
// idealmente a 16000 Hz (el bus I2S esta fijo a esa frecuencia; si el
// archivo esta a otra frecuencia, se reproduce igual pero mas rapido/lento).
// Ruta esperada: /alarmas/0.wav, /alarmas/1.wav, /alarmas/2.wav, /alarmas/3.wav
#include "SD_MMC.h"

bool sdDisponible = false;

void iniciarSD() {
  if (!SD_MMC.begin("/sdcard", true)) { // true = modo de 1 bit
    Serial.println("SD no detectada. Se usaran los patrones de sonido generados.");
    sdDisponible = false;
    return;
  }
  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("SD sin tarjeta insertada. Se usaran los patrones generados.");
    sdDisponible = false;
    return;
  }
  sdDisponible = true;
  Serial.println("SD lista.");
}

bool reproducirWAVDesdeSD(const char* ruta, float volumen) {
  File archivo = SD_MMC.open(ruta);
  if (!archivo) return false;

  uint8_t cabecera[44];
  if (archivo.read(cabecera, 44) != 44 ||
      memcmp(cabecera, "RIFF", 4) != 0 || memcmp(cabecera + 8, "WAVE", 4) != 0) {
    archivo.close();
    return false;
  }
  uint16_t numCanales     = cabecera[22] | (cabecera[23] << 8);
  uint32_t sampleRate     = cabecera[24] | (cabecera[25] << 8) | (cabecera[26] << 16) | ((uint32_t)cabecera[27] << 24);
  uint16_t bitsPorMuestra = cabecera[34] | (cabecera[35] << 8);

  if (bitsPorMuestra != 16) {
    Serial.printf("WAV '%s' no es de 16 bits, se ignora.\n", ruta);
    archivo.close();
    return false;
  }
  if (sampleRate != 16000) {
    Serial.printf("Aviso: '%s' esta a %lu Hz (bus fijo a 16000 Hz).\n", ruta, (unsigned long)sampleRate);
  }

  const int TAM_BLOQUE = 128;
  static int16_t bloqueLeido[TAM_BLOQUE * 2];
  static int32_t bloqueSalida[TAM_BLOQUE * 2];

  while (archivo.available()) {
    size_t bytesAPedir = sizeof(int16_t) * (numCanales == 2 ? TAM_BLOQUE * 2 : TAM_BLOQUE);
    int bytesLeidos = archivo.read((uint8_t*)bloqueLeido, bytesAPedir);
    if (bytesLeidos <= 0) break;
    int muestrasLeidas  = bytesLeidos / sizeof(int16_t);
    int paresGenerados  = (numCanales == 2) ? muestrasLeidas / 2 : muestrasLeidas;

    for (int i = 0; i < paresGenerados; i++) {
      int16_t izq = (numCanales == 2) ? bloqueLeido[i * 2]     : bloqueLeido[i];
      int16_t der = (numCanales == 2) ? bloqueLeido[i * 2 + 1] : bloqueLeido[i];
      bloqueSalida[i * 2]     = ((int32_t)(izq * volumen)) << 16;
      bloqueSalida[i * 2 + 1] = ((int32_t)(der * volumen)) << 16;
    }
    size_t escrito = 0;
    i2s_write(I2S_PORT, bloqueSalida, paresGenerados * 2 * sizeof(int32_t), &escrito, portMAX_DELAY);
  }

  archivo.close();
  return true;
}

// Alternativa generada por software (si no hay SD, o falta el archivo del
// patron elegido). 4 patrones distintos, mismo volumen ajustable.
void reproducirPatronGenerado(uint8_t patron, float volumen) {
  switch (patron) {
    case 1:
      reproducirTono(1200, 100, volumen);
      delay(60);
      reproducirTono(1200, 100, volumen);
      delay(60);
      reproducirTono(1200, 100, volumen);
      break;
    case 2:
      reproducirTono(500, 120, volumen);
      reproducirTono(750, 120, volumen);
      reproducirTono(1000, 120, volumen);
      break;
    case 3:
      reproducirTono(300, 400, volumen);
      break;
    default: // 0: patron original de dos tonos
      reproducirTono(TONO_ALARMA_1_HZ, DURACION_TONO_MS, volumen);
      delay(50);
      reproducirTono(TONO_ALARMA_2_HZ, DURACION_TONO_MS, volumen);
      break;
  }
}

// Punto unico desde el que taskAlarma dispara el sonido. Si se pasa un RTTTL
// no vacio (biblioteca de sonidos.ino), tiene prioridad sobre todo lo demas.
// Si no, primero intenta el archivo correspondiente en la SD; si no existe o
// la SD no esta disponible, cae al patron generado con el mismo indice.
void reproducirAlarmaSonora(uint8_t patron, int volumenPorcentaje, const String &rtttl) {
  float volumen = constrain(volumenPorcentaje, 0, 100) / 100.0f;

  if (rtttl.length() > 0) {
    reproducirRTTTL(rtttl, volumen);
    i2s_zero_dma_buffer(I2S_PORT);
    return;
  }

  if (sdDisponible) {
    char ruta[24];
    snprintf(ruta, sizeof(ruta), "/alarmas/%u.wav", patron);
    if (reproducirWAVDesdeSD(ruta, volumen)) {
      i2s_zero_dma_buffer(I2S_PORT);
      return;
    }
  }
  reproducirPatronGenerado(patron, volumen);
  i2s_zero_dma_buffer(I2S_PORT);
}


// ======================= BUS I2S COMPARTIDO (mic + altavoz) =======================
// Full-duplex: TX y RX comparten UN solo bits_per_sample. El INMP441 necesita
// 32 bits, asi que el altavoz (TX) tambien queda en 32 bits.
void iniciarBusI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false
  };

  i2s_pin_config_t pines = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DOUT_PIN,
    .data_in_num = I2S_SD_PIN
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pines);
  i2s_zero_dma_buffer(I2S_PORT);
}

// ======================= TAREA: AUDIO (Core 0) =======================
// Lee el INMP441 a 32 bits. El dato de 24 bits viene alineado a la izquierda,
// por eso se desplaza a la derecha (>> SHIFT_MIC).
void taskAudio(void *parametro) {
  const int PARES = 256;             // pares L/R por lectura
  static int32_t buffer[PARES * 2];  // 32 bits

  for (;;) {
    size_t bytesLeidos = 0;
    esp_err_t r = i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytesLeidos, pdMS_TO_TICKS(100));
    int paresLeidos = bytesLeidos / (2 * sizeof(int32_t));

    // Solo actualizamos si la lectura fue valida: asi el nivel NO cae a 0
    // por un simple timeout del bus (esto elimina el "salta a 0").
    if (r == ESP_OK && paresLeidos > 0) {
      double suma = 0;
      for (int i = 0; i < paresLeidos; i++) {
        // Canal izquierdo = dato del INMP441 (confirmado con el diagnostico).
        int32_t crudo = buffer[i * 2];
        int32_t muestra = crudo >> SHIFT_MIC;
        suma += (double)muestra * muestra;
      }
      int nivel = (int) sqrt(suma / paresLeidos);

      if (bloquearDatos()) {
        datos.nivel_sonido = nivel;
        liberarDatos();
      }
    }
    // si r != ESP_OK o no hubo pares: dejamos el ultimo nivel, no lo pisamos.

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ======================= ALTAVOZ MAX98357A (usado por TaskAlarma) =======================
// Ahora produce muestras de 32 bits: la muestra de 16 bits se coloca en la
// parte alta de la palabra (<< 16), que es lo que espera el MAX98357A.
// volumen: 0.0 a 1.0 (viene de umbrales.volumen / 100.0).
void reproducirTono(float frecuenciaHz, int duracionMs, float volumen) {
  const int sampleRate = 16000;
  const int totalMuestras = (sampleRate * duracionMs) / 1000;
  const int TAM_BLOQUE = 128;
  int32_t bloque[TAM_BLOQUE * 2]; // intercalado L,R,L,R...

  static float fase = 0;
  int muestrasEscritas = 0;
  while (muestrasEscritas < totalMuestras) {
    int enEsteBloque = min(TAM_BLOQUE, totalMuestras - muestrasEscritas);
    for (int i = 0; i < enEsteBloque; i++) {
      int16_t muestra16 = (int16_t)(sinf(fase) * AMPLITUD_TONO * volumen);
      int32_t muestra32 = ((int32_t)muestra16) << 16; // alinear a 32 bits
      bloque[i * 2]     = muestra32;
      bloque[i * 2 + 1] = muestra32;
      fase += 2.0f * PI * frecuenciaHz / sampleRate;
      if (fase > 2.0f * PI) fase -= 2.0f * PI;
    }
    size_t escrito = 0;
    i2s_write(I2S_PORT, bloque, enEsteBloque * 2 * sizeof(int32_t), &escrito, portMAX_DELAY);
    muestrasEscritas += enEsteBloque;
  }
}

