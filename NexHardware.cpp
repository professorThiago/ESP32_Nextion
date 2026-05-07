/**
 * @file NexHardware.cpp
 *
 * The implementation of base API for using Nextion. 
 *
 * @author  Wu Pengfei (email:<pengfei.wu@itead.cc>)
 * @date    2015/8/11
 * @copyright 
 * Copyright (C) 2014-2015 ITEAD Intelligent Systems Co., Ltd. \n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include "NexHardware.h"

#define NEX_RET_CMD_FINISHED            (0x01)
#define NEX_RET_EVENT_LAUNCHED          (0x88)
#define NEX_RET_EVENT_UPGRADED          (0x89)
#define NEX_RET_EVENT_TOUCH_HEAD            (0x65)     
#define NEX_RET_EVENT_POSITION_HEAD         (0x67)
#define NEX_RET_EVENT_SLEEP_POSITION_HEAD   (0x68)
#define NEX_RET_CURRENT_PAGE_ID_HEAD        (0x66)
#define NEX_RET_STRING_HEAD                 (0x70)
#define NEX_RET_NUMBER_HEAD                 (0x71)
#define NEX_RET_INVALID_CMD             (0x00)
#define NEX_RET_INVALID_COMPONENT_ID    (0x02)
#define NEX_RET_INVALID_PAGE_ID         (0x03)
#define NEX_RET_INVALID_PICTURE_ID      (0x04)
#define NEX_RET_INVALID_FONT_ID         (0x05)
#define NEX_RET_INVALID_BAUD            (0x11)
#define NEX_RET_INVALID_VARIABLE        (0x1A)
#define NEX_RET_INVALID_OPERATION       (0x1B)




static HardwareSerial *nexSerialPtr = &Serial1;

static uint32_t nexBaudrateAtual = 9600;
static int8_t nexRxPinAtual = -1;
static int8_t nexTxPinAtual = -1;

HardwareSerial *getNexSerial()
{
    return nexSerialPtr;
}

void setNexSerial(HardwareSerial *serial)
{
    if (serial != nullptr)
    {
        nexSerialPtr = serial;
    }
}

void nexEnd()
{
    if (nexSerialPtr != nullptr)
    {
        nexSerialPtr->end();
    }
}



/*
 * Receive uint32_t data. 
 * 
 * @param number - save uint32_t data. 
 * @param timeout - set timeout time. 
 *
 * @retval true - success. 
 * @retval false - failed.
 *
 */
bool recvRetNumber(uint32_t *number, uint32_t timeout)
{
    bool ret = false;
    uint8_t temp[8] = {0};

    if (!number)
    {
        goto __return;
    }
    
    getNexSerial()->setTimeout(timeout);
    if (sizeof(temp) != getNexSerial()->readBytes((char *)temp, sizeof(temp)))
    {
        goto __return;
    }

    if (temp[0] == NEX_RET_NUMBER_HEAD
        && temp[5] == 0xFF
        && temp[6] == 0xFF
        && temp[7] == 0xFF
        )
    {
        *number = ((uint32_t)temp[4] << 24) | ((uint32_t)temp[3] << 16) | (temp[2] << 8) | (temp[1]);
        ret = true;
    }

__return:

    if (ret) 
    {
        dbSerialPrint("recvRetNumber :");
        dbSerialPrintln(*number);
    }
    else
    {
        dbSerialPrintln("recvRetNumber err");
    }
    
    return ret;
}


/*
 * Receive string data. 
 * 
 * @param buffer - save string data. 
 * @param len - string buffer length. 
 * @param timeout - set timeout time. 
 *
 * @return the length of string buffer.
 *
 */
uint16_t recvRetString(char *buffer, uint16_t len, uint32_t timeout)
{
    uint16_t ret = 0;
    bool str_start_flag = false;
    uint8_t cnt_0xff = 0;
    String temp = String("");
    uint8_t c = 0;
    long start;

    if (!buffer || len == 0)
    {
        goto __return;
    }
    
    start = millis();
    while (millis() - start <= timeout)
    {
        while (getNexSerial()->available())
        {
            c = getNexSerial()->read();
            if (str_start_flag)
            {
                if (0xFF == c)
                {
                    cnt_0xff++;                    
                    if (cnt_0xff >= 3)
                    {
                        break;
                    }
                }
                else
                {
                    temp += (char)c;
                }
            }
            else if (NEX_RET_STRING_HEAD == c)
            {
                str_start_flag = true;
            }
        }
        
        if (cnt_0xff >= 3)
        {
            break;
        }
    }

    ret = temp.length();
    ret = ret > len ? len : ret;
    strncpy(buffer, temp.c_str(), ret);
    
__return:

    dbSerialPrint("recvRetString[");
    dbSerialPrint(temp.length());
    dbSerialPrint(",");
    dbSerialPrint(temp);
    dbSerialPrintln("]");

    return ret;
}

