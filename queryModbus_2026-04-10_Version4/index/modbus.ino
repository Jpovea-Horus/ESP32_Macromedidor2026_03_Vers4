/*
 * RS485 / Modbus: serial de macromedidores + lectura 34 variables cada 5 min.
 */
#include "lib/defines.h"
#include "lib/CRC.h"
#include "lib/converters.h"
#include "lib/tramasModbus.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <time.h>

#define SERIAL_RS485_BUFFER_MAX 64
#define SERIAL_RS485_MAX_INTENTOS 5
#define SERIAL_REENVIOS_MAX 15
#define SERIAL_INVALIDO 68010288UL

// Tiempos de espera para recibir respuesta serial Modbus (9 bytes típicos)
#define RX_DELAY_INICIAL_MS    80   // Espera tras pasar a RX antes de leer (turnaround bus + medidor)
#define RX_VENTANAS            8    // Número de ventanas de lectura
#define RX_DELAY_VENTANA_MS    50   // ms por ventana (9600 baud ~1 ms/byte)
#define RX_DELAY_ENTRE_INTENTOS_MS 150  // Entre reintentos si no hay respuesta válida
#define TX_TO_RX_DELAY_MS      60   // Entre enviar trama y empezar a escuchar (caller)

static byte plotsModbus[8];
static bool rs485Inited = false;

void modbusRS485Init() {
  if (rs485Inited) return;
#if RS485_USE_DE_RE
  pinMode(DE, OUTPUT);
  pinMode(RE, OUTPUT);
#endif
#if RS485_8E1
  Serial2.begin(BAUD, SERIAL_8E1, RXPIN, TXPIN);
  Serial.println("[RS485] Serial2 init: " + String(BAUD) + " 8E1, RX=" + String(RXPIN) + " TX=" + String(TXPIN) + (RS485_USE_DE_RE ? " DE/RE=on" : " DE/RE=off (auto)"));
#else
  Serial2.begin(BAUD, SERIAL_8N1, RXPIN, TXPIN);
  Serial.println("[RS485] Serial2 init: " + String(BAUD) + " 8N1, RX=" + String(RXPIN) + " TX=" + String(TXPIN) + (RS485_USE_DE_RE ? " DE/RE=on" : " DE/RE=off (auto)"));
#endif
  rs485Inited = true;
}

static int limpiarBufferSerial2() {
  int n = 0;
  while (Serial2.available()) { Serial2.read(); n++; }
  if (n > 0) Serial.println("[RS485] Buffer descartados: " + String(n) + " bytes");
  return n;
}

static void decodeDataModbus(const String& hexStr, byte out[8]) {
  for (int i = 0; i < 8 && (i * 2 + 2) <= (int)hexStr.length(); i++) {
    String two = hexStr.substring(i * 2, i * 2 + 2);
    out[i] = (byte)strtoul(two.c_str(), NULL, 16);
  }
}

// Envío Modbus (alineado con Prueba_Modbus: flush tras write, delay antes de pasar a RX).
static void enviarDatoModbus(byte* data, const char* logTrama) {
#if RS485_USE_DE_RE
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
#endif
  Serial2.write(data, 8);
  Serial2.flush();
#if RS485_USE_DE_RE
  delay(15);
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
#endif
  Serial.println("[RS485] TX -> " + String(logTrama));
  Serial.print("[RS485] TX bytes: ");
  for (int i = 0; i < 8; i++) Serial.print(decToHex(data[i], 2) + " ");
  Serial.println();
}

