#ifndef __NEXCONFIG_H__
#define __NEXCONFIG_H__

#include <Arduino.h>

// =========================================================
// CONFIGURAÇÃO DE DEBUG
// =========================================================

// Comente a linha abaixo para desativar mensagens de debug da biblioteca
#define DEBUG_SERIAL_ENABLE

#define dbSerial Serial

#ifdef DEBUG_SERIAL_ENABLE
#define dbSerialPrint(a) dbSerial.print(a)
#define dbSerialPrintln(a) dbSerial.println(a)
#define dbSerialBegin(a) dbSerial.begin(a)
#else
#define dbSerialPrint(a) do {} while (0)
#define dbSerialPrintln(a) do {} while (0)
#define dbSerialBegin(a) do {} while (0)
#endif

#endif