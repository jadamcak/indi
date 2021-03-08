/*******************************************************************************
  Copyright(c) 2019 Richard Komzik. All rights reserved.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <memory>

#include "watec_remote.h"
#include "watec_config.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"


//
#define MAX_CCD_EXP            1000  /* Max CCD exposure time in seconds */


const char *CAMERA_SETTINGS_TAB = "FIGU Settings";
//const char *CALIBRATION_UNIT_TAB      = "Calibration Unit";

std::unique_ptr<Watec>
    watec(new Watec()); // create std:unique_ptr (smart pointer) to  our spectrograph object

void ISGetProperties(const char *dev)
{
    watec->ISGetProperties(dev);
}

/* The next 4 functions are executed when the indiserver requests a change of
 * one of the properties. We pass the request on to our spectrograph object.
 */
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    watec->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    watec->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    watec->ISNewNumber(dev, name, values, names, num);
}
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

/* This function is fired when a property we are snooping on is changed. We
 * pass it on to our spectrograph object.
 */
void ISSnoopDevice(XMLEle *root)
{
    watec->ISSnoopDevice(root);
}

Watec::Watec()
{
    PortFD = -1;


    setVersion(WATEC_VERSION_MAJOR, WATEC_VERSION_MINOR);
}

Watec::~Watec()
{
}

/* Returns the name of the device. */
const char *Watec::getDefaultName()
{
    return (char *)"Watec Remote";
}

