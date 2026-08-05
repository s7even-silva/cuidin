// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= TAREA: CAMARA + IA DE POSTURA (Core 1) =======================
#if ENABLE_CAMERA
void iniciarCamara() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0        = Y2_GPIO_NUM;
  config.pin_d1        = Y3_GPIO_NUM;
  config.pin_d2        = Y4_GPIO_NUM;
  config.pin_d3        = Y5_GPIO_NUM;
  config.pin_d4        = Y6_GPIO_NUM;
  config.pin_d5        = Y7_GPIO_NUM;
  config.pin_d6        = Y8_GPIO_NUM;
  config.pin_d7        = Y9_GPIO_NUM;
  config.pin_xclk      = XCLK_GPIO_NUM;
  config.pin_pclk      = PCLK_GPIO_NUM;
  config.pin_vsync     = VSYNC_GPIO_NUM;
  config.pin_href      = HREF_GPIO_NUM;
  config.pin_sscb_sda  = SIOD_GPIO_NUM;
  config.pin_sscb_scl  = SIOC_GPIO_NUM;
  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset     = RESET_GPIO_NUM;
  config.xclk_freq_hz  = XCLK_FREQ_HZ;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.frame_size    = FRAMESIZE_QVGA;
  config.jpeg_quality  = 12;
  config.fb_count      = 1;
  config.fb_location   = CAMERA_FB_IN_PSRAM;
  config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error iniciando camara: 0x%x\n", err);
    return;
  }
  Serial.println("Camara iniciada OK.");

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL && s->id.PID == OV3660_PID) {
    Serial.println("Sensor OV3660 detectado: aplicando vflip/brillo/saturacion.");
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, 0);
  }
}

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS   320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS   240
#define EI_CAMERA_FRAME_BYTE_SIZE         3

static uint8_t *snapshot_buf = nullptr;

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;
  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) +
                          (snapshot_buf[pixel_ix + 1] << 8) +
                           snapshot_buf[pixel_ix];
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  return 0;
}

static bool ei_camera_capturar_y_preparar(camera_fb_t *fb, uint32_t ancho, uint32_t alto) {
  bool convertido = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
  if (!convertido) return false;

  if (ancho != EI_CAMERA_RAW_FRAME_BUFFER_COLS || alto != EI_CAMERA_RAW_FRAME_BUFFER_ROWS) {
    ei::image::processing::crop_and_interpolate_rgb888(
        snapshot_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        snapshot_buf, ancho, alto);
  }
  return true;
}

PosturaEstado etiquetaAPostura(const char* etiqueta) {
  if (strcmp(etiqueta, "good_posture") == 0)  return POSTURA_OK;
  if (strcmp(etiqueta, "empty_chair") == 0)   return POSTURA_SIN_PERSONA;
  return POSTURA_MALA;
}

// Contadores de diagnostico: se imprimen periodicamente para saber, sin
// ambiguedad, en cual de los 3 pasos se esta atascando la deteccion.
static uint32_t contFbNull = 0;
static uint32_t contConversionFallo = 0;
static uint32_t contClasificadorFallo = 0;
static uint32_t contOk = 0;

PosturaEstado detectarPostura() {
  if (snapshot_buf == nullptr) {
    snapshot_buf = (uint8_t*) heap_caps_malloc(
        EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE,
        MALLOC_CAP_SPIRAM);
    if (snapshot_buf == nullptr) {
      Serial.println("ERROR: no se pudo reservar snapshot_buf en PSRAM");
      return POSTURA_DESCONOCIDA;
    }
    Serial.println("snapshot_buf reservado en PSRAM correctamente.");
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    contFbNull++;
    // Este es el punto que ANTES fallaba en silencio. Ahora se ve por Serial.
    Serial.printf("ERR: esp_camera_fb_get() devolvio NULL (van %u veces)\n", contFbNull);
    return POSTURA_DESCONOCIDA;
  }

  bool ok = ei_camera_capturar_y_preparar(fb, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
  esp_camera_fb_return(fb);

  if (!ok) {
    contConversionFallo++;
    Serial.printf("ERR: fallo la conversion JPEG->RGB888 (van %u veces)\n", contConversionFallo);
    return POSTURA_DESCONOCIDA;
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    contClasificadorFallo++;
    Serial.printf("ERR: run_classifier fallo (%d) (van %u veces)\n", err, contClasificadorFallo);
    Serial.printf("     Heap libre interno: %u bytes | PSRAM libre: %u bytes\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());
    return POSTURA_DESCONOCIDA;
  }

  contOk++;
  int mejorIndice = 0;
  float mejorValor = result.classification[0].value;
  for (uint16_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > mejorValor) {
      mejorValor = result.classification[i].value;
      mejorIndice = i;
    }
  }

  Serial.printf("Postura: %s (%.2f) [ok=%u, fb_null=%u, conv_fallo=%u, clasif_fallo=%u]\n",
                ei_classifier_inferencing_categories[mejorIndice], mejorValor,
                contOk, contFbNull, contConversionFallo, contClasificadorFallo);

  return etiquetaAPostura(ei_classifier_inferencing_categories[mejorIndice]);
}

void taskCamaraIA(void *parametro) {
  for (;;) {
    PosturaEstado resultado = detectarPostura();

    if (bloquearDatos()) {
      posturaActual = resultado;
      liberarDatos();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
#endif

