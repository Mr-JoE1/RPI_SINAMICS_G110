/**
 * Copyright (c) 2020, Merlin Kr�mmel - Created G110 C++ for Arduino
 * https://github.com/zocker007
 *
 * Copyright (c) 2021, Mohamed Maher - wrapped the lib for Raspberry Pi
 * https://github.com/Mr-JoE1
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

/**
 * @section LICENSE
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 *   @file   USS.cpp
 *   @brief  class definition for Siemens USS protocol, implements the low level
 *           functionality of communication protocol as baselayer for higher layers
 *           like inverters.
 *   @author Merlin Kr�mmel
 *   @date   22.07.2020
 *   @author Mohamed Maher
 *   @date   30.07.2021
 */
#include "USS.h"

extern USS uss;

USS::USS() :
    m_slaves{0},
    m_nrSlaves(0),
    m_actualSlave(0),
    m_sendBuffer{0},
    m_recvBuffer{0},
    m_mainsetpoint{0},
    m_mainactualvalue{0},
    m_ctlword{0},
    m_statusword{0},
    m_paramValue{{0}, {0}},
    m_nextSend(0),
    m_period(0),
    m_characterRuntime(0),
    m_dePin(-1),
	rs_begin(0),
	rs_end(0)
{
    m_sendBuffer[0] = STX_BYTE_STX;
    m_sendBuffer[1] = (PKW_LENGTH_CHARACTERS * PKW_ANZ) + (PZD_LENGTH_CHARACTERS * PZD_ANZ) + 2; // 2 for ADR and BCC bytes
}

int USS::begin(char *sertty, unsigned int speed, const char slaves[], const int nrSlaves, const int dePin)
{
    int telegramRuntime;
    rs_begin= clock();
    if(nrSlaves > USS_SLAVES || slaves == nullptr)
        return -1;
    memcpy(m_slaves, slaves, nrSlaves);

    m_nrSlaves = nrSlaves;
    m_dePin = dePin;
    m_characterRuntime = CHARACTER_RUNTIME_BASE_US / (speed / BAUDRATE_BASE);
    telegramRuntime = USS_BUFFER_LENGTH * m_characterRuntime * 1.5f / 1000;
    serOpen(sertty,speed, 0);
    //Serial1.setTimeout(telegramRuntime + MAX_RESP_DELAY_TIME_MS);
    gpioSetMode(m_dePin,PI_OUTPUT);
    gpioWrite(m_dePin, 1);
    m_period = telegramRuntime * 2 + (START_DELAY_LENGTH_CHARACTERS * m_characterRuntime / 1000) + MAX_RESP_DELAY_TIME_MS + MASTER_COMPUTE_DELAY_MS;

    return 0;
}

double USS::millis(double start,double stop )
{
	double diff= stop - start;
	return diff;
}

int USS::setParameter(const uint16_t param, const uint16_t value, const int slaveIndex)
{
    int ret = 0;

    if(slaveIndex >= m_nrSlaves)
        return -1;

    m_paramValue[0][slaveIndex] = (param & PKE_WORD_PARAM_MASK) | PKE_WORD_AK_CHW_PWE;
    m_paramValue[1][slaveIndex] = 0;
    m_paramValue[2][slaveIndex] = 0;
    m_paramValue[3][slaveIndex] = value;

    while(m_paramValue[0][slaveIndex] != PARAM_VALUE_EMPTY)
    {
        send();
        ret = receive();
    }

    return ret;
}

int USS::setParameter(const uint16_t param, const uint32_t value, const int slaveIndex)
{
    int ret = 0;

    if(slaveIndex >= m_nrSlaves)
        return -1;

    m_paramValue[0][slaveIndex] = (param & PKE_WORD_PARAM_MASK) | PKE_WORD_AK_CHD_PWE;
    m_paramValue[1][slaveIndex] = 0;
    m_paramValue[2][slaveIndex] = (value >> 16) & 0xFFFF;
    m_paramValue[3][slaveIndex] = value & 0xFFFF;

    while(m_paramValue[0][slaveIndex] != PARAM_VALUE_EMPTY)
    {
        send();
        ret = receive();
    }

    return ret;
}

int USS::setParameter(const uint16_t param, const float value, const int slaveIndex)
{
    parameter_t p;

    p.f32 = value;

    return setParameter(param, p.u32, slaveIndex);
}

void USS::setMainsetpoint(const uint16_t value, const int slaveIndex)
{
    if(slaveIndex >= m_nrSlaves)
        return;

    m_mainsetpoint[slaveIndex] = value;
}

void USS::setCtlFlag(const uint16_t flags, const int slaveIndex)
{
    if(slaveIndex >= m_nrSlaves)
        return;

    m_ctlword[slaveIndex] |= flags;
}

void USS::clearCtlFlag(const uint16_t flags, const int slaveIndex)
{
    if(slaveIndex >= m_nrSlaves)
        return;

    m_ctlword[slaveIndex] &= ~flags;
}

uint16_t USS::getActualvalue(const int slaveIndex) const
{
    if(slaveIndex >= m_nrSlaves)
        return -1;

    uint16_t ret;

    ret = m_mainactualvalue[slaveIndex];

    return ret;
}