/*
 * Send command to Nextion.
 *
 * @param cmd - the string of command.
 */
void sendCommand(const char* cmd)
{
    while (getNexSerial()->available())
    {
        getNexSerial()->read();
    }
    
    getNexSerial()->print(cmd);
    getNexSerial()->write(0xFF);
    getNexSerial()->write(0xFF);
    getNexSerial()->write(0xFF);
}


/*
 * Command is executed successfully. 
 *
 * @param timeout - set timeout time.
 *
 * @retval true - success.
 * @retval false - failed. 
 *
 */
bool recvRetCommandFinished(uint32_t timeout)
{    
    bool ret = false;
    uint8_t temp[4] = {0};
    
    getNexSerial()->setTimeout(timeout);
    if (sizeof(temp) != getNexSerial()->readBytes((char *)temp, sizeof(temp)))
    {
        ret = false;
    }

    if (temp[0] == NEX_RET_CMD_FINISHED
        && temp[1] == 0xFF
        && temp[2] == 0xFF
        && temp[3] == 0xFF
        )
    {
        ret = true;
    }

    if (ret) 
    {
        dbSerialPrintln("recvRetCommandFinished ok");
    }
    else
    {
        dbSerialPrintln("recvRetCommandFinished err");
    }
    
    return ret;
}


bool nexInit(uint32_t baudrate, int8_t rxPin, int8_t txPin, HardwareSerial *serial)
{
    bool ret1 = false;
    bool ret2 = false;

    nexBaudrateAtual = baudrate;
    nexRxPinAtual = rxPin;
    nexTxPinAtual = txPin;

    dbSerialBegin(115200);

    setNexSerial(serial);

    if (getNexSerial() == nullptr)
    {
        dbSerialPrintln("Erro: serial do Nextion nao configurada.");
        return false;
    }

#if defined(ESP32)
    if (rxPin >= 0 && txPin >= 0)
    {
        getNexSerial()->begin(baudrate, SERIAL_8N1, rxPin, txPin);
    }
    else
    {
        getNexSerial()->begin(baudrate);
    }
#else
    getNexSerial()->begin(baudrate);
#endif

    delay(100);

    while (getNexSerial()->available())
    {
        getNexSerial()->read();
    }

    sendCommand("");

    // bkcmd=1 faz o Nextion responder se o comando foi executado.
    // Isso ajuda a testar se a comunicação está funcionando.
    sendCommand("bkcmd=1");
    ret1 = recvRetCommandFinished(300);

    sendCommand("page 0");
    ret2 = recvRetCommandFinished(300);

    if (ret1 && ret2)
    {
        dbSerialPrintln("Nextion iniciado com sucesso.");
    }
    else
    {
        dbSerialPrintln("Falha ao iniciar Nextion.");
    }

    return ret1 && ret2;
}

void nexLoop(NexTouch *nex_listen_list[])
{
    static uint8_t __buffer[10];
    
    uint16_t i;
    uint8_t c;  
    
    while (getNexSerial()->available() > 0)
    {   
        delay(10);
        c = getNexSerial()->read();
        
        if (NEX_RET_EVENT_TOUCH_HEAD == c)
        {
            if (getNexSerial()->available() >= 6)
            {
                __buffer[0] = c;  
                for (i = 1; i < 7; i++)
                {
                    __buffer[i] = getNexSerial()->read();
                }
                __buffer[i] = 0x00;
                
                if (0xFF == __buffer[4] && 0xFF == __buffer[5] && 0xFF == __buffer[6])
                {
                    NexTouch::iterate(nex_listen_list, __buffer[1], __buffer[2], (int32_t)__buffer[3]);
                }
                
            }
        }
    }
}

void nexBegin(uint32_t baudrate)
{
#if defined(ESP32)
    if (nexRxPinAtual >= 0 && nexTxPinAtual >= 0)
    {
        getNexSerial()->begin(baudrate, SERIAL_8N1, nexRxPinAtual, nexTxPinAtual);
    }
    else
    {
        getNexSerial()->begin(baudrate);
    }
#else
    getNexSerial()->begin(baudrate);
#endif

    nexBaudrateAtual = baudrate;
}

