// ============================================================
// Parte de Cuidin - este archivo se compila junto con los demas
// .ino de la misma carpeta (Arduino los une automaticamente).
// ============================================================

// ======================= RTTTL: PARSER + REPRODUCTOR =======================
// Formato estandar Nokia RTTTL: "nombre:d=<duracion>,o=<octava>,b=<bpm>:<notas>"
// Cada nota: [duracion]nota[#][.][octava]  (ej: "4c#5.", "8p", "g")
// Traduce cada nota a un par (frecuencia Hz, duracion ms) y las reproduce en
// secuencia con reproducirTono() (audio.ino) - no toca el bus I2S directamente.

#define RTTTL_MAX_CARACTERES 200

// Frecuencias (Hz) de las notas C..B en la octava 4, redondeadas al entero
// mas cercano. Cada octava siguiente duplica la frecuencia (>> o << segun
// suba o baje respecto a la 4).
static const int FRECUENCIAS_OCTAVA_4[12] = {
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
  //  c    c#   d    d#   e    f    f#   g    g#   a    a#   b
};

static int indiceNota(char letra) {
  switch (letra) {
    case 'c': return 0;
    case 'd': return 2;
    case 'e': return 4;
    case 'f': return 5;
    case 'g': return 7;
    case 'a': return 9;
    case 'b': return 11;
    default:  return -1; // 'p' (pausa) u otro caracter no reconocido
  }
}

static int frecuenciaDeNota(int indice, int octava) {
  if (indice < 0) return 0; // pausa: sin tono
  float frecuencia = FRECUENCIAS_OCTAVA_4[indice];
  int desplazamiento = octava - 4;
  if (desplazamiento > 0) {
    for (int i = 0; i < desplazamiento; i++) frecuencia *= 2.0f;
  } else if (desplazamiento < 0) {
    for (int i = 0; i < -desplazamiento; i++) frecuencia /= 2.0f;
  }
  return (int)(frecuencia + 0.5f);
}

// Valida sintacticamente un string RTTTL (nombre + 2 ':' + defaults + al
// menos una nota), sin reproducirlo. Usado antes de guardarlo en la
// biblioteca (sonidos.ino) para no persistir basura.
bool rtttlEsValido(const String &rtttl) {
  if (rtttl.length() == 0 || rtttl.length() > RTTTL_MAX_CARACTERES) return false;
  int primerosDosPuntos = rtttl.indexOf(':');
  if (primerosDosPuntos < 0) return false;
  int segundosDosPuntos = rtttl.indexOf(':', primerosDosPuntos + 1);
  if (segundosDosPuntos < 0) return false;
  // Debe quedar al menos un caracter de notas despues del segundo ':'.
  return segundosDosPuntos + 1 < (int)rtttl.length();
}

// Reproduce un RTTTL completo nota por nota, bloqueante (igual que el resto
// de reproducirTono()/reproducirPatronGenerado(), que ya son bloqueantes).
void reproducirRTTTL(const String &rtttl, float volumen) {
  if (!rtttlEsValido(rtttl)) {
    Serial.println("RTTTL invalido, no se reproduce.");
    return;
  }

  int primerosDosPuntos = rtttl.indexOf(':');
  int segundosDosPuntos = rtttl.indexOf(':', primerosDosPuntos + 1);

  // ----- Defaults: seccion entre el primer y segundo ':' -----
  String defaults = rtttl.substring(primerosDosPuntos + 1, segundosDosPuntos);
  int duracionDefecto = 4;
  int octavaDefecto = 5;
  int bpm = 63;
  {
    int inicio = 0;
    while (inicio < (int)defaults.length()) {
      int coma = defaults.indexOf(',', inicio);
      if (coma < 0) coma = defaults.length();
      String parte = defaults.substring(inicio, coma);
      parte.trim();
      if (parte.startsWith("d=")) duracionDefecto = parte.substring(2).toInt();
      else if (parte.startsWith("o=")) octavaDefecto = parte.substring(2).toInt();
      else if (parte.startsWith("b=")) bpm = parte.substring(2).toInt();
      inicio = coma + 1;
    }
  }
  if (duracionDefecto <= 0) duracionDefecto = 4;
  if (octavaDefecto <= 0) octavaDefecto = 5;
  if (bpm <= 0) bpm = 63;

  // Duracion en ms de una redonda (nota de duracion "1") a este bpm.
  const float msPorRedonda = (60000.0f / bpm) * 4.0f;

  // ----- Notas: seccion despues del segundo ':' -----
  String notas = rtttl.substring(segundosDosPuntos + 1);

  int inicio = 0;
  while (inicio < (int)notas.length()) {
    if (!debeSeguirSonandoAlarma()) return; // corte inmediato

    int coma = notas.indexOf(',', inicio);
    if (coma < 0) coma = notas.length();
    String nota = notas.substring(inicio, coma);
    nota.trim();
    inicio = coma + 1;
    if (nota.length() == 0) continue;

    int p = 0;

    // Duracion explicita al inicio de la nota (uno o dos digitos).
    int duracion = duracionDefecto;
    int inicioDuracion = p;
    while (p < (int)nota.length() && isDigit(nota[p])) p++;
    if (p > inicioDuracion) duracion = nota.substring(inicioDuracion, p).toInt();

    // Letra de la nota (o 'p' de pausa).
    if (p >= (int)nota.length()) continue;
    char letra = tolower(nota[p]);
    p++;

    // Sostenido opcional ('#'), no distinguimos # de b, RTTTL solo usa #.
    bool sostenido = false;
    if (p < (int)nota.length() && nota[p] == '#') {
      sostenido = true;
      p++;
    }

    // Octava explicita opcional.
    int octava = octavaDefecto;
    int inicioOctava = p;
    while (p < (int)nota.length() && isDigit(nota[p])) p++;
    if (p > inicioOctava) octava = nota.substring(inicioOctava, p).toInt();

    // Punto de prolongacion opcional (alarga la nota 1.5x).
    bool punteada = (p < (int)nota.length() && nota[p] == '.');

    int indice = indiceNota(letra);
    if (indice >= 0 && sostenido) indice++; // c# = indice de c + 1, etc.
    if (indice > 11) indice -= 12;

    int frecuencia = frecuenciaDeNota(indice, octava);
    float duracionMs = msPorRedonda / duracion;
    if (punteada) duracionMs *= 1.5f;

    if (frecuencia > 0) {
      reproducirTono((float)frecuencia, (int)duracionMs, volumen);
    } else {
      int restante = (int)duracionMs;
      while (restante > 0) {
        if (!debeSeguirSonandoAlarma()) return;
        int paso = min(50, restante);
        delay(paso);
        restante -= paso;
      }
    }
  }
}