/* Initialize and setup all properties on startup. */
bool Watec::initProperties()
{
    INDI::DefaultDevice::initProperties();

    //--------------------------------------------------------------------------------
    // Camera Settings
    //--------------------------------------------------------------------------------
    // a switch to select Exposure Time
    IUFillSwitch(&ExposureS[0],  "EXPTIM_7", "  5.12 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[1],  "EXPTIM_8", "  2.56 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[2],  "EXPTIM_9", "  1.28 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[3],  "EXPTIM_A", "  0.64 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[4],  "EXPTIM_B", "  0.32 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[5],  "EXPTIM_C", "  0.16 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[6],  "EXPTIM_D", "  0.08 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[7],  "EXPTIM_E", "  1/25 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[8],  "EXPTIM_F", "  1/50 sec", ISS_ON);
    IUFillSwitch(&ExposureS[9],  "EXPTIM_0", " *1/50 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[10], "EXPTIM_1", "  1/60 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[11], "EXPTIM_2", " 1/125 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[12], "EXPTIM_3", " 1/250 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[13], "EXPTIM_4", " 1/500 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[14], "EXPTIM_5", "1/1000 sec", ISS_OFF);
    IUFillSwitch(&ExposureS[15], "EXPTIM_6", "1/2000 sec", ISS_OFF);
    IUFillSwitchVector(&ExposureSP, ExposureS, 16, getDeviceName(), "EXPOSURE_TIME", "Exposure Time", 
                       CAMERA_SETTINGS_TAB, IP_WO, ISR_1OFMANY, 0, IPS_IDLE);

    // a switch to select Gain
    IUFillSwitch(&GainS[0],  "GAIN_0", "  0% ", ISS_OFF);
    IUFillSwitch(&GainS[1],  "GAIN_1", " 10% ", ISS_OFF);
    IUFillSwitch(&GainS[2],  "GAIN_2", " 20% ", ISS_OFF);
    IUFillSwitch(&GainS[3],  "GAIN_3", " 30% ", ISS_OFF);
    IUFillSwitch(&GainS[4],  "GAIN_4", " 40% ", ISS_OFF);
    IUFillSwitch(&GainS[5],  "GAIN_5", " 50% ", ISS_ON);
    IUFillSwitch(&GainS[6],  "GAIN_6", " 60% ", ISS_OFF);
    IUFillSwitch(&GainS[7],  "GAIN_7", " 70% ", ISS_OFF);
    IUFillSwitch(&GainS[8],  "GAIN_8", " 80% ", ISS_OFF);
    IUFillSwitch(&GainS[9],  "GAIN_9", " 90% ", ISS_OFF);
    IUFillSwitch(&GainS[10], "GAIN_A", "100% ", ISS_OFF);
    IUFillSwitchVector(&GainSP, GainS, 11, getDeviceName(), "GAIN", "Gain", 
                       CAMERA_SETTINGS_TAB, IP_WO, ISR_ATMOST1, 0, IPS_IDLE);

    //another switch for gain
    IUFillNumber(&GainN[0], "GAIN VALUE", "Value", "%3.0f %%", 0., 100., 0., 0.);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "GAIN VAL", "Gain", 
                        CAMERA_SETTINGS_TAB, IP_RO, 0, IPS_IDLE);
    IUFillSwitch(&Gain1S[0], "-25%", "-25%", ISS_OFF);
    IUFillSwitch(&Gain1S[1], "+25%", "+25%", ISS_OFF);
    IUFillSwitchVector(&Gain1SP, Gain1S, 2, getDeviceName(), "GAIN 25", "Gain +/- 25%", 
                        CAMERA_SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    
    IUFillSwitch(&Gain2S[0], "-10%", "-10%", ISS_OFF);
    IUFillSwitch(&Gain2S[1], "+10%", "+10%", ISS_OFF);
    IUFillSwitchVector(&Gain2SP, Gain2S, 2, getDeviceName(), "GAIN 10", "Gain +/- 10%", 
                        CAMERA_SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    IUFillSwitch(&Gain3S[0], "-1%", "-1%", ISS_OFF);    
    IUFillSwitch(&Gain3S[1], "+1%", "+1%", ISS_OFF);
    IUFillSwitchVector(&Gain3SP, Gain3S, 2, getDeviceName(), "GAIN REL", "Gain +/- 1%", 
                        CAMERA_SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    //--------------------------------------------------------------------------------
    // Calibration Unit
    //--------------------------------------------------------------------------------

    // setup the mirror switch
    IUFillSwitch(&MirrorS[0], "ACTIVATED", "Activated", ISS_ON);
    IUFillSwitch(&MirrorS[1], "DEACTIVATED", "Deactivated", ISS_OFF);
    IUFillSwitchVector(&MirrorSP, MirrorS, 2, getDeviceName(), "FLIP_MIRROR", "Flip mirror via RasPI", CAMERA_SETTINGS_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);


    //--------------------------------------------------------------------------------
    // Options
    //--------------------------------------------------------------------------------
    addDebugControl();
    addSimulationControl();
    addConfigurationControl();
    // setup the text input for the serial port
    // std::string portID("This is a string.");
    // portID = std::string("GPIO (LSB-MSB): ");
    // IUFillText(&PortT[0], "PORT", "Port", portID.c_str());
    // IUFillTextVector(&PortTP, PortT, 1, getDeviceName(), "DEVICE_PORT", "Ports", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);
    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]() { return Handshake_serial(); });
    serialConnection->setDefaultBaudRate(Connection::Serial::B_57600);
    // Arduino default port
    serialConnection->setDefaultPort("/dev/ttyUSB0");
    registerConnection(serialConnection);

    tcpConnection = new Connection::TCP(this);
    tcpConnection->registerHandshake([&]() { return Handshake_tcp(); });
    tcpConnection->setDefaultPort(23);
    tcpConnection->setConnectionType(Connection::TCP::ConnectionType::TYPE_TCP);
    registerConnection(tcpConnection);

    return true;
}

void Watec::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
//    defineText(&PortTP);
//    loadConfig(true, PortTP.name);
}

bool Watec::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    if (isConnected())
    {
        // create properties if we are connected
        defineSwitch(&ExposureSP);
        defineSwitch(&GainSP);
        defineNumber(&GainNP);
        defineSwitch(&Gain1SP);
        defineSwitch(&Gain2SP);
        defineSwitch(&Gain3SP);
        defineSwitch(&MirrorSP);
    }
    else
    {
        // delete properties if we aren't connected
        deleteProperty(ExposureSP.name);
        deleteProperty(GainSP.name);
        deleteProperty(GainNP.name);
        deleteProperty(Gain1SP.name);
        deleteProperty(Gain2SP.name);
        deleteProperty(Gain3SP.name);
        deleteProperty(MirrorSP.name);
    }
    return true;
}

