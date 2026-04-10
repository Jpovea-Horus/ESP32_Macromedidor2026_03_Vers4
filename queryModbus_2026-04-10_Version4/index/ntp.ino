/*
 * Sincronización de hora: NTP (varios servidores) + respaldo HTTP (WorldTimeAPI).
 * Se llama después de networkEnsureConnected() (red ya verificada).
 */
#include "lib/defines.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <EthernetLarge.h>
#include <sys/time.h>

static WiFiUDP s_ntpUdp;
static EthernetUDP s_ethUdp;
static WiFiClientSecure s_wifiSecure;

// Intenta obtener hora por NTP (WiFi). Devuelve true si epoch válido (>= EPOCH_MIN_2026).
static bool tryNtpEpochWiFi(const char* server, time_t& outEpoch) {
  NTPClient client(s_ntpUdp, server, 0, 60000);
  client.begin();
  client.setTimeOffset(TIME_OFFSET_COL_SEC);
  if (!client.update()) return false;
  outEpoch = client.getEpochTime();
  return (outEpoch >= (time_t)EPOCH_MIN_2026);
}

// Intenta obtener hora por NTP (Ethernet).
static bool tryNtpEpochEthernet(const char* server, time_t& outEpoch) {
  NTPClient client(s_ethUdp, server, 0, 60000);
  client.begin();
  client.setTimeOffset(TIME_OFFSET_COL_SEC);
  if (!client.update()) return false;
  outEpoch = client.getEpochTime();
  return (outEpoch >= (time_t)EPOCH_MIN_2026);
}

// Respaldo: hora por HTTP (WorldTimeAPI). Solo con WiFi (HTTPS).
static bool tryHttpEpoch(time_t& outEpoch) {
  HTTPClient http;
  http.begin(s_wifiSecure, "https://worldtimeapi.org/api/timezone/America/Bogota");
  http.setTimeout(8000);
  int code = http.GET();
  bool ok = false;
  if (code == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      if (doc.containsKey("unixtime")) {
        time_t utc = (time_t)doc["unixtime"].as<long>();
        outEpoch = utc + TIME_OFFSET_COL_SEC;
        ok = (outEpoch >= (time_t)EPOCH_MIN_2026);
      }
    }
  }
  http.end();
  return ok;
}

// Prueba solo si NTP responde (para diagnóstico de bloqueo). No modifica la hora.
bool ntpTestReachability() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  time_t epoch = 0;
  if (mode == "1") return tryNtpEpochWiFi("0.pool.ntp.org", epoch);
  return tryNtpEpochEthernet("0.pool.ntp.org", epoch);
}

// Sincroniza hora: NTP (3 servidores) y si falla en WiFi usa HTTP. Devuelve true si se actualizó.
bool syncTimeFromNetwork() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  time_t epoch = 0;
  bool actualizado = false;

  const char* ntpServers[] = { "0.pool.ntp.org", "time.google.com", "time.windows.com" };
  const int numNtp = 3;

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Sincronizando hora...");
  Serial.println("Sincronizando hora (NTP + respaldo HTTP)");

  if (mode == "1") {
    for (int i = 0; i < numNtp && !actualizado; i++) {
      Serial.print("NTP opcion ");
      Serial.println(i + 1);
      actualizado = tryNtpEpochWiFi(ntpServers[i], epoch);
    }
    if (!actualizado) {
      Serial.println("NTP fallido, respaldo: HTTP WorldTimeAPI");
      M5.Lcd.println("NTP fallo, intento HTTP");
      actualizado = tryHttpEpoch(epoch);
    }
  } else {
    for (int i = 0; i < numNtp && !actualizado; i++) {
      Serial.print("NTP opcion ");
      Serial.println(i + 1);
      actualizado = tryNtpEpochEthernet(ntpServers[i], epoch);
    }
  }

  if (actualizado && epoch >= (time_t)EPOCH_MIN_2026) {
    struct tm* ptm = gmtime(&epoch);
    time_t t = mktime(ptm);
    struct timeval tv = { t, 0 };
    settimeofday(&tv, nullptr);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Hora actualizada");
    Serial.println("Hora actualizada");
    return true;
  }

  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.println("Fallo NTP/HTTP");
  Serial.println("Fallo al obtener hora (NTP y HTTP)");
  return false;
}
