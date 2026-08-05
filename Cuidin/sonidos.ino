// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= BIBLIOTECA DE SONIDOS RTTTL =======================
// Coleccion de melodias RTTTL definidas por el usuario desde la web (BLE),
// persistidas en NVS (Preferences, namespace "sonidos"). Cada melodia es un
// par (nombre, rtttl) guardado bajo claves "n_<id>" / "r_<id>"; el indice de
// ids existentes se guarda por separado (Preferences no permite iterar
// claves dinamicamente).
#define LIMITE_SONIDOS 8

// struct SonidoMeta declarado en cuidinQR.ino (antes de los #include, ver
// el comentario de PosturaEstado ahi) por el mismo motivo: el generador
// automatico de prototipos de Arduino IDE necesita el tipo ya definido.

// Devuelve la lista de ids guardados (puede estar vacia). Se guarda como un
// solo string con ids separados por coma, ej: "s1,s2,s3".
static String leerIndiceSonidos(Preferences &p) {
  return p.getString("indice", "");
}

static void escribirIndiceSonidos(Preferences &p, const String &indice) {
  p.putString("indice", indice);
}

static int contarIds(const String &indice) {
  if (indice.length() == 0) return 0;
  int total = 1;
  for (unsigned int i = 0; i < indice.length(); i++) {
    if (indice[i] == ',') total++;
  }
  return total;
}

static bool indiceContiene(const String &indice, const String &id) {
  int inicio = 0;
  while (inicio < (int)indice.length()) {
    int coma = indice.indexOf(',', inicio);
    if (coma < 0) coma = indice.length();
    if (indice.substring(inicio, coma) == id) return true;
    inicio = coma + 1;
  }
  return false;
}

static String indiceSinId(const String &indice, const String &id) {
  String resultado = "";
  int inicio = 0;
  while (inicio < (int)indice.length()) {
    int coma = indice.indexOf(',', inicio);
    if (coma < 0) coma = indice.length();
    String actual = indice.substring(inicio, coma);
    if (actual != id && actual.length() > 0) {
      if (resultado.length() > 0) resultado += ",";
      resultado += actual;
    }
    inicio = coma + 1;
  }
  return resultado;
}

// Lista los metadatos (id + nombre) de todas las melodias guardadas, sin
// cargar el RTTTL completo de cada una (se mantiene el payload BLE pequeno).
int listarSonidos(SonidoMeta *destino, int capacidadMaxima) {
  Preferences p;
  p.begin("sonidos", true);
  String indice = leerIndiceSonidos(p);

  int total = 0;
  int inicio = 0;
  while (inicio < (int)indice.length() && total < capacidadMaxima) {
    int coma = indice.indexOf(',', inicio);
    if (coma < 0) coma = indice.length();
    String id = indice.substring(inicio, coma);
    inicio = coma + 1;
    if (id.length() == 0) continue;

    destino[total].id = id;
    destino[total].nombre = p.getString(("n_" + id).c_str(), "");
    total++;
  }
  p.end();
  return total;
}

// Devuelve el RTTTL guardado para ese id, o "" si no existe o el id viene
// vacio (caso normal: sonido_rtttl_id == "" significa "usar patron_sonido").
String buscarRTTTLPorId(const String &id) {
  if (id.length() == 0) return "";
  Preferences p;
  p.begin("sonidos", true);
  String rtttl = p.getString(("r_" + id).c_str(), "");
  p.end();
  return rtttl;
}

static String siguienteId(Preferences &p) {
  uint32_t contador = p.getUInt("contador", 0) + 1;
  p.putUInt("contador", contador);
  return "s" + String(contador);
}

// Crea una melodia nueva. Devuelve el id asignado, o "" si se rechazo por
// exceder el limite de cantidad o de longitud de RTTTL.
String subirSonido(const String &nombre, const String &rtttl) {
  if (!rtttlEsValido(rtttl)) {
    Serial.println("RTTTL invalido, no se guarda.");
    return "";
  }

  Preferences p;
  p.begin("sonidos", false);
  String indice = leerIndiceSonidos(p);

  if (contarIds(indice) >= LIMITE_SONIDOS) {
    Serial.println("Limite de sonidos alcanzado, no se guarda.");
    p.end();
    return "";
  }

  String id = siguienteId(p);
  p.putString(("n_" + id).c_str(), nombre);
  p.putString(("r_" + id).c_str(), rtttl);

  String nuevoIndice = indice.length() > 0 ? indice + "," + id : id;
  escribirIndiceSonidos(p, nuevoIndice);
  p.end();

  Serial.printf("Sonido '%s' guardado con id %s.\n", nombre.c_str(), id.c_str());
  return id;
}

// Borra una melodia por id. Si coincidia con el sonido activo en Umbrales,
// lo limpia para volver a usar el patron generado/WAV.
bool borrarSonido(const String &id) {
  Preferences p;
  p.begin("sonidos", false);
  String indice = leerIndiceSonidos(p);
  if (!indiceContiene(indice, id)) {
    p.end();
    return false;
  }

  p.remove(("n_" + id).c_str());
  p.remove(("r_" + id).c_str());
  escribirIndiceSonidos(p, indiceSinId(indice, id));
  p.end();

  bool eraElActivo = false;
  if (bloquearUmbrales()) {
    if (umbrales.sonido_rtttl_id == id) {
      umbrales.sonido_rtttl_id = "";
      eraElActivo = true;
    }
    liberarUmbrales();
  }
  if (eraElActivo) guardarUmbrales();

  return true;
}

// Serializa el indice de sonidos (id + nombre, sin el rtttl) como JSON, para
// publicar por CARACT_SONIDOS.
String sonidosAJSON() {
  SonidoMeta lista[LIMITE_SONIDOS];
  int total = listarSonidos(lista, LIMITE_SONIDOS);

  StaticJsonDocument<512> doc;
  JsonArray arreglo = doc.createNestedArray("sonidos");
  for (int i = 0; i < total; i++) {
    JsonObject item = arreglo.createNestedObject();
    item["id"] = lista[i].id;
    item["nombre"] = lista[i].nombre;
  }

  String salida;
  serializeJson(doc, salida);
  return salida;
}
