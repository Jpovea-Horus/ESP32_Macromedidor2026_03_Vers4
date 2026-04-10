/*
 * Interfaz web solo para configuración inicial.
 * AP "macromedhorus" + DNS (macromedhorus.com -> AP) + WebServer.
 */
#include "lib/defines.h"
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>

static WebServer webServer(80);
static DNSServer dnsServer;
static const char AP_CONFIG_HOST[] = "macromedhorus.com";
static volatile bool webConfigReceived = false;

static const char WEB_HTML[] PROGMEM = R"raw(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Horus - Configuracion</title><style>:root{--bg:#f4f6f8;--card:#fff;--text:#1f2937;--muted:#6b7280;--line:#e5e7eb;--acc:#06b6d4}*{box-sizing:border-box}body{margin:0;font-family:Arial,sans-serif;background:var(--bg);color:var(--text)}.wrap{max-width:380px;margin:18px auto;padding:0 12px}.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px}h1{font-size:20px;margin:0 0 4px}p.sub{margin:0 0 14px;color:var(--muted);font-size:13px}label{display:block;font-size:13px;margin:10px 0 6px}input,select{width:100%;height:40px;border:1px solid var(--line);border-radius:10px;padding:0 10px;background:#fff}hr{border:0;border-top:1px solid var(--line);margin:14px 0}button{width:100%;height:42px;border:0;border-radius:10px;background:var(--acc);color:#fff;font-weight:600}.small{font-size:12px;color:var(--muted);margin-top:8px}</style></head>
<body><div class="wrap"><div class="card"><h1>Horus Smart Control</h1><p class="sub">Configuracion inicial M5Stack</p><form method="post" action="/save">
<label>Conexion</label><select name="mode" id="mode"><option value="1">WiFi</option><option value="0">LAN</option></select>
<label>Codigo proyecto</label><input type="number" name="projectId" min="0" value="0" required>
<label>Cantidad medidores (1-3)</label><select name="meterN"><option value="1">1</option><option value="2">2</option><option value="3">3</option></select>
<hr>
<div id="wifiFields"><label>SSID</label><input type="text" name="ssid" maxlength="32" placeholder="Ej: MiRed_2.4G"><label>Password</label><input type="password" name="pass" maxlength="64" placeholder="********"></div>
<button type="submit">Guardar configuracion</button><div class="small">Luego puedes cerrar esta pagina.</div>
</form></div></div><script>const mode=document.getElementById('mode');const wifi=document.getElementById('wifiFields');function syncWifi(){wifi.style.display=mode.value==="1"?"block":"none";}mode.addEventListener('change',syncWifi);syncWifi();</script></body></html>
)raw";

static void handleRoot() {
  webServer.send(200, F("text/html"), WEB_HTML);
}

static void handleSave() {
  String mode = webServer.hasArg("mode") ? webServer.arg("mode") : "1";
  int projectId = webServer.hasArg("projectId") ? webServer.arg("projectId").toInt() : 0;
  int meterN = webServer.hasArg("meterN") ? webServer.arg("meterN").toInt() : 1;
  if (meterN < 1) meterN = 1;
  if (meterN > 3) meterN = 3;
  String ssid = webServer.hasArg("ssid") ? webServer.arg("ssid") : "";
  String pass = webServer.hasArg("pass") ? webServer.arg("pass") : "";

  prefsPutMode(mode.c_str());
  prefsPutProjectId(projectId);
  prefsPutMeterCount((uint8_t)meterN);
  prefsPutSsid(ssid.c_str());
  prefsPutPass(pass.c_str());
  prefsPutRegistered(false);
  for (uint8_t i = 0; i < (uint8_t)meterN && i < MAX_METERS; i++) {
    String id = (i == 0) ? "01" : (i == 1) ? "02" : "03";
    prefsPutMeterId(i, id);
  }

  webConfigReceived = true;
  webServer.send(200, F("text/html"), F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Guardado</title><style>body{margin:0;background:#f4f6f8;font-family:Arial,sans-serif;color:#1f2937}.wrap{max-width:380px;margin:18px auto;padding:0 12px}.card{background:#fff;border:1px solid #e5e7eb;border-radius:14px;padding:16px}h1{font-size:20px;margin:0 0 8px}p{margin:0 0 10px;color:#6b7280}.ok{color:#059669;font-weight:600}</style></head><body><div class='wrap'><div class='card'><h1>Horus Smart Control</h1><p class='ok'>Configuracion guardada en memoria.</p><p>Puedes cerrar esta ventana.</p></div></div></body></html>"));
}

static void handleNotFound() {
  webServer.send(404, F("text/plain"), F("Not Found"));
}

void webConfigStart() {
  webConfigReceived = false;
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("macromedhorus", "horusconfig2026*", 1, 0, 4)) {
    Serial.println("AP fallo");
    return;
  }
  delay(300);
  IPAddress apIp = WiFi.softAPIP();
  dnsServer.start(53, AP_CONFIG_HOST, apIp);
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  Serial.println(String("Web config: ") + AP_CONFIG_HOST + " -> " + apIp.toString());
}

void webConfigLoop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}

bool webConfigReceivedFlag() {
  return webConfigReceived;
}

void webConfigStop() {
  webServer.stop();
  WiFi.softAPdisconnect(true);
  delay(200);
}
