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
#include "indidome.h"
#include <stdint.h>
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>

namespace Connection
{
class TCP;
}

class Astelco : public INDI::Dome
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

    virtual IPState MoveRel(double azDiff) override;
    virtual IPState MoveAbs(double az) override;
    //virtual IPState ControlShutter(ShutterOperation operation) override;
    //virtual bool Abort() override;

    // Parking
    virtual IPState Park() override;
    virtual IPState UnPark() override;
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

    IPState MoveAbsDome_2(const float deg);
    IPState MoveRelDome_2(const float deg);

    IPState OffsetAbsDome_2(const float deg);
    IPState OffsetRelDome_2(const float deg);

    bool MoveRelDome(const float deg);
    bool OffsetRelDome(const float deg);

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
    //float ticks_mm = 1000;
    int parkDefDeg { 0 };
    int parkDeg { parkDefDeg };


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

    ISwitch Goto1S[2];
    ISwitchVectorProperty Goto1SP;
    ISwitch Goto2S[2];
    ISwitchVectorProperty Goto2SP;
    ISwitch Goto3S[2];
    ISwitchVectorProperty Goto3SP;
    
    INumber TargetOffsetN[1];
    INumberVectorProperty TargetOffsetNP;

    ISwitch Offset1S[2];
    ISwitchVectorProperty Offset1SP;
    ISwitch Offset2S[2];
    ISwitchVectorProperty Offset2SP;
    ISwitch Offset3S[2];
    ISwitchVectorProperty Offset3SP;

    ISwitch PositionS[1];
    ISwitchVectorProperty PositionSP;

    enum PositionE
    {
      //REAL,
      MIN,
      MAX,
      POSITION_COUNT
    };
    IText PositionT[POSITION_COUNT];
    ITextVectorProperty PositionTP;

    ISwitch SyncS[ON_OFF_COUNT];
    ISwitchVectorProperty SyncSP;
    IText SyncT[1];
    ITextVectorProperty SyncTP;
    
    
    bool GetUptime();
    bool OnOff(OnOffE e);
    bool GetStatus(StatusE e, DeviceE dev);
    bool GetPosition(DeviceE dev);
    bool SetPosition(const float pos, DeviceE dev);
    bool SetOffsetPosition(const float pos, DeviceE dev);
    void setHistories(char *txt, uint8_t val = 255, DeviceE dev = DEVICE_COUNT, PositionE pos = POSITION_COUNT);
    float axis[DEVICE_COUNT+1][POSITION_COUNT+1];
    bool SetSync(bool set);
    bool GetSync();

};
