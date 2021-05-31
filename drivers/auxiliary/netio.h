/*******************************************************************************
  Copyright(c) 2021 J.Adamcak for astro.sk. All rights reserved.

  NETIO Driver. Based on Arduino ST4 Driver by Jasem Mutlaq

  Telnet driver for NETIO Power Distribution Units (page 27)
  https://www.netio-products.com/files/download/sw/version/NETIO-4x-MANUAL-en_1-3-0.pdf

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
#include <stdint.h>

namespace Connection
{
//class Serial;
class TCP;
}

class Netio : public INDI::DefaultDevice
{
  public:
    Netio();
    
    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)override ;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)override ;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;


  protected:
    const char *getDefaultName() override;
    virtual bool saveConfigItems(FILE *fp) override;
    virtual bool Disconnect() override;
    virtual void TimerHit() override;
     

  private:    
    //bool Handshake_serial();
    bool Handshake_tcp();
    bool sendCommand(const char *cmd, char *resp);
    bool sendCommand(const char *cmd);
    bool GetStatus();
    bool TurnOn(const int i);
    bool TurnOff(const int i);
    void parse(const char *resp); 
    void parse(const char *resp, const int i, const bool b); 

    int PortFD { -1 };

    char netioName[80];
    char netioPass[80];

    //Connection::Serial *serialConnection { nullptr };
    Connection::TCP *tcpConnection { nullptr };

    const uint8_t COM_TIMEOUT = 3;

    ISwitch StatusS[1];
    ISwitchVectorProperty StatusSP;
    enum SocketE
    {
      OFF,
      ON
    };
    IText NameS1T[1];
    ITextVectorProperty NameS1TP;
    ISwitch Socket1S[2];
    ISwitchVectorProperty Socket1SP;
    IText NameS2T[1];
    ITextVectorProperty NameS2TP;
    ISwitch Socket2S[2];
    ISwitchVectorProperty Socket2SP;
    IText NameS3T[1];
    ITextVectorProperty NameS3TP;
    ISwitch Socket3S[2];
    ISwitchVectorProperty Socket3SP;
    IText NameS4T[1];
    ITextVectorProperty NameS4TP;
    ISwitch Socket4S[2];
    ISwitchVectorProperty Socket4SP;
    IText Name1T[1];
    ITextVectorProperty Name1TP;
    IText Pass1T[1];
    ITextVectorProperty Pass1TP;

};