void Watec::helper()
{
    sendCommand("START#");  
    SetExposureTime('F');
    SetGain('5');
    FlipFlopMirror(1);
    LOGF_INFO("%s is online.", getDeviceName());
}

bool Watec::Handshake_serial()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfuly to simulated %s.", getDeviceName());
        return true;
    }

    PortFD = serialConnection->getPortFD();
    
    helper();

    return true;
}

bool Watec::Handshake_tcp()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfuly to simulated %s.", getDeviceName());
        return true;
    }

    PortFD = tcpConnection->getPortFD();  

    helper();

    return true;
}

bool Watec::Disconnect()
{
    sendCommand("STOP#");

    LOGF_INFO("%s is offline.", getDeviceName());

    return INDI::DefaultDevice::Disconnect();
}

/* Handle a request to change a switch. */
bool Watec::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (!strcmp(dev, getDeviceName())) // check if the message is for our device
    {
        /* Exposure Time */
        // prikazy na prestavenie Watec Exposure Time
        if (!strcmp(name, ExposureSP.name))
        {
            if (IUUpdateSwitch(&ExposureSP, states, names, n) < 0)
                return false;

            if (ExposureS[0].s == ISS_ON)
            {
                if( ! SetExposureTime('7') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[1].s == ISS_ON)
            {
                if( ! SetExposureTime('8') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[2].s == ISS_ON)
            {
                if( ! SetExposureTime('9') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[3].s == ISS_ON)
            {
                if( ! SetExposureTime('A') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[4].s == ISS_ON)
            {
                if( ! SetExposureTime('B') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[5].s == ISS_ON)
            {
                if( ! SetExposureTime('C') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[6].s == ISS_ON)
            {
                if( ! SetExposureTime('D') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[7].s == ISS_ON)
            {
                if( ! SetExposureTime('E') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[8].s == ISS_ON)
            {
                if( ! SetExposureTime('F') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[9].s == ISS_ON)
            {
                if( ! SetExposureTime('0') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[10].s == ISS_ON)
            {
                if( ! SetExposureTime('1') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[11].s == ISS_ON)
            {
                if( ! SetExposureTime('2') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[12].s == ISS_ON)
            {
                if( ! SetExposureTime('3') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[13].s == ISS_ON)
            {
                if( ! SetExposureTime('4') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[14].s == ISS_ON)
            {
                if( ! SetExposureTime('5') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }
            else if (ExposureS[15].s == ISS_ON)
            {
                if( ! SetExposureTime('6') )
                {
                    IUResetSwitch(&ExposureSP);
                    ExposureSP.s = IPS_ALERT;
                    LOG_ERROR(" SetExposureTime failed. ");
                    IDSetSwitch(&ExposureSP, nullptr);
                    return false;
                }
            }

            ExposureSP.s = IPS_OK;
            IDSetSwitch(&ExposureSP, nullptr);
            return true;
        }

        /* Gain */
        // prikazy na prestavenie Watec Gain
        if (!strcmp(name, GainSP.name))
        {
            if (IUUpdateSwitch(&GainSP, states, names, n) < 0)
                return false;

            if (GainS[0].s == ISS_ON)
            {
                if( ! SetGain('0') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[1].s == ISS_ON)
            {
                if( ! SetGain('1') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[2].s == ISS_ON)
            {
                if( ! SetGain('2') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[3].s == ISS_ON)
            {
                if( ! SetGain('3') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[4].s == ISS_ON)
            {
                if( ! SetGain('4') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[5].s == ISS_ON)
            {
                if( ! SetGain('5') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[6].s == ISS_ON)
            {
                if( ! SetGain('6') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[7].s == ISS_ON)
            {
                if( ! SetGain('7') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[8].s == ISS_ON)
            {
                if( ! SetGain('8') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[9].s == ISS_ON)
            {
                if( ! SetGain('9') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }
            else if (GainS[10].s == ISS_ON)
            {
                if( ! SetGain('A') )
                {
                    IUResetSwitch(&GainSP);
                    GainSP.s = IPS_ALERT;
                    LOG_ERROR(" SetGain failed. ");
                    IDSetSwitch(&GainSP, nullptr);
                    return false;
                }
            }

            GainSP.s = IPS_OK;
            IDSetSwitch(&GainSP, nullptr);
            return true;
        }

        /* Gain 2*/
        // prikazy na prestavenie Watec Gain
        if (!strcmp(name, Gain1SP.name))
        {
            if (IUUpdateSwitch(&Gain1SP, states, names, n) < 0)
                return false;

            if (Gain1S[0].s == ISS_ON)
            {
                if( ! AddGain(-25) )
                {
                    IUResetSwitch(&Gain1SP);
                    Gain1SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain1SP, nullptr);
                    return false;
                }
            }
            else if (Gain1S[1].s == ISS_ON)
            {
                if( ! AddGain(+25) )
                {
                    IUResetSwitch(&Gain1SP);
                    Gain1SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain1SP, nullptr);
                    return false;
                }
            }
            IUResetSwitch(&Gain1SP);         
            Gain1SP.s = IPS_OK;
            IDSetSwitch(&Gain1SP, nullptr);
            return true;
        }
        
        if (!strcmp(name, Gain2SP.name))
        {
            if (IUUpdateSwitch(&Gain2SP, states, names, n) < 0)
                return false;

            if (Gain2S[0].s == ISS_ON)
            {
                if( ! AddGain(-10) )
                {
                    IUResetSwitch(&Gain2SP);
                    Gain2SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain2SP, nullptr);
                    return false;
                }
            }
            else if (Gain2S[1].s == ISS_ON)
            {
                if( ! AddGain(+10) )
                {
                    IUResetSwitch(&Gain2SP);
                    Gain2SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain2SP, nullptr);
                    return false;
                }
            }
            IUResetSwitch(&Gain2SP);
            Gain2SP.s = IPS_OK;
            IDSetSwitch(&Gain2SP, nullptr);
            return true;
        }

        if (!strcmp(name, Gain3SP.name))
        {
            if (IUUpdateSwitch(&Gain3SP, states, names, n) < 0)
                return false;

            if (Gain3S[0].s == ISS_ON)
            {
                if( ! AddGain(-1) )
                {
                    IUResetSwitch(&Gain3SP);
                    Gain3SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain3SP, nullptr);
                    return false;
                }
            }
            else if (Gain3S[1].s == ISS_ON)
            {
                if( ! AddGain(+1) )
                {
                    IUResetSwitch(&Gain3SP);
                    Gain3SP.s = IPS_ALERT;
                    LOG_ERROR(" AddGain failed. ");
                    IDSetSwitch(&Gain3SP, nullptr);
                    return false;
                }
            }
            IUResetSwitch(&Gain3SP);
            Gain3SP.s = IPS_OK;
            IDSetSwitch(&Gain3SP, nullptr);
            return true;
        }

        /* Mirror */
        if (!strcmp(MirrorSP.name, name)) // check if its a mirror request
        {
            ISState s = MirrorS[0].s;                    // save current mirror state
            IUUpdateSwitch(&MirrorSP, states, names, n); // update mirror
            MirrorSP.s = IPS_OK;                         // set state to ok (change later if something goes wrong)
            if (s != MirrorS[0].s)
            { // if mirror state has changed send command
               bool rc = false;
               if ( MirrorS[0].s == ISS_ON )
                rc = FlipFlopMirror(1);
               else if ( MirrorS[0].s == ISS_OFF )
                rc = FlipFlopMirror(0);
               if (!rc)
                    MirrorSP.s = IPS_ALERT;
            }
            IDSetSwitch(&MirrorSP, nullptr); // tell clients to update
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n); // send it to the parent classes
}

/* Handle a request to change text. /
bool Watec::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (!strcmp(dev, getDeviceName())) // check if the message is for our device
    {
        if (!strcmp(PortTP.name, name)) //check if is a port change request
        {
            IUUpdateText(&PortTP, texts, names, n); // update port
            PortTP.s = IPS_OK;                      // set state to ok
            IDSetText(&PortTP, nullptr);               // tell clients to update the port
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}
*/

/**************************************************************************************
** Client is asking us to flip/flop the mirror 
***************************************************************************************/
bool Watec::FlipFlopMirror(const int onoff)
{
    //int status;
    bool status;
    char commandline[100];
	if (onoff == true)
	{
	//commandline = std::string("echo -en MON > ") + std::string("/dev/ttyAMA0 ") ;
        sprintf(commandline, "MON");
    	LOGF_INFO(" Mirror flip/flop command: \"%s\".", commandline);
   	  //status = system(commandline.c_str());   
   	  status = sendCommand(commandline);   
   	  //if (status < 0)
        if (status == false)
    	  return false;
     	usleep( 100000 );
     	sleep(1);

    	 LOG_INFO(" Mirror ON.");
	}
	else if (onoff == false)
	{
	//commandline = std::string("echo -en MOF > ") + std::string("/dev/ttyAMA0 ") ;
        sprintf(commandline, "MOF");
    	LOGF_INFO(" Mirror flip/flop command: \"%s\".", commandline);
   	  //status = system(commandline.c_str());   
   	  status = sendCommand(commandline);   
   	  //if (status < 0)
    	if (status == false)
    	  return false;
     	usleep( 100000 );
     	sleep(1);

    	 LOG_INFO(" Mirror OFF.");
	}
	else
	{LOG_INFO(" Mirror did nothing.");}
      

    return true;
}
      
/**************************************************************************************
** Client is asking us to setup the exposure time
***************************************************************************************/
bool Watec::SetExposureTime(const int exposure)
{
    //int status;
    bool status;
    char commandline[100];
    char ExposureArg[2] = "1";
    char ExposureTxt[11] = "1/60";
    switch(exposure)
    {
       case '1':  ExposureArg[1] = '1';
                  strcpy(ExposureTxt, " 1/60  sec");
                  // DEC01 .. BIN0001
          
			break;       
       case '2':  ExposureArg[1] = '2';
                  strcpy(ExposureTxt, " 1/125 sec");
                  // DEC02 .. BIN0010
         
			break;       
       case '3':  ExposureArg[1] = '3';
                  strcpy(ExposureTxt, " 1/250 sec");
                  // DEC03 .. BIN0011
          
			break;       
       case '4':  ExposureArg[1] = '4';
                  strcpy(ExposureTxt, " 1/500 sec");
                  // DEC04 .. BIN0100
          
			break;      
       case '5':  ExposureArg[1] = '5';
                  strcpy(ExposureTxt, "1/1000 sec");
                  // DEC05 .. BIN0101
          
			break;       
       case '6':  ExposureArg[1] = '6';
                  strcpy(ExposureTxt, "1/2000 sec");
                  // DEC06 .. BIN0110
          
			break;       
       case '7':  ExposureArg[1] = '7';
                  // 256 frm = 256 / 2 / 25 sec = 5.12sec
                  strcpy(ExposureTxt, "  5.12 sec");
                  // DEC07 .. BIN0111
        
			break;      
       case '8':  ExposureArg[1] = '8';
                  // 128 frm = 128 / 2 / 25 sec = 2.56sec
                  strcpy(ExposureTxt, "  2.56 sec");
                  // DEC08 .. BIN1000
           
			break;      
       case '9':  ExposureArg[1] = '9';
                  // 64 frm = 64 / 2 / 25 sec = 1.28sec
                  strcpy(ExposureTxt, "  1.28 sec");
                  // DEC09 .. BIN1001
            
			break;     
       case 'A':  ExposureArg[1] = 'A';
                  // 32 frm = 32 / 2 / 25 sec = 0.64sec
                  strcpy(ExposureTxt, "  0.64 sec");
                  // DEC10 .. BIN1010
          
			break;       
       case 'B':  ExposureArg[1] = 'B';
                  // 16 frm = 16 / 2 / 25 sec = 0.32sec
                  strcpy(ExposureTxt, "  0.32 sec");
                  // DEC11 .. BIN1011
          
			break;      
       case 'C':  ExposureArg[1] = 'C';
                  // 8 frm = 8 / 2 / 25 sec = 0.16sec
                  strcpy(ExposureTxt, "  0.16 sec");
                  // DEC12 .. BIN1100
        
			break;         
       case 'D':  ExposureArg[1] = 'D';
                  // 4 frm = 4 / 2 / 25 sec = 0.08sec
                  strcpy(ExposureTxt, "  0.08 sec");
                  // DEC13 .. BIN1101
        
			break;          
       case 'E':  ExposureArg[1] = 'E';
                  // 2 frm = 2 / 2 / 25 sec = 1/25sec
                  strcpy(ExposureTxt, "  1/25 sec");
                  // DEC14 .. BIN1110
       
			break;          
       case 'F':  ExposureArg[1] = 'F';
                  // 2 frm = 2 / 2 / 25 sec = 1/25sec
                  strcpy(ExposureTxt, "  1/?? sec");
                  // DEC14 .. BIN1110

			break;
                 
       case '0':  ExposureArg[1] = '0';
                  // 2 frm = 2 / 2 / 25 sec = 1/25sec
                  strcpy(ExposureTxt, "  1/25 sec");
                  // DEC14 .. BIN1110
			break;
                
       default:   ExposureArg[1] = 'F';
                  // 1 frm = 1 / 2 / 25 sec = 1/50sec
                  strcpy(ExposureTxt, "  1/50 sec");
                  // DEC15 .. BIN1111         
    }

	//commandline = std::string("echo -en E") + std::string(ExposureArg) + std::string(" > ")+ std::string("/dev/ttyAMA0 ") ;
    //commandline = std::string("E") + std::string(ExposureArg) + std::string("#");
    sprintf(commandline, "E%c%c",ExposureArg[0],ExposureArg[1]);
                  //status = system(commandline.c_str());   
                  //status = sendCommand(commandline.c_str());   
                  status = sendCommand(commandline);
                  usleep( 10000 );
                  LOGF_INFO("Watec Exposure time set via remote to %s", ExposureTxt);
                  //LOGF_INFO("w exp command \"%s\"", commandline.c_str());
                  LOGF_INFO("w exp command \"%s\"", commandline);
    
    //if (status < 0)
    if (status == false)
    {
     LOG_INFO(" unable to set Exposure Time for Watec camera , something got horrybly wrong");
     return false;
    }
     
    LOGF_INFO(" Exposure Time for Watec camera set to %s (code %c).", ExposureTxt, ExposureArg[1]);


    return true;
}

/**************************************************************************************
** Client is asking us to setup the gain
***************************************************************************************/
bool Watec::SetGain(const int gain)
{
    //int status;
    bool status;
    char commandline[100];
    char GainArg[3] = "00";
    char GainTxt[6] = " 10% ";

    switch(gain)
    {
       case '1':  GainArg[0] = '1';
                  strcpy(GainTxt, " 10% ");
                  // 10% ... 102 = 0.1 x 1023 
                  
                  break;
       case '2':  GainArg[0] = '2';
                  strcpy(GainTxt, " 20% ");
                  // 20% ... 205 = 0.2 x 1023 
                  
                  break;
       case '3':  GainArg[0] = '3';
                  strcpy(GainTxt, " 30% ");
                  // 30% ... 307 = 0.3 x 1023 
                  
                  break;
       case '4':  GainArg[0] = '4';
                  strcpy(GainTxt, " 40% ");
                  // 40% ... 409 = 0.4 x 1023 
                 
                  break;
       case '5':  GainArg[0] = '5';
                  strcpy(GainTxt, " 50% ");
                  // 50% ... 512 = 0.5 x 1023 
                  
                  break;
       case '6':  GainArg[0] = '6';
                  strcpy(GainTxt, " 60% ");
                  // 60% ... 614 = 0.6 x 1023 
                  
                  break;
       case '7':  GainArg[0] = '7';
                  strcpy(GainTxt, " 70% ");
                  // 70% ... 716 = 0.7 x 1023 
                  
                  break;
       case '8':  GainArg[0] = '8';
                  strcpy(GainTxt, " 80% ");
                  // 80% ... 818 = 0.8 x 1023 
                 
                  break;
       case '9':  GainArg[0] = '9';
                  strcpy(GainTxt, " 90% ");
                  // 90% ... 921 = 0.9 x 1023 
                 
                  break;
       case 'A':  GainArg[0] = '9'; GainArg[1] ='9';
                  strcpy(GainTxt, "100% ");
                  // 100% ... 1023 = 1.0 x 1023 
                 
                  break;
       case '0':  
       default:   GainArg[0] = '0';
                  strcpy(GainTxt, "  0% ");
                  //  0% ... 0 = 0 x 1023 
    }

	//commandline = std::string("echo -en G") + std::string(GainArg) + std::string(" > ") + std::string("/dev/ttyAMA0 ") ;
    sprintf(commandline, "G%s",GainArg);
    //status = system(commandline.c_str());   
    status = sendCommand(commandline);   
    usleep( 10000 );
    LOGF_INFO("Watec Exposure time set via remote to %s", GainTxt);
    
    //if (status < 0)
    if (status == false)
    {
     LOG_INFO(" Unable to use  subsystem to set Gain for Watec camera.");
     return false;
    }
    
    GainN[0].value = atoi(GainArg);
    GainNP.s       = IPS_OK;
    IDSetNumber(&GainNP, nullptr); 

    LOGF_INFO(" Gain for Watec camera set to %s (code %c).", GainTxt, GainArg);
    LOGF_INFO("  command: \"%s\".", commandline);

    //LOGF_INFO(" Gain Value set to: \"%3.0f\".", GainN[0].value);

    return true;
}

bool Watec::AddGain(const int inc)
{
    bool status;
    char commandline[100];
    char GainArg[50];
    char GainTxt[50];
    int newValue = (int)(GainN[0].value + 0.5);//round double to int
    //LOGF_INFO("Actual gain is %d", newValue);
    int tempValue = newValue + inc;
    //LOGF_INFO("Computed gain is %d", tempValue);
    
    if(tempValue < 0)
        newValue = 0;
    else if(tempValue > 99)
        newValue = 99;
    else
        newValue = tempValue;
    sprintf(GainArg, "%d",newValue);
    sprintf(GainTxt, " %d%% ",newValue);
    sprintf(commandline, "G%s",GainArg);  
    status = sendCommand(commandline);   
    usleep( 10000 );
    LOGF_INFO("Watec Exposure time set via remote to %s", GainTxt);
    
    //if (status < 0)
    if (status == false)
    {
     LOG_INFO(" Unable to use  subsystem to set Gain for Watec camera.");
     return false;
    }
    
    GainN[0].value = newValue;
    GainNP.s       = IPS_OK;
    IDSetNumber(&GainNP, nullptr);

    LOGF_INFO(" Gain for Watec camera set to %s (code %c).", GainTxt, GainArg);
    LOGF_INFO("  command: \"%s\".", commandline);
    
    //LOGF_INFO(" Gain Value set to: \"%3.0f\".", GainN[0].value);
    return true;
}

/**************************************************************************************
** Send command via connected interface
***************************************************************************************/
bool Watec::sendCommand(const char *cmd)
{
    int nbytes_read=0, nbytes_written=0, tty_rc = 0;
    LOGF_DEBUG("CMD <%s>", cmd);
    char resp[256]="\0";
    sprintf(resp,"%s#",cmd); // add endincg char #

    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);
        if ( (tty_rc = tty_write_string(PortFD, resp, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }
    }

    if (isSimulation())
    {
        nbytes_read = strlen(resp);
        //resp[nbytes_read] = '\0';
    }
    else
    {
        if ( (tty_rc = tty_read_section(PortFD, resp, '#', WATEC_TIMEOUT, &nbytes_read)) != TTY_OK)
        //if ( (tty_rc = tty_read(PortFD, resp, strlen(cmd), WATEC_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial read error: %s", errorMessage);
            return false;
        }
    }

    resp[nbytes_read - 1] = '\0';
    LOGF_DEBUG("RES <%s>", resp);

    return true;
}