// Recepción Modbus: ventanas de lectura para acumular los 9 bytes de la respuesta (serial).
static String serialCheckModbus() {
  String hexResponse = "";
  for (int intentos = 0; intentos < SERIAL_RS485_MAX_INTENTOS; intentos++) {
#if RS485_USE_DE_RE
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
#endif
    delay(RX_DELAY_INICIAL_MS);
    hexResponse = "";
    for (int w = 0; w < RX_VENTANAS; w++) {
      delay(RX_DELAY_VENTANA_MS);
      while (Serial2.available()) {
        byte b = Serial2.read();
        if (hexResponse.length() / 2 < (int)SERIAL_RS485_BUFFER_MAX)
          hexResponse += decToHex(b, 2);
      }
    }
    int numBytes = hexResponse.length() / 2;
    Serial.println("[RS485] RX intento " + String(intentos + 1) + "/" + String(SERIAL_RS485_MAX_INTENTOS) + " -> " + String(numBytes) + " bytes");
    if (numBytes == 0) {
      Serial.println("[RS485] No se recibieron datos del medidor.");
      delay(RX_DELAY_ENTRE_INTENTOS_MS);
      continue;
    }
    Serial.println("[RS485] Respuesta completa en hex: " + hexResponse);
    if (hexResponse.length() >= 18) {
      String serialHex = hexResponse.substring(6, 14);
      unsigned long serialDec = hexToDec(serialHex);
      Serial.print("[RS485] Serial extraído: ");
      Serial.println(serialDec);
      Serial.print("[RS485] Intento #: ");
      Serial.println(intentos + 1);
      if (serialDec != SERIAL_INVALIDO) return hexResponse;
      Serial.println("[RS485] Serial 68010288 inválido, reintentando...");
      delay(RX_DELAY_ENTRE_INTENTOS_MS);
      continue;
    }
    Serial.println("[RS485] Respuesta demasiado corta.");
    delay(RX_DELAY_ENTRE_INTENTOS_MS);
  }
  Serial.println("[RS485] Máximo de intentos alcanzado.");
  return "";
}

// Lee respuesta Modbus para datos (18 hex = 9 bytes); sin filtrar por serial. Reintentos reducidos.
static String readResponseModbusData(int maxIntentos) {
  String hexResponse = "";
  for (int intentos = 0; intentos < maxIntentos; intentos++) {
#if RS485_USE_DE_RE
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
#endif
    delay(RX_DELAY_INICIAL_MS);
    hexResponse = "";
    for (int w = 0; w < RX_VENTANAS; w++) {
      delay(RX_DELAY_VENTANA_MS);
      while (Serial2.available()) {
        byte b = Serial2.read();
        if (hexResponse.length() / 2 < (int)SERIAL_RS485_BUFFER_MAX)
          hexResponse += decToHex(b, 2);
      }
    }
    if (hexResponse.length() >= 18) return hexResponse;
    delay(RX_DELAY_ENTRE_INTENTOS_MS);
  }
  return "";
}

// Lee un registro (variable) del medidor. Devuelve valor float o NAN si falla.
static float readOneRegisterModbus(const String& dirNorm, int varIndex) {
  if (varIndex < 0 || varIndex >= NUM_MODBUS_VARS) return NAN;
  String plot = dirNorm + String(VariablesModbus[varIndex]);
  String crc = CyclicRedundancyCheck(plot);
  String tramaHex = plot + crc;
  decodeDataModbus(tramaHex, plotsModbus);
  limpiarBufferSerial2();
#if RS485_USE_DE_RE
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
#endif
  Serial2.write(plotsModbus, 8);
  Serial2.flush();
#if RS485_USE_DE_RE
  delay(15);
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);
#endif
  delay(TX_TO_RX_DELAY_MS);
  String hexResp = readResponseModbusData(3);
  if (hexResp.length() < 18) return NAN;
  String hex8 = hexResp.substring(6, 14);
  return hexToFloatIEEE754(hex8);
}

