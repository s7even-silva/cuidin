// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= BLE: SERVIDOR DE AJUSTES =======================
// El ESP32 es el Peripheral BLE; la pagina web (Web Bluetooth API) es el
// Central. Cuatro caracteristicas, todas en formato JSON como texto plano
// UTF-8 (protocolo completo documentado en docs/protocol.md del repo):
//
//   UUID_CARACT_LEER     (NOTIFY + READ): el ESP32 publica aqui el JSON con
//                         TODOS los ajustes actuales, cada vez que cambian.
//   UUID_CARACT_ESCRIBIR (WRITE): la pagina web escribe aqui un JSON con
//                         los campos que quiere cambiar (no hace falta mandar
//                         todos los campos, solo los que se estan editando).
//   UUID_CARACT_SONIDOS  (NOTIFY + READ + WRITE): indice de la biblioteca de
//                         sonidos RTTTL (lectura) y comandos subir/borrar/
//                         probar (escritura). Ver sonidos.ino.
//   UUID_CARACT_ESTADO   (NOTIFY + READ): datos en vivo de los sensores
//                         (no ajustes) - se actualiza automaticamente cada
//                         ~200ms mientras haya alguien conectado.
#define UUID_SERVICIO            "6d5a1000-0001-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_LEER         "6d5a1000-0002-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_ESCRIBIR     "6d5a1000-0003-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_SONIDOS      "6d5a1000-0004-4c1a-8b1a-2f6a9c8e1a01"
#define UUID_CARACT_ESTADO       "6d5a1000-0005-4c1a-8b1a-2f6a9c8e1a01"

BLECharacteristic *caractLeer;
BLECharacteristic *caractSonidos;
BLECharacteristic *caractEstado;

// Datos en vivo (no ajustes) para mostrar en la pagina web: lo mismo que
// ya se ve en la pantalla del dispositivo. postura viene como texto para
// que la web no tenga que conocer el enum interno.
String estadoAJSON() {
  Lecturas d;
  PosturaEstado p;
  if (bloquearDatos()) {
    d = datos;
    p = posturaActual;
    liberarDatos();
  }

  const char* textoPostura;
  switch (p) {
    case POSTURA_OK:          textoPostura = "ok";           break;
    case POSTURA_MALA:        textoPostura = "mala";         break;
    case POSTURA_SIN_PERSONA: textoPostura = "sin_persona";  break;
    default:                  textoPostura = "desconocida";  break;
  }

  StaticJsonDocument<256> doc;
  doc["distancia"] = d.distancia_cm;
  doc["luz"]        = d.luz_adc;
  doc["temperatura"] = d.temperatura;
  doc["humedad"]     = d.humedad;
  doc["sonido"]      = d.nivel_sonido;
  doc["postura"]     = textoPostura;
  doc["alarma"]      = d.alarma_activa;
  doc["mensaje"]     = d.mensajeAlarma;

  String salida;
  serializeJson(doc, salida);
  return salida;
}

void notificarEstadoActual() {
  if (caractEstado == nullptr) return;
  String json = estadoAJSON();
  caractEstado->setValue((uint8_t*)json.c_str(), json.length());
  caractEstado->notify();
}

String umbralesAJSON() {
  Umbrales u;
  if (bloquearUmbrales()) { u = umbrales; liberarUmbrales(); }

  StaticJsonDocument<512> doc;
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
  doc["sonidoRtttlId"] = u.sonido_rtttl_id;
  doc["modoCamara"]    = u.modo_camara;

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

    StaticJsonDocument<512> doc;
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
      if (doc.containsKey("sonidoRtttlId")) umbrales.sonido_rtttl_id = doc["sonidoRtttlId"].as<String>();
      if (doc.containsKey("modoCamara"))    umbrales.modo_camara     = doc["modoCamara"];
      liberarUmbrales();
    }
    guardarUmbrales();
    notificarAjustesActuales(); // confirma a la web el estado final guardado
    Serial.println("Ajustes actualizados por BLE.");
  }
};

// ======================= CARACT_SONIDOS: biblioteca RTTTL =======================
void notificarSonidos() {
  if (caractSonidos == nullptr) return;
  String json = sonidosAJSON();
  caractSonidos->setValue((uint8_t*)json.c_str(), json.length());
  caractSonidos->notify();
}

class CallbackSonidos : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *caract) override {
    String valor = caract->getValue();
    if (valor.length() == 0) return;

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, valor);
    if (err) {
      Serial.println("JSON invalido recibido en CARACT_SONIDOS, se ignora.");
      return;
    }

    String accion = doc["accion"] | "";

    if (accion == "subir") {
      String nombre = doc["nombre"] | "";
      String rtttl  = doc["rtttl"]  | "";
      subirSonido(nombre, rtttl);
      notificarSonidos();
    } else if (accion == "borrar") {
      String id = doc["id"] | "";
      if (borrarSonido(id)) {
        notificarSonidos();
        notificarAjustesActuales(); // por si el borrado limpio sonido_rtttl_id activo
      }
    } else if (accion == "probar") {
      String id = doc["id"] | "";
      String rtttl = buscarRTTTLPorId(id);
      if (rtttl.length() > 0) {
        int volumenActual = 80;
        if (bloquearUmbrales()) { volumenActual = umbrales.volumen; liberarUmbrales(); }
        reproducirRTTTL(rtttl, constrain(volumenActual, 0, 100) / 100.0f);
      }
    } else {
      Serial.println("Accion desconocida en CARACT_SONIDOS, se ignora.");
    }
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

  caractSonidos = servicio->createCharacteristic(
      UUID_CARACT_SONIDOS,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  caractSonidos->addDescriptor(new BLE2902());
  caractSonidos->setCallbacks(new CallbackSonidos());
  String sonidosInicial = sonidosAJSON();
  caractSonidos->setValue((uint8_t*)sonidosInicial.c_str(), sonidosInicial.length());

  caractEstado = servicio->createCharacteristic(
      UUID_CARACT_ESTADO, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  caractEstado->addDescriptor(new BLE2902());
  String estadoInicial = estadoAJSON();
  caractEstado->setValue((uint8_t*)estadoInicial.c_str(), estadoInicial.length());

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
