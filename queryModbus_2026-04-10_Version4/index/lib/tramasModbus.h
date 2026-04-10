#ifndef TRAMAS_MODBUS_H
#define TRAMAS_MODBUS_H

// 34 variables SDM (función 04, 2 registros float cada una). Sin prefijo de dirección.
const char* const VariablesModbus[34] = {
  "0400000002",   // 0  VFA
  "0400020002",   // 1  VFB
  "0400040002",   // 2  VFC
  "0400060002",   // 3  CFA
  "0400080002",   // 4  CFB
  "04000A0002",   // 5  CFC
  "04000C0002",   // 6  PAFA
  "04000E0002",   // 7  PAFB
  "0400100002",   // 8  PAFC
  "04001E0002",   // 9  FPFA
  "0400200002",   // 10 FPFB
  "0400220002",   // 11 FPFC
  "0400340002",   // 12 TSE
  "0400380002",   // 13 TSVa
  "04003C0002",   // 14 TSVar
  "04003E0002",   // 15 TFT
  "0400460002",   // 16 FHz
  "0400480002",   // 17 TImKwh
  "04004A0002",   // 18 TExKwh
  "04004C0002",   // 19 TImKVarh
  "04004E0002",   // 20 TExKVarh
  "0400500002",   // 21 TVah
  "0400520002",   // 22 Ah
  "0400C80002",   // 23 VFAFB
  "0400CA0002",   // 24 VFBFC
  "0400CC0002",   // 25 VFCFA
  "0400E00002",   // 26 NC
  "0400EA0002",   // 27 FALNVTHD
  "0400EC0002",   // 28 FBLNVTHD
  "0400EE0002",   // 29 FCLNVTHD
  "0400F00002",   // 30 FACTHD
  "0400F20002",   // 31 FBCTHD
  "0400F40002",   // 32 FCCTHD
  "0401560002",   // 33 TKWh
};

// Nombres de las 34 variables (índice 0..33)
const char* const NAMES_MODBUS[34] = {
  "VFA", "VFB", "VFC", "CFA", "CFB", "CFC",
  "PAFA", "PAFB", "PAFC", "FPFA", "FPFB", "FPFC",
  "TSE", "TSVa", "TSVar", "TFT", "FHz",
  "TImKwh", "TExKwh", "TImKVarh", "TExKVarh", "TVah", "Ah",
  "VFAFB", "VFBFC", "VFCFA", "NC",
  "FALNVTHD", "FBLNVTHD", "FCLNVTHD", "FACTHD", "FBCTHD", "FCCTHD",
  "TKWh"
};

#define NUM_MODBUS_VARS 34

#endif
