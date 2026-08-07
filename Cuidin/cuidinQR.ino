/*
  ============================================================
  Cuidin - Monitor de persona - ESP32-S3-CAM (FreeRTOS multi-tarea, dual-core)
  Version con I2S a 32 bits (mic + altavoz) y modo DEBUG.

  Cambios en esta version:
  - La alarma ahora tiene "problemas prioritarios": Poca luz y Postura
    mala disparan la alarma por si solos, sin necesitar combinarse con
    otro problema (a diferencia del resto, que siguen necesitando 2).
  - Las caras feliz/molesta ya no rellenan un circulo de color de fondo:
    solo se dibujan los ojos, la boca (y cejas en la molesta) sobre fondo
    negro.
  - El texto de la pantalla de datos es mas grande (tamano 2 para los
    valores, etiquetas cortas en tamano 1 para que quepa todo).
  ============================================================
*/

// Declarado ANTES de cualquier #include a proposito: el generador
// automatico de prototipos de Arduino IDE inserta sus prototipos muy
// arriba del archivo, y como algunas funciones devuelven PosturaEstado,
// necesita que este tipo ya exista en ese punto (si no: "PosturaEstado
// does not name a type").
enum PosturaEstado { POSTURA_OK, POSTURA_MALA, POSTURA_SIN_PERSONA, POSTURA_DESCONOCIDA };

// Mismo motivo que PosturaEstado arriba: listarSonidos() (sonidos.ino)
// devuelve/recibe este tipo, asi que debe existir antes de los prototipos
// autogenerados.
struct SonidoMeta {
  String id;
  String nombre;
};

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <DHT.h>
#include "driver/i2s.h"
#include "esp_camera.h"

