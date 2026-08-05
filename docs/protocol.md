# Protocolo BLE de Cuidín

Fuente de verdad única del contrato entre el firmware (`Cuidin/*.ino`) y el frontend (`web/`). Si cambias algo aquí, cambia también el lado correspondiente (`Cuidin/ble.ino` y `web/src/lib/bleProtocol.ts`).

El ESP32 es el **Peripheral** BLE (anuncia el servicio); la página web es el **Central** (se conecta vía Web Bluetooth API). No hay fragmentación de mensajes: todos los payloads son JSON UTF-8 que caben en un único `StaticJsonDocument`, sin negociación especial de MTU.

## Servicio

```
UUID_SERVICIO  6d5a1000-0001-4c1a-8b1a-2f6a9c8e1a01
```

Nombre de anuncio BLE: `Cuidin` (`BLE_NOMBRE_DISPOSITIVO`).

## Características

| Característica | UUID | Propiedades | Contenido |
|---|---|---|---|
| `CARACT_LEER` | `6d5a1000-0002-4c1a-8b1a-2f6a9c8e1a01` | READ, NOTIFY | JSON completo de `Umbrales` (estado actual). Se notifica cada vez que cambia, incluida la confirmación tras una escritura. |
| `CARACT_ESCRIBIR` | `6d5a1000-0003-4c1a-8b1a-2f6a9c8e1a01` | WRITE | JSON parcial: solo los campos de `Umbrales` que se quieren cambiar. Tras aplicar, el firmware persiste en NVS y notifica el estado final por `CARACT_LEER`. |
| `CARACT_SONIDOS` | `6d5a1000-0004-4c1a-8b1a-2f6a9c8e1a01` | READ, NOTIFY, WRITE | Índice de la biblioteca de sonidos RTTTL (lectura/notificación) y comandos de gestión (escritura). Ver [§3](#3-caract_sonidos). |
| `CARACT_ESTADO` | `6d5a1000-0005-4c1a-8b1a-2f6a9c8e1a01` | READ, NOTIFY | Datos en vivo de los sensores (no ajustes), notificados automáticamente cada ~200ms mientras haya alguien conectado. Ver [§4](#4-caract_estado). |

## 1. `CARACT_LEER` — estado completo

Publicado por `umbralesAJSON()` en `ble.ino`, `StaticJsonDocument<512>`.

```json
{
  "dist": 120.0,
  "tmin": 18.0,
  "tmax": 30.0,
  "hmin": 30.0,
  "hmax": 90.0,
  "sonido": 3000,
  "luz": 800,
  "umbralp": 2,
  "volumen": 80,
  "activa": true,
  "prio": 24,
  "mostrar": 63,
  "patron": 0,

  "sonidoRtttlId": "",
  "modoCamara": false
}
```

| Campo | Tipo | Corresponde a | Descripción |
|---|---|---|---|
| `dist` | float | `dist_presencia_cm` | Distancia (cm) bajo la cual se considera "persona detectada". No dispara alarma, solo indica presencia. |
| `tmin` / `tmax` | float | `temp_min` / `temp_max` | Rango válido de temperatura (°C, DHT11). Fuera de rango = problema. |
| `hmin` / `hmax` | float | `hum_min` / `hum_max` | Rango válido de humedad (%). |
| `sonido` | int | `sonido_alarma` | Nivel de sonido ambiente (RMS crudo del INMP441) por encima del cual se considera "sonido fuerte". |
| `luz` | int | `luz_oscuridad` | Lectura ADC del LDR por debajo de la cual se considera "poca luz" (0-4095). |
| `umbralp` | int | `umbral_problemas` | Cantidad de problemas no-prioritarios simultáneos necesarios para disparar la alarma. |
| `volumen` | int | `volumen` | Volumen del sonido de alarma, 0-100 (%). |
| `activa` | bool | `alarma_habilitada` | Interruptor general: si es `false`, ningún problema dispara la alarma. |
| `prio` | uint8 (bitmask) | `prioritarios` | Qué problemas disparan la alarma por sí solos, sin combinarse. Ver [bitmask PRIO_*](#bitmask-prio_). |
| `mostrar` | uint8 (bitmask) | `mostrar_datos` | Qué filas se muestran en la vista de datos del TFT. Ver [bitmask MOSTRAR_*](#bitmask-mostrar_). Si es `0`, la pantalla se queda solo en la cara feliz/molesta. |
| `patron` | uint8 (0-3) | `patron_sonido` | Índice del patrón de sonido generado / archivo WAV en SD (`/alarmas/<patron>.wav`) a usar si `sonidoRtttlId` está vacío. |
| `sonidoRtttlId` | string | *(nuevo)* | Id de una melodía guardada en la biblioteca RTTTL (ver §3). Si no está vacío, tiene prioridad sobre `patron` al disparar la alarma. Vacío `""` = usar `patron` como hasta ahora. |
| `modoCamara` | bool | *(nuevo)* | Activa/desactiva la vista de cámara en vivo con overlay de postura en el TFT (reemplaza temporalmente el ciclo datos/cara mientras esté activo). |

### Bitmask `PRIO_*` (campo `prio`)

| Bit | Constante | Problema |
|---|---|---|
| `1 << 0` (1) | `PRIO_TEMP` | Temperatura fuera de rango |
| `1 << 1` (2) | `PRIO_HUM` | Humedad fuera de rango |
| `1 << 2` (4) | `PRIO_SONIDO` | Sonido fuerte |
| `1 << 3` (8) | `PRIO_LUZ` | Poca luz |
| `1 << 4` (16) | `PRIO_POSTURA` | Postura mala |

Valor por defecto: `PRIO_LUZ | PRIO_POSTURA` = `24`.

### Bitmask `MOSTRAR_*` (campo `mostrar`)

| Bit | Constante | Fila mostrada |
|---|---|---|
| `1 << 0` (1) | `MOSTRAR_PERSONA` | Presencia (distancia) |
| `1 << 1` (2) | `MOSTRAR_TEMP` | Temperatura |
| `1 << 2` (4) | `MOSTRAR_HUM` | Humedad |
| `1 << 3` (8) | `MOSTRAR_LUZ` | Luz |
| `1 << 4` (16) | `MOSTRAR_SONIDO` | Nivel de sonido |
| `1 << 5` (32) | `MOSTRAR_POSTURA` | Postura |

Valor por defecto: todos activos (`MOSTRAR_TODO` = `63`).

## 2. `CARACT_ESCRIBIR` — cambios parciales

Mismo esquema de campos que `CARACT_LEER`, pero **todos opcionales**: el firmware solo actualiza las claves presentes en el JSON recibido (`doc.containsKey(...)`), el resto de `Umbrales` queda igual. Tras aplicar los cambios válidos, persiste en NVS (`guardarUmbrales()`) y notifica el estado final completo por `CARACT_LEER`.

Ejemplo — cambiar solo el volumen y activar el modo cámara:
```json
{ "volumen": 60, "modoCamara": true }
```

JSON inválido (no parsea) se ignora completamente y se loguea por Serial; no hay respuesta de error explícita hoy — el frontend debe inferir éxito/fallo comparando el estado notificado en `CARACT_LEER` contra lo que intentó escribir.

## 3. `CARACT_SONIDOS` — biblioteca de melodías RTTTL

*(nuevo, fase 3 del plan)*. Colección de melodías RTTTL definidas por el usuario, reproducidas por `reproducirAlarmaSonora` cuando `sonidoRtttlId` (en `Umbrales`) no está vacío. Límites duros validados en firmware antes de guardar: **máx. 8 melodías**, **máx. 200 caracteres por string RTTTL**.

### Lectura / notificación

Solo id + nombre (nunca el RTTTL completo, para mantener el payload pequeño):
```json
{
  "sonidos": [
    { "id": "s1", "nombre": "Suave" },
    { "id": "s2", "nombre": "Urgente" }
  ]
}
```

### Escritura — comandos (campo `accion`)

**Subir (crear o reemplazar por nombre):**
```json
{ "accion": "subir", "nombre": "Urgente", "rtttl": "urgente:d=4,o=5,b=140:c,e,g" }
```
El firmware genera el `id` si es una melodía nueva. Si se excede el límite de 8 melodías o el RTTTL supera 200 caracteres, la operación se ignora (se loguea por Serial) y no se aplica — igual que el manejo de JSON inválido en `CARACT_ESCRIBIR`.

**Borrar:**
```json
{ "accion": "borrar", "id": "s2" }
```
Si `id` coincide con el `sonidoRtttlId` activo en `Umbrales`, este se limpia a `""` (vuelve a usar `patron`).

**Probar (reproducir en el buzzer físico de Cuidín, sin tocar `Umbrales`):**
```json
{ "accion": "probar", "id": "s2" }
```

Tras cualquier `subir`/`borrar`, el firmware renotifica el índice completo por `CARACT_SONIDOS` (mismo patrón de "confirmar con el estado final" que ya usa `CARACT_LEER`).

## 4. `CARACT_ESTADO` — datos en vivo de sensores

Publicado por `estadoAJSON()` en `ble.ino`, `StaticJsonDocument<256>`. A diferencia de `CARACT_LEER` (ajustes/configuración), esta característica expone **lecturas**, no configuración — es solo lectura desde la web (no tiene característica de escritura asociada). Se renotifica automáticamente cada ~200ms desde `taskAlarma` (`alarma.ino`), independientemente de si la alarma está habilitada o no.

```json
{
  "distancia": 118.2,
  "luz": 812,
  "temperatura": 24.3,
  "humedad": 55.1,
  "sonido": 1450,
  "postura": "ok",
  "alarma": false,
  "mensaje": ""
}
```

| Campo | Tipo | Descripción |
|---|---|---|
| `distancia` | float | Distancia (cm) medida por el HC-SR04. `-1` si no hay lectura válida. |
| `luz` | int | Lectura ADC cruda del LDR (0-4095). |
| `temperatura` | float | Temperatura (°C) del DHT11. `NaN` (se serializa como `null`) si no hay lectura válida aún. |
| `humedad` | float | Humedad (%) del DHT11. `NaN` (→ `null`) si no hay lectura válida aún. |
| `sonido` | int | Nivel de sonido ambiente (RMS crudo del INMP441). |
| `postura` | string | Una de `"ok"`, `"mala"`, `"sin_persona"`, `"desconocida"` (traducción de `PosturaEstado`, para que la web no necesite conocer el enum interno). |
| `alarma` | bool | Si la alarma está sonando/activa en este momento. |
| `mensaje` | string | Texto descriptivo de los problemas activos (ej. `"Poca luz + Corrige tu postura"`), vacío si no hay alarma. |

## Formato RTTTL soportado

Formato estándar Nokia RTTTL: `nombre:d=<duración>,o=<octava>,b=<bpm>:<notas separadas por coma>`. El parser de firmware (`Cuidin/rtttl.ino`) traduce cada nota a un par (frecuencia Hz, duración ms) y los reproduce en secuencia con la función ya existente `reproducirTono(frecuenciaHz, duracionMs, volumen)` (`audio.ino`). El parser en el frontend (`web/src/lib/rtttl.ts`) es una reimplementación en TypeScript del mismo formato, usada tanto para validar antes de enviar como para el preview de audio en el navegador (Web Audio API).

## Persistencia en el ESP32

Todo vía `Preferences` (NVS), mismo mecanismo ya usado hoy:

- Namespace `"cuidin"`: struct `Umbrales` completo (ya existente, se le añaden `sonidoRtttlId` y `modoCamara`).
- Namespace `"sonidos"` *(nuevo)*: pares `id -> (nombre, rtttl)` más una clave `"indice"` con la lista de ids separados por coma (`Preferences` no permite iterar claves dinámicamente).

## Notas de compatibilidad

- Todos los campos y características nuevos (`sonidoRtttlId`, `modoCamara`, `CARACT_SONIDOS`, `CARACT_ESTADO`) son aditivos: un frontend viejo que no los conozca sigue funcionando contra el resto del protocolo sin romperse, porque `CARACT_ESCRIBIR` ignora silenciosamente las claves que no reconoce y el firmware sigue publicando el resto de campos/características igual.
- No hay autenticación ni cifrado a nivel de aplicación (decisión consciente, ver plan de implementación). El firmware valida rangos y límites de todo dato recibido antes de persistir, para no corromper su propio estado, aunque no autentique quién lo envía.
