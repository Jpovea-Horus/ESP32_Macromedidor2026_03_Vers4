#ifndef CRC_h
#define CRC_h

byte StrtoByte(String strHex) {
  char charArray[3];
  strHex.toCharArray(charArray, 3);
  return (byte)strtoul(charArray, NULL, 16);
}

String CyclicRedundancyCheck(String hexFrame) {
  int numBytes = hexFrame.length() / 2;
  byte byteArray[numBytes];
  for (int i = 0; i < numBytes; i++) {
    String hexByte = hexFrame.substring(2 * i, 2 * i + 2);
    byteArray[i] = StrtoByte(hexByte);
  }
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < numBytes; pos++) {
    crc ^= byteArray[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  String crcHex = String(crc, HEX);
  crcHex.toUpperCase();
  while (crcHex.length() < 4) crcHex = "0" + crcHex;
  return crcHex.substring(2, 4) + crcHex.substring(0, 2);
}

#endif