bool USS::checkStatusFlag(const uint16_t flag, const int slaveIndex) const
{
    if(slaveIndex >= m_nrSlaves)
        return false;

    return (m_statusword[slaveIndex] & flag) != 0 ? true : false;
}

byte USS::BCC(const char buffer[], const int length) const
{
    byte ret = 0;

    for(int i = 0; i < length; i++)
        ret = ret ^ buffer[i];

    return ret;
}

void USS::send()
{
    while(!(millis(rs_begin,rs_end) > m_nextSend && (millis(rs_begin,rs_end) - m_nextSend) < 10000));

    m_nextSend = millis(rs_begin,rs_end) + m_period;

    if(m_actualSlave == m_nrSlaves)
        m_actualSlave = 0;

    m_sendBuffer[2] = m_slaves[m_actualSlave] & ADDR_BYTE_ADDR_MASK;

    if(m_paramValue[0][m_actualSlave] != PARAM_VALUE_EMPTY)
    {
        m_sendBuffer[3] = (m_paramValue[0][m_actualSlave] >> 8) & 0xFF;
        m_sendBuffer[4] = m_paramValue[0][m_actualSlave] & 0xFF;
        m_sendBuffer[5] = (m_paramValue[1][m_actualSlave] >> 8) & 0xFF;
        m_sendBuffer[6] = m_paramValue[1][m_actualSlave] & 0xFF;
        m_sendBuffer[7] = (m_paramValue[2][m_actualSlave] >> 8) & 0xFF;
        m_sendBuffer[8] = m_paramValue[2][m_actualSlave] & 0xFF;
        m_sendBuffer[9] = (m_paramValue[3][m_actualSlave] >> 8) & 0xFF;
        m_sendBuffer[10] = m_paramValue[3][m_actualSlave] & 0xFF;
    }
    else
    {
        m_sendBuffer[3] = 0;
        m_sendBuffer[4] = 0;
        m_sendBuffer[5] = 0;
        m_sendBuffer[6] = 0;
        m_sendBuffer[7] = 0;
        m_sendBuffer[8] = 0;
        m_sendBuffer[9] = 0;
        m_sendBuffer[10] = 0;
    }

    m_sendBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 3] = (m_ctlword[m_actualSlave] >> 8) & 0xFF;
    m_sendBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 4] = m_ctlword[m_actualSlave] & 0xFF;
    m_sendBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 5] = (m_mainsetpoint[m_actualSlave] >> 8) & 0xFF;
    m_sendBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 6] = m_mainsetpoint[m_actualSlave] & 0xFF;

    m_sendBuffer[USS_BUFFER_LENGTH - 1] = BCC(m_sendBuffer, USS_BUFFER_LENGTH - 1);

    serWrite(0, m_sendBuffer, USS_BUFFER_LENGTH);
    //you need to log error here in case send data failed !!!!!

    usleep(START_DELAY_LENGTH_CHARACTERS * m_characterRuntime);
    rs_end= clock()
	gpioWrite(m_dePin, 0);
}

int USS::receive()
{
    int ret = 0;

    if(serRead(0,m_recvBuffer, USS_BUFFER_LENGTH) == USS_BUFFER_LENGTH &&
        m_recvBuffer[0] == STX_BYTE_STX && (m_recvBuffer[2] & ADDR_BYTE_ADDR_MASK) == (m_slaves[m_actualSlave] & ADDR_BYTE_ADDR_MASK) &&
        BCC(m_recvBuffer, USS_BUFFER_LENGTH - 1) == m_recvBuffer[USS_BUFFER_LENGTH - 1])
    {
        m_statusword[m_actualSlave] = (m_recvBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 3] << 8) & 0xFF00;
        m_statusword[m_actualSlave] |= m_recvBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 4] & 0xFF;
        m_mainactualvalue[m_actualSlave] = (m_recvBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 5] << 8) & 0xFF00;
        m_mainactualvalue[m_actualSlave] |= m_recvBuffer[PKW_LENGTH_CHARACTERS * PKW_ANZ + 6] & 0xFF;

        if(m_paramValue[0][m_actualSlave] != PARAM_VALUE_EMPTY)
        {
            if(((m_recvBuffer[3] << 8) & PKE_WORD_AK_MASK) == PKE_WORD_AK_NO_RESP)
                ret = -1;
            if(((m_recvBuffer[3] << 8) & PKE_WORD_AK_MASK) == PKE_WORD_AK_NO_RIGHTS)
                ret = -2;
            if(((m_recvBuffer[3] << 8) & PKE_WORD_AK_MASK) == PKE_WORD_AK_CANT_EXECUTE)
            {
                ret = m_recvBuffer[10] | m_recvBuffer[9] << 8;

                if(!ret)
                    ret = -3;   // 0 is error code for illegal parameter number
            }

            m_paramValue[0][m_actualSlave] = PARAM_VALUE_EMPTY;
        }
    }
    else
    {
        ret = -1;
    }

    gpioWrite(m_dePin, 1);
    m_actualSlave++;

    return ret;
}
