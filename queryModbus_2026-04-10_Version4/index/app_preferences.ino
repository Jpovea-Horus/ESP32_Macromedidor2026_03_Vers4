/*
 * Persistencia solo con Preferences (NVS). Sin EEPROM.
 */
#include "lib/defines.h"

extern Preferences prefs;

void preferencesLoad() {
  prefs.begin(PREF_NAMESPACE, true);  // read-only
}

void preferencesEnd() {
  prefs.end();
}

bool prefsGetRegistered() {
  String v = prefs.getString(PREF_KEY_REGISTERED, "false");
  return (v == "true");
}

uint8_t prefsGetMeterCount() {
  int n = prefs.getUChar(PREF_KEY_METER_N, 0);
  return (uint8_t)(n <= MAX_METERS ? n : 0);
}

String prefsGetMeterId(uint8_t index) {
  if (index >= MAX_METERS) return "";
  String key = String(PREF_KEY_METER_ID) + String(index);
  return prefs.getString(key.c_str(), "");
}

bool prefsNeedsConfig() {
  String mode = prefs.getString(PREF_KEY_MODE, "");
  if (mode == "1") {
    String ssid = prefs.getString(PREF_KEY_SSID, "");
    return ssid.length() == 0;
  }
  return mode.length() == 0;
}

void preferencesSave() {
  prefs.end();
  prefs.begin(PREF_NAMESPACE, false);
  // Los valores los escribe config.ino con prefsPut*()
}

void prefsPutMode(const char* mode) {
  prefs.putString(PREF_KEY_MODE, mode);
}
void prefsPutSsid(const char* ssid) {
  prefs.putString(PREF_KEY_SSID, ssid);
}
void prefsPutPass(const char* pass) {
  prefs.putString(PREF_KEY_PASS, pass);
}
void prefsPutRegistered(bool registered) {
  prefs.putString(PREF_KEY_REGISTERED, registered ? "true" : "false");
}
void prefsPutMeterCount(uint8_t n) {
  prefs.putUChar(PREF_KEY_METER_N, n);
}
void prefsPutMeterId(uint8_t index, const String& id) {
  if (index >= MAX_METERS) return;
  String key = String(PREF_KEY_METER_ID) + String(index);
  prefs.putString(key.c_str(), id);
}

void prefsPutProjectId(int projectId) {
  prefs.putInt(PREF_KEY_PROJECT_ID, projectId);
}

int prefsGetProjectId() {
  return prefs.getInt(PREF_KEY_PROJECT_ID, 0);
}

void prefsPutSerialMedidor(uint8_t index, const String& serial) {
  if (index >= MAX_METERS) return;
  String key = String(PREF_KEY_SERIAL_M) + String(index);
  prefs.putString(key.c_str(), serial);
}

String prefsGetSerialMedidor(uint8_t index) {
  if (index >= MAX_METERS) return "";
  String key = String(PREF_KEY_SERIAL_M) + String(index);
  return prefs.getString(key.c_str(), "");
}

// Estado registro en BD por medidor: 1=Ok_Reg, 0=No_Reg, -1=Sin_Resp (error red/parsing)
void prefsPutMeterRegStatus(uint8_t index, int status) {
  if (index >= MAX_METERS) return;
  String key = String(PREF_KEY_METER_REG) + String(index);
  prefs.putInt(key.c_str(), status);
}

int prefsGetMeterRegStatus(uint8_t index) {
  if (index >= MAX_METERS) return -1;
  String key = String(PREF_KEY_METER_REG) + String(index);
  return prefs.getInt(key.c_str(), -1);
}

// true si hay al menos un medidor y todos tienen serial guardado (no mostrar menú 1/2/3 al reiniciar).
bool prefsHasSerialsStored() {
  uint8_t n = prefs.getUChar(PREF_KEY_METER_N, 0);
  if (n == 0 || n > MAX_METERS) return false;
  for (uint8_t i = 0; i < n; i++) {
    String key = String(PREF_KEY_SERIAL_M) + String(i);
    if (prefs.getString(key.c_str(), "").length() == 0) return false;
  }
  return true;
}

void preferencesWipe() {
  prefs.clear();
  prefs.end();
  prefs.begin(PREF_NAMESPACE, false);
  prefs.end();
}
