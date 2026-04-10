#ifndef CONVERTERS_H
#define CONVERTERS_H

// Convierte MAC en formato "AA:BB:CC:DD:EE:FF" a byte mac[6]
void stringToMacArray(String macString, byte macArray[]) {
  if (macString.length() < 17) return;
  for (int i = 0; i < 6; i++) {
    String hex = macString.substring(i * 3, i * 3 + 2);
    macArray[i] = (byte)strtoul(hex.c_str(), NULL, 16);
  }
}

// Hex string -> decimal (para extraer serial de respuesta Modbus)
unsigned long hexToDec(String hexString) {
  unsigned long decValue = 0;
  for (size_t i = 0; i < hexString.length(); i++) {
    char c = hexString.charAt(i);
    int v = 0;
    if (c >= '0' && c <= '9') v = c - '0';
    else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
    else continue;
    decValue = (decValue << 4) + v;
  }
  return decValue;
}

// Byte -> hex string (2 chars para respuesta Modbus)
String decToHex(byte decValue, byte desiredStringLength) {
  String hexString = String(decValue, HEX);
  while ((unsigned)hexString.length() < desiredStringLength) hexString = "0" + hexString;
  return hexString;
}

// Hex (2 chars) -> binario para IEEE-754
String hexToBin(String hexString) {
  unsigned long decValue = hexToDec(hexString);
  if (decValue == 0) return "0";
  String b = "";
  while (decValue) {
    b = String(decValue & 1) + b;
    decValue >>= 1;
  }
  return b;
}

String addLeadingZeros(String binaryNumber, int desiredLength) {
  while ((int)binaryNumber.length() < desiredLength) binaryNumber = "0" + binaryNumber;
  return binaryNumber;
}

// Binario string -> entero (para exponente IEEE-754)
int binToInt(String binStr) {
  int val = 0;
  for (size_t i = 0; i < binStr.length(); i++)
    val = val * 2 + (binStr[i] - '0');
  return val;
}

// Convierte 8 caracteres hex (4 bytes, orden Modbus big-endian) a float IEEE-754
float hexToFloatIEEE754(String hex8) {
  if (hex8.length() < 8) return NAN;
  String b0 = addLeadingZeros(hexToBin(hex8.substring(0, 2)), 8);
  String b1 = addLeadingZeros(hexToBin(hex8.substring(2, 4)), 8);
  String b2 = addLeadingZeros(hexToBin(hex8.substring(4, 6)), 8);
  String b3 = addLeadingZeros(hexToBin(hex8.substring(6, 8)), 8);
  String binary = b0 + b1 + b2 + b3;
  if (binary.length() < 32) binary = addLeadingZeros(binary, 32);
  int exponent = binToInt(binary.substring(1, 9)) - 127;
  String mantissa = (exponent <= -1) ? binary.substring(9) : ("1" + binary.substring(9));
  String mantissaEntero = mantissa.substring(0, exponent + 1);
  String mantissaDecimal = mantissa.substring(exponent + 1);
  float numEntero = 0, numDecimal = 0;
  int expE = exponent, expD = -1;
  for (size_t i = 0; i < mantissaEntero.length(); i++) {
    numEntero += (mantissaEntero[i] - '0') * pow(2, expE);
    expE--;
  }
  for (size_t i = 0; i < mantissaDecimal.length(); i++) {
    numDecimal += (mantissaDecimal[i] - '0') * pow(2, expD);
    expD--;
  }
  float sign = (binary[0] == '1') ? -1.0f : 1.0f;
  return sign * (numEntero + numDecimal);
}

#endif
