/*******************************************************************************
  Copyright(c) 2017 Simon Holmbo. All rights reserved.

  Edited by J.Adamcak for astro.sk

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


#ifndef WATEC_H
#define WATEC_H

#include <map>

#include "defaultdevice.h"

std::map<ISState, char> COMMANDS       = { { ISS_ON, 0x53 }, { ISS_OFF, 0x43 } };
std::map<std::string, char> PARAMETERS = { { "MIRROR", 0x31 },
                                           { "LED", 0x32 },
                                           { "THAR", 0x33 },
                                           { "TUNGSTEN", 0x34 } };

namespace Connection
{
class Serial;
class TCP;
}

class Watec : public INDI::DefaultDevice
{
  public:
    Watec();
    ~Watec();

    void ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    //bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    //bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    

  protected:
    const char *getDefaultName();

    bool initProperties();
    bool updateProperties();

    //bool Connect();
    bool Disconnect();

    bool FlipFlopMirror(const int onoff);
    bool SetExposureTime(const int exposure);
    bool SetGain(const int gain);
    bool AddGain(const int inc);

  private:
    int PortFD { -1 }; // file descriptor for serial port

    // Main Control
    ISwitchVectorProperty MirrorSP;
    ISwitch MirrorS[2];

    ISwitch ExposureS[16];
    ISwitchVectorProperty ExposureSP;

    ISwitch GainS[11];
    ISwitchVectorProperty GainSP;

    INumber GainN[1];
    INumberVectorProperty GainNP;
    ISwitch Gain1S[2];
    ISwitchVectorProperty Gain1SP;
    ISwitch Gain2S[2];
    ISwitchVectorProperty Gain2SP;
    ISwitch Gain3S[2];
    ISwitchVectorProperty Gain3SP;

    bool Handshake_serial();
    bool Handshake_tcp();
    bool sendCommand(const char *cmd);
    void helper();

    Connection::Serial *serialConnection { nullptr };
    Connection::TCP *tcpConnection { nullptr };

    const uint8_t WATEC_TIMEOUT = 3;
    

    /*/ Options
    ITextVectorProperty PortTP;
    IText PortT[1] {};
*/
};

#endif // WATEC_H
