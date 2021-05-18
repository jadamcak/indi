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

namespace Connection
{
class TCP;
}

class Astelco : public INDI::DefaultDevice//, public INDI::Focuser
{
  public:
    Astelco();

    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)override ;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)override ;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    const char *getDefaultName() override;
    virtual void TimerHit() override;
    virtual bool Disconnect() override;

    bool SetPosition(const float pos);
    bool SetLogin(const char* usr, const char* pas);
    bool SetDevice(const char* device);
    int GetWord(const char* cmd, char *word);
    bool GetPosition(char *real, char *min_real, char *max_real);

  private: 
    bool Handshake_tcp();
    bool sendCommand(const char *cmd, char *resp);
    bool sendCommand(const char *cmd);
    

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
    bool connected = false;

    IText LoginT[2];
    ITextVectorProperty LoginTP;
    enum StatusE
    {
      UPTIME,
      TELESCOPE_READY,
      POWER_STATE,
      REAL_POSITION,
      LIMIT_STATE,
      MOTION_STATE,
      TARGET_POSITION,
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
    
    bool GetUptime();
    bool OnOff(OnOffE e);
    bool GetStatus(StatusE e);
};
