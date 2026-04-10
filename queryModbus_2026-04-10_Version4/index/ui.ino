/*
 * Pantalla M5: mensajes de estado, errores, progreso.
 * Tamaño de letra 2 y logo como en v1.
 */
#include "lib/defines.h"
#include "image/xbm.h"
#include <WiFi.h>
#include <EthernetLarge.h>
#include <time.h>

// Pantalla para config solo por web (B al arranque + memoria vacía): conectate al AP y abre la URL.
void uiShowConfigQR() {
  M5.Lcd.clear();
  M5.Lcd.setTextSize(2);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("Config Web");

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("--------------------");
  M5.Lcd.println("WiFi");

  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("macromedhorus");

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Pass");
  M5.Lcd.println("horusconfig2026*");
  M5.Lcd.println("");

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("http://macromedhorus.com");

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("o");

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("http://192.168.4.1");
}

void uiShowSplash() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Horus Smart Energy");
  M5.Lcd.setTextColor(TFT_GREEN);
  M5.Lcd.println("Comunicador - Version 4");
  M5.Lcd.println("Fecha: Feb 28 del 2026");
  M5.Lcd.setTextColor(TFT_BLUE);
  M5.Lcd.println("Para: EASTRON");
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("Modelo: SDM630MCT V2");
  M5.Lcd.drawXBitmap(LOGO_AXIS_X, LOGO_AXIS_Y, logo, logoWidth, logoHeight, TFT_WHITE, TFT_BLACK);
  delay(1500);
}

// Nombres de días (tm_wday 0=Dom .. 6=Sab) y meses (tm_mon 0=Ene .. 11=Dic)
static const char* DAY_NAMES[]  = { "Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab" };
static const char* MONTH_NAMES[] = { "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic" };

// Pantalla 2: Horus Smart Energy, red, medidores, estado registro, fecha/hora, logo.
void uiShowConfigDone() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("Horus Smart Energy");

  String mode = prefs.getString(PREF_KEY_MODE, "");
  String ipStr;
  if (mode == "1") {
    M5.Lcd.print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
      ipStr = WiFi.localIP().toString();
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.println("Conectada");
    } else {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Desconectado");
      ipStr = "---";
    }
  } else {
    M5.Lcd.print("Ethernet: ");
    if (Ethernet.linkStatus() == LinkON) {
      ipStr = Ethernet.localIP().toString();
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.println("Conectada");
    } else {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Desconectado");
      ipStr = "---";
    }
  }
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(ipStr);

  uint8_t n = prefsGetMeterCount();
  for (uint8_t i = 0; i < n && i < MAX_METERS; i++) {
    String s = prefsGetSerialMedidor(i);
    int regStatus = prefsGetMeterRegStatus(i);

    M5.Lcd.setTextColor(TFT_CYAN);
    M5.Lcd.print("M");
    M5.Lcd.print(i + 1);
    M5.Lcd.print(": ");
    M5.Lcd.print(s.length() > 0 ? s : "--");
    M5.Lcd.print(" - ");

    if (regStatus == 1) {
      M5.Lcd.setTextColor(TFT_BLUE);
      M5.Lcd.println("Ok_Reg");
    } else if (regStatus == 0) {
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("No_Reg");
    } else {
      M5.Lcd.setTextColor(TFT_YELLOW);
      M5.Lcd.println("Sin_Resp");
    }
  }

  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.print("Estado del registro: ");
  if (prefsGetRegistered()) {
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("True");
  } else {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("False");
  }

  M5.Lcd.setTextColor(TFT_WHITE);
  time_t now = time(nullptr);
  if (now >= EPOCH_MIN_2026) {
    struct tm* t = localtime(&now);
    int wday = t->tm_wday;
    if (wday < 0 || wday > 6) wday = 0;
    int mon = t->tm_mon;
    if (mon < 0 || mon > 11) mon = 0;
    char buf[56];
    snprintf(buf, sizeof(buf), "%s %d/ %s/ %d - %02d:%02d",
             DAY_NAMES[wday], t->tm_mday, MONTH_NAMES[mon], t->tm_year + 1900,
             t->tm_hour, t->tm_min);
    M5.Lcd.println(buf);
  } else {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("Error en hora");
  }

  M5.Lcd.drawXBitmap(LOGO_AXIS_X, LOGO_AXIS_Y, logo, logoWidth, logoHeight, TFT_WHITE, TFT_BLACK);
}

// Pantalla 3: Enviando datos al servidor. Info M1/M2/M3 >>> server: OK (verde) / ERR (rojo).
void uiShowSendingScreen(uint8_t numMeters) {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.println("Enviando datos al server:");
  M5.Lcd.println("");
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.print("Macromedidores: ");
  M5.Lcd.println(numMeters);
  const int LINE_Y[3] = { 48, 72, 96 };
  for (uint8_t i = 0; i < numMeters && i < 3; i++) {
    M5.Lcd.setCursor(0, LINE_Y[i]);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.print("InfoM");
    M5.Lcd.print(i + 1);
    M5.Lcd.print(" >>>Server: ");
  }
}

void uiUpdateSendingResult(uint8_t meterIndex, int result) {
  const int LINE_Y[3] = { 48, 72, 96 };
  const int X_BASE = 180;
  const int X_RESULT_OK = X_BASE + 4 * 12;   // OK: casilla +4
  const int X_RESULT_ERR = X_BASE + 2 * 12;  // ERR_Macro / ERR_RED: casilla +2
  if (meterIndex > 2) return;
  if (result == SEND_RESULT_OK) {
    M5.Lcd.setCursor(X_RESULT_OK, LINE_Y[meterIndex]);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("OK");
  } else if (result == SEND_RESULT_ERR_MACRO) {
    M5.Lcd.setCursor(X_RESULT_ERR, LINE_Y[meterIndex]);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("ERR_Macro");
  } else {
    M5.Lcd.setCursor(X_RESULT_ERR, LINE_Y[meterIndex]);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.println("ERR_RED");
  }
}

void uiShowError(const char* msg) {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_RED);
  M5.Lcd.println(msg);
}

void uiShowStatus(const char* msg) {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println(msg);
}

void uiDrawLogo() {
  M5.Lcd.drawXBitmap(LOGO_AXIS_X, LOGO_AXIS_Y, logo, logoWidth, logoHeight, TFT_WHITE, TFT_BLACK);
}