bool obtenerSerialesMedidoresRS485() {
  modbusRS485Init();
  delay(500);   // Calentamiento bus / primer medidor
  uint8_t n = prefsGetMeterCount();
  if (n == 0) return false;
  bool alMenosUno = false;
  for (uint8_t i = 0; i < n; i++) {
    String dir = prefsGetMeterId(i);
    if (dir.length() == 0) continue;
    // Dirección siempre 2 dígitos hex para la trama (como Funcional: "01", "02")
    String dirNorm = (dir.length() == 1) ? ("0" + dir) : dir;
    String direccion = dirNorm + "03FC000002";
    String crc = CyclicRedundancyCheck(direccion);
    String tramaHex = direccion + crc;
    decodeDataModbus(tramaHex, plotsModbus);
    String num = "";
    Serial.println("[RS485] --- Medidor " + String(i + 1) + " dir=" + dirNorm + " trama=" + tramaHex + " ---");
    for (int reenvio = 0; reenvio < SERIAL_REENVIOS_MAX && num.length() < 18; reenvio++) {
      limpiarBufferSerial2();
      enviarDatoModbus(plotsModbus, ("medidor " + String(i + 1) + " reenvio " + String(reenvio + 1)).c_str());
      delay(TX_TO_RX_DELAY_MS);
      num = serialCheckModbus();
      if (num.length() >= 18) {
        String addrResp = num.substring(0, 2);
        if (addrResp != dirNorm) {
          Serial.println("[RS485] Respuesta de otro medidor (dir " + addrResp + "), reintento.");
          num = "";
          delay(400);
          continue;
        }
        String serialStr = String(hexToDec(num.substring(6, 14)));
        prefsPutSerialMedidor(i, serialStr);
        alMenosUno = true;
        Serial.print("Serial medidor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(serialStr);
        break;
      }
      delay(500);
    }
    limpiarBufferSerial2();
    delay(600);
  }
  return alMenosUno;
}

void modbusInit() {
  modbusRS485Init();
}

String modbusQueryMeter(const String& meterId) {
  (void)meterId;
  return "{}";
}

// Último minuto en que se ejecutó la lectura cada 5 min (evita repetir en el mismo minuto)
static int s_lastMinuteRun = -1;

void queryMetersLoop() {
  time_t now = time(nullptr);
  if (now < EPOCH_MIN_2026) return;  // Hora no sincronizada
  struct tm* t = localtime(&now);
  int min = t->tm_min;
  // Ejecutar solo cuando el último dígito del minuto es 0 o 5 (cada 5 min)
  if ((min % 5) != 0) return;
  if (min == s_lastMinuteRun) return;
  s_lastMinuteRun = min;

  modbusRS485Init();
  uint8_t n = prefsGetMeterCount();
  if (n == 0) return;

  uiShowSendingScreen(n);

  Serial.println();
  Serial.println("=== Estado Macromedidores (cada 5 min) ===");
  Serial.print("Hora: ");
  Serial.print(t->tm_hour);
  Serial.print(":");
  if (t->tm_min < 10) Serial.print("0");
  Serial.println(t->tm_min);

  for (uint8_t m = 0; m < n; m++) {
    String dir = prefsGetMeterId(m);
    if (dir.length() == 0) continue;
    String dirNorm = (dir.length() == 1) ? ("0" + dir) : dir;
    String serialM = prefsGetSerialMedidor(m);
    Serial.println();
    Serial.println("--- Medidor " + String(m + 1) + " dir=" + dirNorm + " serial=" + serialM + " ---");

    // Mismo formato que se envía al servidor: array de "Nombre: valor" + Time
    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();
    int validCount = 0;
    for (int v = 0; v < NUM_MODBUS_VARS; v++) {
      float val = readOneRegisterModbus(dirNorm, v);
      String item = String(NAMES_MODBUS[v]) + ": ";
      if (isnan(val)) item += "--";
      else {
        item += String(val);
        validCount++;
      }
      arr.add(item);
      delay(60);
    }
    // Hora en formato como v1 (asctime)
    char timeBuf[32];
    struct tm* tt = localtime(&now);
    strftime(timeBuf, sizeof(timeBuf), "%a %b %d %H:%M:%S %Y", tt);
    arr.add(String(timeBuf));

    String jsonPayload;
    serializeJson(arr, jsonPayload);
    Serial.println("Payload (formato servidor):");
    Serial.println(jsonPayload);

    if (serialM.length() > 0) {
      if (validCount == 0) {
        Serial.println("Medidor " + String(m + 1) + " sin respuesta: no se envia - ERR_Dato_Vacio");
        uiUpdateSendingResult(m, SEND_RESULT_ERR_MACRO);
      } else {
        int code = apiPostPlots(serialM, jsonPayload);
        bool ok = (code >= 200 && code < 300);
        uiUpdateSendingResult(m, ok ? SEND_RESULT_OK : SEND_RESULT_ERR_RED);
        if (ok) {
          Serial.println("Medidor " + String(m + 1) + " enviado OK");
        } else {
          Serial.println("Medidor " + String(m + 1) + " envio fallo (cod " + String(code) + ")");
        }
      }
    } else {
      uiUpdateSendingResult(m, SEND_RESULT_ERR_MACRO);
    }
  }
  delay(2500);
  uiShowConfigDone();

  Serial.println("=== Fin estado Macromedidores ===");
  Serial.println();
}
