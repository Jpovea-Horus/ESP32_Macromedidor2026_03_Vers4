/*
 * Capa red: WiFi y Ethernet (LAN).
 * WiFi: conectar, mostrar IP/MAC, test 8.8.8.8.
 * LAN: MAC real, DHCP, enlace, test 8.8.8.8.
 * Diagnóstico: Google (8.8.8.8), NTP y servidor para identificar tipo de bloqueo.
 */
#include "lib/defines.h"
#include "lib/converters.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Preferences.h>
#include <EthernetLarge.h>

#define DIAG_SERVER_TIMEOUT_MS 8000
#define PING_SERVER_TIMEOUT_MS 5000

#define WIFI_TIMEOUT_MS 15000
#define WIFI_TEST_TIMEOUT_MS 3000

bool ntpTestReachability();  // ntp.ino: prueba si NTP responde

static byte g_ethMac[6];
static bool g_lanUp = false;

// Test conectividad por WiFi: TCP a 8.8.8.8:53
static bool networkTestReachabilityWiFi() {
  WiFiClient client;
  if (!client.connect(IPAddress(8, 8, 8, 8), 53)) {
    return false;
  }
  client.stop();
  return true;
}

// Test conectividad por Ethernet: TCP a 8.8.8.8:53
static bool networkTestReachabilityEthernet() {
  EthernetClient testClient;
  if (!testClient.connect(IPAddress(8, 8, 8, 8), 53)) {
    return false;
  }
  testClient.stop();
  return true;
}

// Conecta por WiFi: lee SSID/pass de Preferences, intenta conexión, muestra IP/MAC, test 8.8.8.8.
// Devuelve true si WiFi conectado, false si timeout o error.
bool networkConnectWiFi() {
  String ssid = prefs.getString(PREF_KEY_SSID, "");
  String pass = prefs.getString(PREF_KEY_PASS, "");
  if (ssid.length() == 0) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("SSID vacio");
    Serial.println("Error: SSID vacio en Preferences");
    return false;
  }

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("WiFi - Conectando...");
  M5.Lcd.println(ssid);
  Serial.println("Conectando a WiFi...");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < WIFI_TIMEOUT_MS)) {
    delay(300);
    M5.Lcd.print(".");
    Serial.print(".");
  }
  M5.Lcd.println();

  if (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("WiFi fallo");
    Serial.println("\nWiFi fallo (timeout)");
    delay(3000);
    return false;
  }

  String macStr = WiFi.macAddress();
  prefs.putString(PREF_KEY_MAC, macStr);

  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("IP: ");
  M5.Lcd.println(WiFi.localIP());
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(macStr);

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Test 8.8.8.8...");
  Serial.println("Test conectividad 8.8.8.8...");

  if (networkTestReachabilityWiFi()) {
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Conexion 8.8.8.8 OK");
    Serial.println("Conexion 8.8.8.8 OK");
  } else {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.println("Sin respuesta 8.8.8.8");
    Serial.println("Sin respuesta 8.8.8.8");
  }
  delay(4000);
  return true;
}

// --- LAN (Ethernet) ---

static bool networkTestReachability() {
  return networkTestReachabilityEthernet();
}

// Conecta por LAN: asigna MAC, inicia Ethernet por DHCP, comprueba enlace y test 8.8.8.8.
// Muestra en LCD y Serial: MAC, IP, resultado del test.
// Devuelve true si LAN está operativa, false si falla hardware, cable o test.
bool networkConnectLAN() {
  g_lanUp = false;
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("LAN - Obteniendo MAC");
  Serial.println("LAN - Obteniendo MAC");

  WiFi.mode(WIFI_STA);
  delay(100);
  String macStr = WiFi.macAddress();
  stringToMacArray(macStr, g_ethMac);

  M5.Lcd.println("MAC: ");
  M5.Lcd.println(macStr);
  Serial.print("MAC del equipo: ");
  Serial.println(macStr);

  prefs.putString(PREF_KEY_MAC, macStr);  // prefs global (index.ino)

  M5.Lcd.println("Iniciando Ethernet DHCP");
  Serial.println("Iniciando conexion Ethernet...");

  SPI.begin(SCK, MISO, MOSI, -1);
  Ethernet.init(CS);

  if (Ethernet.begin(g_ethMac) == 0) {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.println("DHCP fallo (IP 0.0.0.0)");
    Serial.println("Error: DHCP no asigno IP");
    delay(3000);
    return false;
  }

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("HW Ethernet no detectado");
    Serial.println("No se detecto hardware Ethernet.");
    delay(3000);
    return false;
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("Cable desconectado");
    Serial.println("El cable Ethernet no esta conectado.");
    delay(3000);
    return false;
  }

  IPAddress ip = Ethernet.localIP();
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("IP: ");
  M5.Lcd.println(ip);
  Serial.print("IP local: ");
  Serial.println(ip);
  Serial.print("Subnet: ");
  Serial.println(Ethernet.subnetMask());

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Test 8.8.8.8...");
  Serial.println("Test conectividad 8.8.8.8...");

  if (networkTestReachability()) {
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("Conexion 8.8.8.8 OK");
    Serial.println("Conexion 8.8.8.8 OK");
    g_lanUp = true;
  } else {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.println("Sin respuesta 8.8.8.8");
    Serial.println("Sin respuesta 8.8.8.8 (red local OK?)");
  }
  delay(4000);
  return g_lanUp;
}