// ----- IA de postura (Edge Impulse) -----
#include <posture-detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "img_converters.h"   // fmt2rgb888()
#include "esp_heap_caps.h"    // heap_caps_malloc() para reservar en PSRAM

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ----- Pagina de ajustes: ESP32 como servidor BLE, pagina web (GitHub
// Pages) como cliente via Web Bluetooth API. Ya no se usa WiFi para esto. -----
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>   // Library Manager: "ArduinoJson" de Benoit Blanchon
#include <Preferences.h>
#include <TJpg_Decoder.h>  // Library Manager: "TJpg_Decoder" de Bodmer (logo de arranque)
// La URL de GitHub Pages es fija (no depende de IP local), asi que el QR
// se genera una sola vez con un texto conocido de antemano.
#define QRCODEGEN_MAX_VERSION 10
#include "qrcodegen.h"   // archivos locales (qrcodegen.h/.c) junto al .ino, NO instalados como libreria

// ======================= CONFIGURACION GENERAL =======================
// Pon esto en 0 si quieres probar primero solo los sensores, sin camara.
#define ENABLE_CAMERA 1

// ----- BLE + pagina de ajustes (GitHub Pages, cliente Web Bluetooth) -----
#define BLE_NOMBRE_DISPOSITIVO "Cuidin"
// Cambia esto por tu URL real una vez publicada la pagina en GitHub Pages.
#define URL_PAGINA_AJUSTES "https://s7even-silva.github.io/cuidin/#/conexion"

// 1 = modo debug: la pantalla muestra SOLO los datos (sin cara) y ademas
//     se vuelca todo por el puerto Serial. Util para calibrar umbrales.
// 0 = modo normal: la pantalla alterna entre datos y la cara feliz/molesta.
#define DEBUG 0

// ----- Pines de la camara -----
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

#define XCLK_FREQ_HZ      20000000

// ----- Pines (reparto fijado, ver tabla del proyecto) -----
#define TRIG_PIN      44   // UART0-RX, libre por USB nativo
#define ECHO_PIN      46   // solo-entrada; usar divisor resistivo 1K/2K
#define LDR_PIN       2    // ADC1

#define DHT_PIN       0    // strapping (BOOT): usa pull-up externo
#define DHTTYPE       DHT11

#define LED_FELIZ_PIN     3    // strapping (JTAG sel.), ok como salida simple
#define LED_MOLESTO_PIN   45   // strapping (VDD_SPI): debe quedar LOW al arrancar

#define TFT_CS        -1   // CS atado fisicamente a GND
#define TFT_DC        42
#define TFT_RST       41
#define TFT_MOSI      21
#define TFT_SCLK      47
// TFT_BL -> 3V3 fija, no se controla por software

// ----- Bus I2S UNICO, compartido entre microfono (RX) y altavoz (TX) -----
#define I2S_BCLK_PIN   1    // compartido
#define I2S_WS_PIN     48   // compartido
#define I2S_SD_PIN     14   // datos del microfono (IN)
#define I2S_DOUT_PIN   43   // datos hacia el altavoz (OUT)
#define I2S_PORT       I2S_NUM_0

// ----- Bits para elegir "problemas prioritarios" y "que mostrar en pantalla" -----
#define PRIO_TEMP        (1 << 0)
#define PRIO_HUM         (1 << 1)
#define PRIO_SONIDO      (1 << 2)
#define PRIO_LUZ         (1 << 3)
#define PRIO_POSTURA     (1 << 4)

#define MOSTRAR_PERSONA  (1 << 0)
#define MOSTRAR_TEMP     (1 << 1)
#define MOSTRAR_HUM      (1 << 2)
#define MOSTRAR_LUZ      (1 << 3)
#define MOSTRAR_SONIDO   (1 << 4)
#define MOSTRAR_POSTURA  (1 << 5)
#define MOSTRAR_TODO     (MOSTRAR_PERSONA|MOSTRAR_TEMP|MOSTRAR_HUM|MOSTRAR_LUZ|MOSTRAR_SONIDO|MOSTRAR_POSTURA)

// Patrones de sonido de alarma disponibles (sustituto de "elegir un archivo
// de la SD": no hay pines libres para un lector SD real, asi que en vez de
// eso se guardan varios patrones distintos en el propio codigo y se elige
// cual usar desde la web). Ver reproducirTono().
#define NUM_PATRONES_SONIDO 4

// ----- Umbrales/ajustes: TODOS editables desde la pagina web (BLE) -----
// Estos valores son solo el "de fabrica": la primera vez que arranca (o si
// se borra la memoria NVS) se usan estos; despues, lo que guardes desde la
// pagina web manda y sobrevive a apagados/reinicios.
struct Umbrales {
  float   dist_presencia_cm = 120.0;
  float   temp_min          = 18.0;
  float   temp_max          = 30.0;
  float   hum_min           = 30.0;
  float   hum_max           = 90.0;
  int     sonido_alarma     = 3000;
  int     luz_oscuridad     = 800;
  int     umbral_problemas  = 2;
  int     volumen           = 80;                      // 0-100 %
  bool    alarma_habilitada = true;
  uint8_t prioritarios      = PRIO_LUZ | PRIO_POSTURA;  // igual que antes por defecto
  uint8_t mostrar_datos     = MOSTRAR_TODO;             // filas visibles en pantalla
  uint8_t patron_sonido     = 0;                        // indice 0..NUM_PATRONES_SONIDO-1
  String  sonido_rtttl_id   = "";                       // id de sonidos.ino; si no esta
                                                          // vacio, tiene prioridad sobre
                                                          // patron_sonido al sonar la alarma
  bool    modo_camara       = false;                    // vista de camara en vivo en el TFT
};

Umbrales umbrales;
SemaphoreHandle_t xUmbralesMutex;
Preferences prefs;

bool bloquearUmbrales(TickType_t espera = pdMS_TO_TICKS(100)) {
  return xSemaphoreTake(xUmbralesMutex, espera) == pdTRUE;
}
void liberarUmbrales() {
  xSemaphoreGive(xUmbralesMutex);
}

void cargarUmbrales() {
  prefs.begin("cuidin", true); // solo lectura
  umbrales.dist_presencia_cm = prefs.getFloat("dist",    umbrales.dist_presencia_cm);
  umbrales.temp_min          = prefs.getFloat("tmin",    umbrales.temp_min);
  umbrales.temp_max          = prefs.getFloat("tmax",    umbrales.temp_max);
  umbrales.hum_min           = prefs.getFloat("hmin",    umbrales.hum_min);
  umbrales.hum_max           = prefs.getFloat("hmax",    umbrales.hum_max);
  umbrales.sonido_alarma     = prefs.getInt("sonido",    umbrales.sonido_alarma);
  umbrales.luz_oscuridad     = prefs.getInt("luz",       umbrales.luz_oscuridad);
  umbrales.umbral_problemas  = prefs.getInt("umbralp",   umbrales.umbral_problemas);
  umbrales.volumen           = prefs.getInt("volumen",   umbrales.volumen);
  umbrales.alarma_habilitada = prefs.getBool("activa",   umbrales.alarma_habilitada);
  umbrales.prioritarios      = prefs.getUChar("prio",    umbrales.prioritarios);
  umbrales.mostrar_datos     = prefs.getUChar("mostrar", umbrales.mostrar_datos);
  umbrales.patron_sonido     = prefs.getUChar("patron",  umbrales.patron_sonido);
  umbrales.sonido_rtttl_id   = prefs.getString("rtttlId", umbrales.sonido_rtttl_id);
  umbrales.modo_camara       = prefs.getBool("modoCam",  umbrales.modo_camara);
  prefs.end();
}

void guardarUmbrales() {
  prefs.begin("cuidin", false); // lectura/escritura
  prefs.putFloat("dist",    umbrales.dist_presencia_cm);
  prefs.putFloat("tmin",    umbrales.temp_min);
  prefs.putFloat("tmax",    umbrales.temp_max);
  prefs.putFloat("hmin",    umbrales.hum_min);
  prefs.putFloat("hmax",    umbrales.hum_max);
  prefs.putInt("sonido",    umbrales.sonido_alarma);
  prefs.putInt("luz",       umbrales.luz_oscuridad);
  prefs.putInt("umbralp",   umbrales.umbral_problemas);
  prefs.putInt("volumen",   umbrales.volumen);
  prefs.putBool("activa",   umbrales.alarma_habilitada);
  prefs.putUChar("prio",    umbrales.prioritarios);
  prefs.putUChar("mostrar", umbrales.mostrar_datos);
  prefs.putUChar("patron",  umbrales.patron_sonido);
  prefs.putString("rtttlId", umbrales.sonido_rtttl_id);
  prefs.putBool("modoCam",  umbrales.modo_camara);
  prefs.end();
}

// Desplazamiento del dato del INMP441 (dato de 24 bits alineado a la izquierda
// dentro de 32 bits). Empieza en 11; sube (12..14) si el nivel es demasiado
// grande, baja si es demasiado pequeno.
#define SHIFT_MIC             11

#define INTERVALO_DHT_MS      2000
#define MSG_LEN               40

// ================== CONFIGURACION DE LAS CARAS ==================
#define TIEMPO_VISTA_DATOS_MS   4000
#define TIEMPO_VISTA_CARA_MS    2500
#define MARGEN_CARA             8
#define CURVATURA_BOCA_DIVISOR  4

// Las caras ya NO rellenan un circulo de fondo: solo se ven ojos, boca
// (y cejas en la molesta), del color indicado, sobre fondo negro.
#define COLOR_RASGOS_FELIZ       ST77XX_YELLOW
#define COLOR_RASGOS_MOLESTO     ST77XX_RED

// ================== CONFIGURACION DEL ALTAVOZ ==================
#define TONO_ALARMA_1_HZ    880
#define TONO_ALARMA_2_HZ    660
#define DURACION_TONO_MS    200
#define AMPLITUD_TONO       6000  // volumen del tono (a nivel 16 bits, luego se sube a 32)

// ======================= OBJETOS GLOBALES DE HARDWARE =======================
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHT_PIN, DHTTYPE);

