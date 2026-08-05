// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= BLE: SERVIDOR DE AJUSTES =======================
// El ESP32 es el Peripheral BLE; la pagina web (Web Bluetooth API, hecha
// por tu compañero) es el Central. Dos caracteristicas, ambas en formato
// JSON como texto plano UTF-8:
//
//   UUID_CARACT_LEER  (NOTIFY + READ): el ESP32 publica aqui el JSON con
//                      TODOS los ajustes actuales, cada vez que cambian.
//   UUID_CARACT_ESCRIBIR (WRITE): la pagina web escribe aqui un JSON con
//                      los campos que quiere cambiar (no hace falta mandar
//                      todos los campos, solo los que se estan editando).
//
// Ver el mensaje aparte con el detalle completo del JSON para tu compañero.
#define UUID_SERVICIO            "6d5a1000-0001-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_LEER         "6d5a1000-0002-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_ESCRIBIR     "6d5a1000-0003-4c1a-8b1a-2f6a9c8e1a01"

BLECharacteristic *caractLeer;

String umbralesAJSON() {
  Umbrales u;
  if (bloquearUmbrales()) { u = umbrales; liberarUmbrales(); }

  StaticJsonDocument<384> doc;
  doc["dist"]    = u.dist_presencia_cm;
  doc["tmin"]    = u.temp_min;
  doc["tmax"]    = u.temp_max;
  doc["hmin"]    = u.hum_min;
  doc["hmax"]    = u.hum_max;
  doc["sonido"]  = u.sonido_alarma;
  doc["luz"]     = u.luz_oscuridad;
  doc["umbralp"] = u.umbral_problemas;
  doc["volumen"] = u.volumen;
  doc["activa"]  = u.alarma_habilitada;
  doc["prio"]    = u.prioritarios;
  doc["mostrar"] = u.mostrar_datos;
  doc["patron"]  = u.patron_sonido;

  String salida;
  serializeJson(doc, salida);
  return salida;
}

void notificarAjustesActuales() {
  if (caractLeer == nullptr) return;
  String json = umbralesAJSON();
  caractLeer->setValue((uint8_t*)json.c_str(), json.length());
  caractLeer->notify();
}

class CallbackEscritura : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *caract) override {
    String valor = caract->getValue();
    if (valor.length() == 0) return;

    StaticJsonDocument<384> doc;
    DeserializationError err = deserializeJson(doc, valor);
    if (err) {
      Serial.println("JSON invalido recibido por BLE, se ignora.");
      return;
    }

    if (bloquearUmbrales()) {
      if (doc.containsKey("dist"))    umbrales.dist_presencia_cm = doc["dist"];
      if (doc.containsKey("tmin"))    umbrales.temp_min          = doc["tmin"];
      if (doc.containsKey("tmax"))    umbrales.temp_max          = doc["tmax"];
      if (doc.containsKey("hmin"))    umbrales.hum_min           = doc["hmin"];
      if (doc.containsKey("hmax"))    umbrales.hum_max           = doc["hmax"];
      if (doc.containsKey("sonido"))  umbrales.sonido_alarma     = doc["sonido"];
      if (doc.containsKey("luz"))     umbrales.luz_oscuridad     = doc["luz"];
      if (doc.containsKey("umbralp")) umbrales.umbral_problemas  = doc["umbralp"];
      if (doc.containsKey("volumen")) umbrales.volumen           = doc["volumen"];
      if (doc.containsKey("activa"))  umbrales.alarma_habilitada = doc["activa"];
      if (doc.containsKey("prio"))    umbrales.prioritarios      = doc["prio"];
      if (doc.containsKey("mostrar")) umbrales.mostrar_datos     = doc["mostrar"];
      if (doc.containsKey("patron"))  umbrales.patron_sonido     = doc["patron"];
      liberarUmbrales();
    }
    guardarUmbrales();
    notificarAjustesActuales(); // confirma a la web el estado final guardado
    Serial.println("Ajustes actualizados por BLE.");
  }
};

void iniciarBLE() {
  BLEDevice::init(BLE_NOMBRE_DISPOSITIVO);
  BLEServer *servidor = BLEDevice::createServer();
  BLEService *servicio = servidor->createService(UUID_SERVICIO);

  caractLeer = servicio->createCharacteristic(
      UUID_CARACT_LEER, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  caractLeer->addDescriptor(new BLE2902());
  String jsonInicial = umbralesAJSON();
  caractLeer->setValue((uint8_t*)jsonInicial.c_str(), jsonInicial.length());

  BLECharacteristic *caractEscribir = servicio->createCharacteristic(
      UUID_CARACT_ESCRIBIR, BLECharacteristic::PROPERTY_WRITE);
  caractEscribir->setCallbacks(new CallbackEscritura());

  servicio->start();
  BLEAdvertising *publicidad = BLEDevice::getAdvertising();
  publicidad->addServiceUUID(UUID_SERVICIO);
  publicidad->start();

  Serial.println("BLE listo, anunciandose como '" BLE_NOMBRE_DISPOSITIVO "'.");
}

// Muestra un QR fijo (la URL de GitHub Pages no cambia) para que sea facil
// abrir la pagina desde el celular. No depende de WiFi ni de IP local.
void mostrarQRAjustes() {
  static uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
  static uint8_t bufferTemporal[qrcodegen_BUFFER_LEN_MAX];
  bool ok = qrcodegen_encodeText(URL_PAGINA_AJUSTES, bufferTemporal, qrcode,
                                  qrcodegen_Ecc_LOW,
                                  qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                  qrcodegen_Mask_AUTO, true);
  if (!ok) {
    Serial.println("No se pudo generar el QR de la pagina de ajustes.");
    return;
  }
  int tamanoQR = qrcodegen_getSize(qrcode);

  tft.fillScreen(ST77XX_WHITE);
  int escala = 3;
  int offsetX = (tft.width() - tamanoQR * escala) / 2;
  int offsetY = 8;
  for (int yq = 0; yq < tamanoQR; yq++) {
    for (int xq = 0; xq < tamanoQR; xq++) {
      if (qrcodegen_getModule(qrcode, xq, yq)) {
        tft.fillRect(offsetX + xq * escala, offsetY + yq * escala, escala, escala, ST77XX_BLACK);
      }
    }
  }
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(2, offsetY + tamanoQR * escala + 6);
  tft.println("Bluetooth: " BLE_NOMBRE_DISPOSITIVO);

  delay(5000); // tiempo para escanear antes de pasar a la vista normal
}