void networkEnsureConnected() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  if (mode == "1") {
    String s = prefs.getString(PREF_KEY_SSID, "");
    String p = prefs.getString(PREF_KEY_PASS, "");
    if (s.length() > 0 && WiFi.status() != WL_CONNECTED) {
      WiFi.begin(s.c_str(), p.c_str());
      unsigned long t = millis();
      while (WiFi.status() != WL_CONNECTED && (millis() - t < 15000)) {
        delay(300);
      }
    }
    return;
  }
  if (mode == "0") {
    if (!g_lanUp) {
      networkConnectLAN();
    }
  }
}

bool networkIsConnected() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  if (mode == "1") return (WiFi.status() == WL_CONNECTED);
  if (mode == "0") return g_lanUp;
  return false;
}

// Estado real del enlace (para chequeo periódico). Ethernet: si link baja, marcar g_lanUp=false.
bool networkIsLinkUp() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  if (mode == "1") return (WiFi.status() == WL_CONNECTED);
  if (mode == "0") {
    if (Ethernet.linkStatus() != LinkON) {
      g_lanUp = false;
      return false;
    }
    return true;
  }
  return false;
}

// Ping por TCP al servidor (SERVER_HOST:SERVER_PORT). Devuelve true si connect OK.
bool networkPingServer() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  bool ok = false;
  if (mode == "1") {
    WiFiClient client;
    ok = client.connect(SERVER_HOST, SERVER_PORT);
    if (ok) client.stop();
  } else if (mode == "0") {
    EthernetClient client;
    ok = client.connect(SERVER_HOST, SERVER_PORT);
    if (ok) client.stop();
  }
  Serial.print("Ping TCP servidor ");
  Serial.print(SERVER_HOST);
  Serial.print(":");
  Serial.print(SERVER_PORT);
  Serial.println(ok ? " OK" : " FALLO");
  return ok;
}

// Test servidor: WiFi = GET HTTPS a SERVER_URL; LAN = TCP a SERVER_HOST:443 (solo comprueba si el puerto responde).
static bool networkTestServerReachability() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  if (mode == "1") {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String url = String(SERVER_URL);
    if (!url.startsWith("http")) url = "https://" + url;
    if (!url.endsWith("/")) url += "/";
    http.begin(client, url);
    http.setTimeout(DIAG_SERVER_TIMEOUT_MS);
    int code = http.GET();
    http.end();
    return (code > 0 && code < 500);  // 2xx, 3xx, 4xx = servidor respondió
  }
  EthernetClient client;
  if (client.connect(SERVER_HOST, 443)) {
    client.stop();
    return true;
  }
  return false;
}