// ======================= ESTADO COMPARTIDO (PROTEGIDO POR MUTEX) =======================
struct Lecturas {
  float distancia_cm    = -1;
  int   luz_adc         = 0;
  float temperatura     = NAN;
  float humedad         = NAN;
  int   nivel_sonido    = 0;
  bool  alarma_activa   = false;
  char  mensajeAlarma[MSG_LEN] = "";
};

// (enum PosturaEstado ya se declaro al principio del archivo, antes de los #include)

Lecturas datos;
PosturaEstado posturaActual = POSTURA_DESCONOCIDA;
SemaphoreHandle_t xDatosMutex;

bool bloquearDatos(TickType_t espera = pdMS_TO_TICKS(100)) {
  return xSemaphoreTake(xDatosMutex, espera) == pdTRUE;
}
void liberarDatos() {
  xSemaphoreGive(xDatosMutex);
}

// ---- Prototipos ----
void iniciarPantalla();
void iniciarBusI2S();
#if ENABLE_CAMERA
void iniciarCamara();
#endif
void pantallaBienvenida();
void taskSensores(void *parametro);
void taskAudio(void *parametro);
void taskAlarma(void *parametro);
#if ENABLE_CAMERA
void taskCamaraIA(void *parametro);
#endif
void taskPantalla(void *parametro);
float medirDistanciaCm();
void reproducirTono(float frecuenciaHz, int duracionMs, float volumen = 1.0f);
void dibujarPantalla(const Lecturas &copia, PosturaEstado postura, const Umbrales &u);
void dibujarCaraFeliz();
void dibujarCaraMolesta();
#if DEBUG
void imprimirSerialDebug(const Lecturas &c, PosturaEstado postura);
#endif

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Iniciando Cuidin (ESP32-S3, FreeRTOS)...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_FELIZ_PIN, OUTPUT);
  pinMode(LED_MOLESTO_PIN, OUTPUT);
  digitalWrite(LED_FELIZ_PIN, LOW);
  digitalWrite(LED_MOLESTO_PIN, LOW);

  analogReadResolution(12); // 0-4095

  dht.begin();

  iniciarPantalla();
  iniciarBusI2S();

  #if ENABLE_CAMERA
    iniciarCamara();
  #endif

  xDatosMutex = xSemaphoreCreateMutex();
  if (xDatosMutex == NULL) {
    Serial.println("ERROR: no se pudo crear el mutex. Deteniendo.");
    while (true) { delay(1000); }
  }

  xUmbralesMutex = xSemaphoreCreateMutex();
  if (xUmbralesMutex == NULL) {
    Serial.println("ERROR: no se pudo crear el mutex de umbrales. Deteniendo.");
    while (true) { delay(1000); }
  }
  cargarUmbrales(); // trae lo guardado en NVS (o los valores de fabrica si es la primera vez)

  iniciarSD(); // sonidos de alarma; si no hay SD, cae a los patrones generados
  iniciarBLE(); // servidor BLE con los ajustes (ver documento del protocolo)

  pantallaBienvenida();
  mostrarQRAjustes(); // QR fijo (URL de GitHub Pages) + recuerda el nombre BLE

  xTaskCreatePinnedToCore(taskSensores,  "TaskSensores",  3072, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskAudio,     "TaskAudio",     3072, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(taskAlarma,    "TaskAlarma",    3072, NULL, 4, NULL, 0);
  #if ENABLE_CAMERA
    xTaskCreatePinnedToCore(taskCamaraIA, "TaskCamaraIA", 16384, NULL, 1, NULL, 1);
  #endif
  xTaskCreatePinnedToCore(taskPantalla,  "TaskPantalla",  4096, NULL, 2, NULL, 1);

  Serial.println("Tareas creadas. loop() queda inactivo.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

