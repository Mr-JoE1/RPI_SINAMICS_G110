/**
 * copyright (c) 2021, Mohamed Maher
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
 *   @file   left_right_control.cpp
 *   @brief  example for SINAMICS G110 Raspberry Pi library
 *   @author Mohamed Maher
 *   @date   30.07.2021
 */

#include <G110.h>
#include <USS.h>

#define DE_PIN 5
#define NR_SLAVES 2

USS uss;

G110 left;
G110 right;

int main()
{
  const char slaves[NR_SLAVES] = { 0x1, 0x2 };

  quickCommissioning_t motor_data;
  motor_data.powerSetting = POWER_SETTING_EUROPE;
  motor_data.motorVoltage = 230;
  motor_data.motorCurrent = 1.9f;
  motor_data.motorPower = 0.37f;
  motor_data.motorCosPhi = 0.74f;
  motor_data.motorFreq = 50.0f;
  motor_data.motorSpeed = 1390;
  motor_data.motorCooling = MOTOR_COOLING_SELF_COOLED;
  motor_data.motorOverload = 150.0f;
  motor_data.cmdSource = COMMAND_SOURCE_USS;
  motor_data.setpointSource = FREQ_SETPOINT_USS;
  motor_data.minFreq = 0.0f;
  motor_data.maxFreq = 100.0f;
  motor_data.rampupTime = 4.0f;
  motor_data.rampdownTime = 4.0f;
  motor_data.OFF3rampdownTime = 3.0f;
  motor_data.ctlMode = CTL_MODE_V_F_LINEAR;
  motor_data.endQuickComm = END_QUICK_COMM_ONLY_MOTOR_DATA;

  uss.begin("/dev/ttyS0",38400, slaves, NR_SLAVES, DE_PIN);

  sleep(2);

//  left.reset();
  left.begin(&uss, motor_data, 0);
  right.begin(&uss, motor_data, 1);

  left.setParameter(PARAM_NR_PULSE_FREQ_KHZ, (unsigned short) 16);
  left.setParameter(PARAM_NR_ROUNDING_TIME_S, 1.0f);

  right.setParameter(PARAM_NR_PULSE_FREQ_KHZ, (unsigned short) 16);
  right.setParameter(PARAM_NR_ROUNDING_TIME_S, 1.0f);

  left.setFrequency(30.0f);
  left.setON();

  right.setFrequency(35.0f);
  right.setON();

  while(1)
  {
	  uss.send();
	//  Serial.print("receive: ");
	  uss.receive();
	  left.setFrequency(-30.0f);
	  right.setFrequency(-35.0f);
	  sleep(2);
	  left.setOFF1();
	  right.setOFF1();
	  sleep(2);
  }
}