// POST de datos de medidor (34 vars + hora) al servidor. Devuelve código HTTP (2xx = OK) o <0 si error.
int apiPostPlots(const String& meterSerial, const String& jsonPayload) {
  if (meterSerial.length() == 0 || jsonPayload.length() == 0) return -1;
  String mode = prefs.getString(PREF_KEY_MODE, "");
  int code = -1;

  if (mode == "1") {
    String url = String(SERVER_URL) + PATH_PLOTS + meterSerial;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("apiKey", API_KEY);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(15000);
    code = http.POST(jsonPayload);
    if (code > 0) {
      (void)http.getString();
    }
    http.end();
  } else if (mode == "0") {
    EthernetClient client;
    if (!client.connect(SERVER_HOST, SERVER_PORT)) {
      Serial.println("apiPostPlots: Ethernet connect fallo");
      return -1;
    }
    String path = String(PATH_PLOTS) + meterSerial;
    String req = String("POST ") + path + " HTTP/1.1\r\n"
                 + "Host: " + SERVER_HOST + "\r\n"
                 + "Authorization: Bearer " + API_KEY + "\r\n"
                 + "Content-Type: application/json\r\n"
                 + "Content-Length: " + String(jsonPayload.length()) + "\r\n\r\n";
    client.print(req);
    client.print(jsonPayload);
    unsigned long t = millis();
    while (client.available() == 0 && (millis() - t < 10000)) delay(10);
    String line = client.readStringUntil('\n');
    client.stop();
    if (line.startsWith("HTTP/1")) {
      int space = line.indexOf(' ', 9);
      if (space > 0) code = line.substring(9, space).toInt();
    }
  }

  Serial.print("POST plots serial ");
  Serial.print(meterSerial);
  Serial.print(" -> ");
  Serial.println(code);
  return code;
}

// GET estado serial en BD. Devuelve: 1=registrado, 0=no registrado, -1=error red/parsing.
int apiGetSerialStatus(const String& serial) {
  if (serial.length() == 0) return -1;
  String mode = prefs.getString(PREF_KEY_MODE, "");
  int bodyVal = -1;

  if (mode == "1") {
    String url = String(SERVER_URL) + PATH_SERIAL_STATUS + serial;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("apiKey", API_KEY);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    int code = http.GET();
    String payload = (code > 0) ? http.getString() : "";
    http.end();
    if (code <= 0 || (code != 200 && code != 201)) return -1;
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return -1;
    bodyVal = doc["body"] | -1;
  } else if (mode == "0") {
    EthernetClient client;
    if (!client.connect(SERVER_HOST, SERVER_PORT)) return -1;
    String path = String(PATH_SERIAL_STATUS) + serial;
    String req = String("GET ") + path + " HTTP/1.1\r\n"
                 + "Host: " + SERVER_HOST + "\r\n"
                 + "Authorization: Bearer " + API_KEY + "\r\n"
                 + "Connection: close\r\n\r\n";
    client.print(req);
    unsigned long t = millis();
    while (client.available() == 0 && (millis() - t < 10000)) delay(10);
    while (client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }
    String payload = client.readString();
    client.stop();
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) return -1;
    bodyVal = doc["body"] | -1;
  }

  return (bodyVal != 0) ? 1 : 0;
}

// Diagnóstico de bloqueo: Google (8.8.8.8), NTP y servidor. Muestra en LCD y Serial para saber qué está bloqueado.
void networkDiagnoseConnectivity() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  bool okGoogle = (mode == "1") ? networkTestReachabilityWiFi() : networkTestReachabilityEthernet();
  bool okNtp = ntpTestReachability();
  bool okServer = networkTestServerReachability();

  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Diagnostico red");
  M5.Lcd.println();
  M5.Lcd.setTextColor(okGoogle ? TFT_GREEN : TFT_RED);
  M5.Lcd.println(okGoogle ? "Google (8.8.8.8): OK" : "Google (8.8.8.8): NO");
  M5.Lcd.setTextColor(okNtp ? TFT_GREEN : TFT_RED);
  M5.Lcd.println(okNtp ? "NTP (hora): OK" : "NTP (hora): NO");
  M5.Lcd.setTextColor(okServer ? TFT_GREEN : TFT_RED);
  M5.Lcd.println(okServer ? "Servidor: OK" : "Servidor: NO");
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println();
  if (!okGoogle) M5.Lcd.println("Sin internet o firewall");
  else if (!okNtp && okServer) M5.Lcd.println("Firewall bloquea NTP");
  else if (!okServer && okNtp) M5.Lcd.println("Firewall bloquea servidor");
  else if (!okServer && !okNtp) M5.Lcd.println("Firewall NTP+servidor");

  Serial.println("=== Diagnostico conectividad ===");
  Serial.println(okGoogle ? "Google (8.8.8.8): OK" : "Google (8.8.8.8): NO");
  Serial.println(okNtp ? "NTP: OK" : "NTP: NO");
  Serial.println(okServer ? "Servidor: OK" : "Servidor: NO");
  Serial.println("=================================");

  delay(6000);
}

