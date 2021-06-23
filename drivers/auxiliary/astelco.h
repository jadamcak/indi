/*******************************************************************************
  Copyright(c) 2021 J.Adamcak for astro.sk. All rights reserved.

  Astelco Driver. Based on Arduino ST4 Driver by Jasem Mutlaq

  For this project: https://github.com/kevinferrare/arduino-st4

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#pragma once

#include "defaultdevice.h"
//#include "indiguiderinterface.h"
#include "indifocuser.h"
#include <stdint.h>
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>

namespace Connection
{
class TCP;
}

class Astelco : public INDI::Focuser
{
  public:
    Astelco();   
    virtual ~Astelco() override;    
    
    void ISGetProperties(const char *dev) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)override ;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)override ;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    const char *getDefaultName() override;
    virtual void TimerHit() override;
    virtual bool Disconnect() override;

    virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;

    IPState MoveAbsFocuser(const float mm);
    IPState MoveRelFocuser(const float mm);

    IPState OffsetAbsFocuser(const float mm);
    IPState OffsetRelFocuser(const float mm);

    bool SetLogin(const char* usr, const char* pas);
    bool SetDevice(const char* device);
    int GetWord(const char* cmd, char *word);
    int GetValue(const char* cmd, char *word);    

  private: 
    bool Handshake_tcp();
    bool sendCommand(const char *cmd, char *resp);
    bool sendCommand(const char *cmd);
    bool readResponse(char *resp);
    int parseAnswer(const char *resp, char *value);
    void setAnswer(const int i, const char *value);
    void thread2();
    bool createThread();
    bool killThread();
    void initHistory(uint8_t val);    

    int PortFD { -1 };

    Connection::TCP *tcpConnection { nullptr };

    const uint8_t ASTELCO_TIMEOUT = 1;
    char loginString[256] = {0};
    char cmdString[256] = {0};
    char cmdDevice[30] = {0};
    int cmdDeviceInt = {0};
    int writeRules = {0};
    int readRules = {0};
    float turnedOn = {0};
    std::atomic<bool> threadOn = {false};
    bool connected = false;
    bool deviceSet = false;
    char *history[1000];
    uint8_t history2[1000];
    uint8_t history3[1000];
    uint8_t history4[1000];
    float ticks_mm = 1000;

    IText LoginT[2];
    ITextVectorProperty LoginTP;
    enum StatusMainE
    {
      UPTIME,
      TELESCOPE_READY,
      STATUS_MAIN_COUNT
    };
    IText StatusMainT[STATUS_MAIN_COUNT];
    ITextVectorProperty StatusMainTP;

    enum StatusE
    {
      POWER_STATE,
      REAL_POSITION,
      LIMIT_STATE,
      MOTION_STATE,
      TARGET_POSITION,
      OFFSET,
      TARGET_DISTANCE,
      STATUS_COUNT
    };
    IText StatusT[STATUS_COUNT];
    ITextVectorProperty StatusTP;

    const char* GetStatusE(StatusE e); 
    enum OnOffE
    {
      ON,
      OFF,
      GET,
      ON_OFF_COUNT
    };

    ISwitch OnOffS[ON_OFF_COUNT];
    ISwitchVectorProperty OnOffSP;

    enum DeviceE
    {
      DOME,
      FOCUS,
      COVER,
      DEVICE_COUNT
    };
    ISwitch DeviceS[DEVICE_COUNT];
    ISwitchVectorProperty DeviceSP;
    
    const char* GetDevice(DeviceE e); 
    INumber TargetPositionN[1];
    INumberVectorProperty TargetPositionNP;
    INumber TargetOffsetN[1];
    INumberVectorProperty TargetOffsetNP;

    ISwitch PositionS[1];
    ISwitchVectorProperty PositionSP;

    enum PositionE
    {
      REAL,
      MIN,
      MAX,
      POSITION_COUNT
    };
    IText PositionT[POSITION_COUNT];
    ITextVectorProperty PositionTP;

    // Current mode of Focus simulator for testing purposes
    enum
    {
      MODE_ALL,
      MODE_ABSOLUTE,
      MODE_RELATIVE,
      MODE_TIMER,
      MODE_COUNT
    };
    ISwitchVectorProperty ModeSP;
    ISwitch ModeS[MODE_COUNT];
    
    bool GetUptime();
    bool OnOff(OnOffE e);
    bool GetStatus(StatusE e, DeviceE dev);
    bool GetPosition(DeviceE dev);
    bool SetPosition(const float pos, DeviceE dev);
    bool SetOffsetPosition(const float pos, DeviceE dev);
    void setHistories(char *txt, uint8_t val = 255, DeviceE dev = DEVICE_COUNT, PositionE pos = POSITION_COUNT);
    float axis[DEVICE_COUNT][POSITION_COUNT];

};
