/*******************************************************************************
 Copyright(c) 2021 J.Adamcak for astro.sk. All rights reserved.
 
 Shelyak Dome Tracker Driver. 
 
 Based on Astrometric Solutions DomePro2 INDI Driver by Jasem Mutlaq

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "shelyak_dome_tracker.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <cstring>
#include <inttypes.h>
#include <memory>
#include <regex>
#include <termios.h>

static std::unique_ptr<ShelyakDT> shelyakdt(new ShelyakDT());

void ISGetProperties(const char *dev)
{
    shelyakdt->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    shelyakdt->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    shelyakdt->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    shelyakdt->ISNewNumber(dev, name, values, names, n);
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

void ISSnoopDevice(XMLEle *root)
{
    shelyakdt->ISSnoopDevice(root);
}

ShelyakDT::ShelyakDT()
{
    m_ShutterState     = SHUTTER_UNKNOWN;

    SetDomeCapability(DOME_CAN_ABORT |
                      DOME_CAN_ABS_MOVE |
                      DOME_CAN_REL_MOVE |
    //                  DOME_CAN_PARK |
    //                  DOME_CAN_SYNC |
                      DOME_HAS_SHUTTER);
}

bool ShelyakDT::initProperties()
{
    INDI::Dome::initProperties();
    // Firmware & Hardware versions
    IUFillText(&VersionT[VERSION_FIRMWARE], "VERSION_FIRMWARE", "Firmware", "NA");
    IUFillText(&VersionT[VERSION_HARDWARE], "VERSION_HARDWARE", "Hardware", "NA");
    IUFillTextVector(&VersionTP, VersionT, 2, getDeviceName(), "VERSION", "Version", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Dome & Shutter statuses
    IUFillText(&StatusT[STATUS_DOME], "STATUS_DOME", "Dome", "NA");
    IUFillText(&StatusT[STATUS_SHUTTER], "STATUS_SHUTTER", "Shutter", "NA");
    IUFillText(&StatusT[ENCODER_VALUE], "ENCODER_VALUE", "Encoder", "NA");
    IUFillText(&StatusT[INDICATOR_SWITCH], "INDICATOR_SWITCH", "Indicator", "NA");
    IUFillText(&StatusT[EMERGENCY_CLOSE_SW], "EMERGENCY_CLOSE_SW", "Emergency Close", "NA");
    IUFillText(&StatusT[TELESCOPE_SECURED], "TELESCOPE_SECURED", "Telescope Secured", "NA");
    IUFillText(&StatusT[RESET_SWITCH], "RESET_SWITCH", "Reset", "NA");
    IUFillTextVector(&StatusTP, StatusT, 7, getDeviceName(), "STATUS", "Status", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&RegimeS[CLASSIC_DOME], "CLASSIC_DOME", "Classic dome", ISS_OFF);
    IUFillSwitch(&RegimeS[ROLL_OFF_ROOF], "ROLL_OFF_ROOF", "Roll-off roof", ISS_OFF);
    IUFillSwitchVector(&RegimeSP, RegimeS, 2, getDeviceName(), "ROOF_TYPE", "Roof type", OPTIONS_TAB, IP_RW,
                       ISR_ATMOST1, 60, IPS_OK);
    
    IUFillSwitch(&DomeShutterS3[SHUT_OPEN], "SHUTTER_OPEN", "Open", ISS_OFF);
    IUFillSwitch(&DomeShutterS3[SHUT_CLOSE], "SHUTTER_CLOSE", "Close", ISS_OFF);
    IUFillSwitch(&DomeShutterS3[SHUT_IDLE], "SHUTTER_IDLE", "Idle", ISS_ON);
    IUFillSwitchVector(&DomeShutterSP3, DomeShutterS3, 3, getDeviceName(), "DOME_SHUTTER", "Shutter", MAIN_CONTROL_TAB, IP_RW,
                       ISR_ATMOST1, 60, IPS_OK);
    
    IUFillNumber(&DomeShutterN[DOME_SHUTTER_MOVE_TIME], "DOME_SHUTTER_MOVE_TIME", "Shutter move time (ms)", "%.f", 0, 100000, 0, 5000);
    IUFillNumberVector(&DomeShutterNP, DomeShutterN, 1, getDeviceName(), "DOME_SHUTTER_TIME", "Shutter Time", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&DomeMotionS3[DOM_CW], "DOME_CW", "Dome CW", ISS_OFF);
    IUFillSwitch(&DomeMotionS3[DOM_CCW], "DOME_CCW", "Dome CCW", ISS_OFF);
    IUFillSwitch(&DomeMotionS3[DOM_IDLE], "DOME_IDLE", "Dome IDLE", ISS_ON);
    IUFillSwitchVector(&DomeMotionSP3, DomeMotionS3, 3, getDeviceName(), "DOME_MOTION", "Motion", MAIN_CONTROL_TAB, IP_RW,
                       ISR_ATMOST1, 60, IPS_OK);

    IUFillSwitch(&DomeResetS[DIRECTION_POSITIVE], "DIRECTION_POSITIVE", "Reset CW", ISS_OFF);
    IUFillSwitch(&DomeResetS[DIRECTION_NEGATIVE], "DIRECTION_NEGATIVE", "Reset CCW", ISS_OFF);
    IUFillSwitchVector(&DomeResetSP, DomeResetS, 2, getDeviceName(), "RESET_DIRECTION", "Reset dome", MAIN_CONTROL_TAB, IP_RW,
                       ISR_ATMOST1, 60, IPS_OK);
    
    IUFillSwitch(&CalibrateS[CALIBRATE], "CALIBRATE", "Calibrate", ISS_OFF);
    IUFillSwitchVector(&CalibrateSP, CalibrateS, 1, getDeviceName(), "DOME_CALIBRATE_MOTION", "Calibrate Motion", MAIN_CONTROL_TAB,
                       IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // Settings
    //IUFillNumber(&SettingsN[SETTINGS_AZ_CPR], "SETTINGS_AZ_CPR", "Az CPR (steps)", "%.f", 0x20, 0x40000000, 0, 0);
    //IUFillNumber(&SettingsN[SETTINGS_AZ_COAST], "SETTINGS_AZ_COAST", "Az Coast (deg)", "%.2f", 0, 15, 0, 0);
    //IUFillNumber(&SettingsN[SETTINGS_AZ_HOME], "SETTINGS_AZ_HOME", "Az Home (deg)", "%.2f", 0, 360, 0, 0);
    //IUFillNumber(&SettingsN[SETTINGS_AZ_PARK], "SETTINGS_AZ_PARK", "Az Park (deg)", "%.2f", 0, 360, 0, 0);
    //IUFillNumber(&SettingsN[SETTINGS_AZ_STALL_COUNT], "SETTINGS_AZ_STALL_COUNT", "Az Stall Count (steps)", "%.f", 0, 0x40000000, 0, 0);
    //IUFillNumberVector(&SettingsNP, SettingsN, 5, getDeviceName(), "SETTINGS", "Settings", SETTINGS_TAB, IP_RW, 60, IPS_IDLE);

    // Home
    //IUFillSwitch(&HomeS[HOME_DISCOVER], "HOME_DISCOVER", "Discover", ISS_OFF);
    //IUFillSwitch(&HomeS[HOME_GOTO], "HOME_GOTO", "Goto", ISS_OFF);
    //IUFillSwitchVector(&HomeSP, HomeS, 2, getDeviceName(), "HOME", "Home", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1, 60, IPS_OK);

    serialConnection->setDefaultBaudRate(Connection::Serial::B_115200);

    //SetParkDataType(PARK_AZ);
    //defineSwitch(&RegimeSP);
    addAuxControls();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setupInitialParameters()
{
    if (InitPark())
    {
        // If loading parking data is successful, we just set the default parking values.
        SetAxis1ParkDefault(0);
    }
    else
    {
        // Otherwise, we set all parking data to default in case no parking data is found.
        SetAxis1Park(0);
        SetAxis1ParkDefault(0);
    }

    if (getFwConf())    //only one call 1d 02 ca e9 during connect
    //if (getFirmwareVersion() && getHardwareConfig())    
        VersionTP.s = IPS_OK;

    if (getStatus())    //only one cyclic call 1d 02 cb ea every 100ms
    //if (getDomeStatus() && getShutterStatus())          
    {
        StatusTP.s = IPS_OK;
        SettingsNP.s = IPS_OK;
        IDSetNumber(&DomeAbsPosNP, nullptr);
    }

    //if (getDomeAzCPR() && getDomeAzCoast() && getDomeHomeAz() && getDomeParkAz() && getDomeAzStallCount())
    //    SettingsNP.s = IPS_OK;

    //if (getDomeAzPos())
    //    IDSetNumber(&DomeAbsPosNP, nullptr);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::Handshake()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfuly to simulated %s.", getDeviceName());
        char versionString[DRIVER_LEN] = {0};
        snprintf(versionString, DRIVER_LEN, "%d", 0);
        IUSaveText(&VersionT[VERSION_FIRMWARE], versionString);
        return true;
    }

    return getFwConf(); //getFirmwareVersion();
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
const char *ShelyakDT::getDefaultName()
{
    return "ShelyakDT";
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::updateProperties()
{
    INDI::Dome::updateProperties();
    defineSwitch(&RegimeSP);

    if (isConnected())
    {
        setupInitialParameters();
        deleteProperty(DomeShutterSP.name);
        deleteProperty(DomeMotionSP.name);
        defineNumber(&DomeShutterNP);
        defineSwitch(&DomeShutterSP3);
        if(roofType==0xAA)
        {
            defineSwitch(&DomeMotionSP3);
            defineSwitch(&DomeResetSP);
        }
        defineText(&VersionTP);
        defineText(&StatusTP);
        if(roofType==0xAA)
            defineSwitch(&CalibrateSP);
        else
        {
            deleteProperty(DomeMotionSP3.name);
            deleteProperty(DomeResetSP.name);
            deleteProperty(CalibrateSP.name);
        }
        //defineNumber(&SettingsNP);
        //defineSwitch(&HomeSP);
    }
    else
    {
        deleteProperty(DomeMotionSP3.name);
        deleteProperty(DomeShutterSP3.name);
        deleteProperty(VersionTP.name);
        deleteProperty(StatusTP.name);
        deleteProperty(DomeShutterNP.name);
        deleteProperty(CalibrateSP.name);
        //deleteProperty(RegimeSP.name);
        deleteProperty(DomeResetSP.name);
        //deleteProperty(SettingsNP.name);
        //
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
 /*       // Home
        if (!strcmp(name, HomeSP.name))
        {
            IUResetSwitch(&HomeSP);
            for (int i = 0; i < HomeSP.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(HomeS[HOME_GOTO].name, names[i]))
                {
                    if (!gotoHomeDomeAz())
                    {
                        HomeSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to go to Home Dome Az.");
                    }
                    else
                    {
                        HomeS[HOME_GOTO].s = ISS_ON;
                        HomeSP.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(HomeS[HOME_DISCOVER].name, names[i]))
                {
                    if (!discoverHomeDomeAz())
                    {
                        HomeSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to discover Home Dome Az.");
                    }
                    else
                    {
                        HomeS[HOME_DISCOVER].s = ISS_ON;
                        HomeSP.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&HomeSP, nullptr);
            return true;
        }
 */       
        // Shutter
        if (!strcmp(name, DomeShutterSP3.name))
        {
            IUResetSwitch(&DomeShutterSP3);
            for (int i = 0; i < DomeShutterSP3.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(DomeShutterS3[SHUT_OPEN].name, names[i]))
                {
                    if (!ControlShutter(SHUTTER_OPEN))//shutterOpen()+timer callback
                    {
                        DomeShutterSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to Open Shutter.");
                    }
                    else
                    {
                        DomeShutterS3[SHUT_OPEN].s = ISS_ON;
                        DomeShutterSP3.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(DomeShutterS3[SHUT_CLOSE].name, names[i]))
                {
                    if (!ControlShutter(SHUTTER_CLOSE))//shutterClose()+timer callback
                    {
                        DomeShutterSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to Close Shutter.");
                    }
                    else
                    {
                        DomeShutterS3[SHUT_CLOSE].s = ISS_ON;
                        DomeShutterSP3.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(DomeShutterS3[SHUT_IDLE].name, names[i]))
                {
                    if (!moveShutter(SHUT_IDLE))//shutterIdle()
                    {
                        DomeShutterSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to Stop Shutter.");
                    }
                    else
                    {
                        DomeShutterS3[SHUT_OPEN].s = ISS_OFF;
                        DomeShutterS3[SHUT_CLOSE].s = ISS_OFF;
                        DomeShutterS3[SHUT_IDLE].s = ISS_ON;
                        DomeShutterSP3.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&DomeShutterSP3, nullptr);
            return true;
        }
        
        // Dome
        if (!strcmp(name, DomeMotionSP3.name))
        {
            IUResetSwitch(&DomeMotionSP3);
            for (int i = 0; i < DomeMotionSP3.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(DomeMotionS3[DOM_CW].name, names[i]))
                {
                    if (!moveDome(DOM_CW))//domeCW()
                    {
                        DomeMotionSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to rotate Dome CW.");
                    }
                    else
                    {
                        DomeMotionS3[DOM_CW].s = ISS_ON;
                        DomeMotionSP3.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(DomeMotionS3[DOM_CCW].name, names[i]))
                {
                    if (!moveDome(DOM_CCW))//domeCCW
                    {
                        DomeMotionSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to rotate Dome CCW.");
                    }
                    else
                    {
                        DomeMotionS3[DOM_CCW].s = ISS_ON;
                        DomeMotionSP3.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(DomeMotionS3[DOM_IDLE].name, names[i]))
                {
                    if (!moveDome(DOM_IDLE))//domeIdle()
                    {
                        DomeMotionSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to Stop Dome.");
                    }
                    else
                    {
                        DomeMotionS3[DOM_CW].s = ISS_OFF;
                        DomeMotionS3[DOM_CCW].s = ISS_OFF;
                        DomeMotionS3[DOM_IDLE].s = ISS_ON;
                        DomeMotionSP3.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&DomeMotionSP3, nullptr);
            return true;
        }
        
        // Reset Dome
        if (!strcmp(name, DomeResetSP.name))
        {
            IUResetSwitch(&DomeResetSP);
            for (int i = 0; i < DomeResetSP.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(DomeResetS[DIRECTION_POSITIVE].name, names[i]))
                {
                    if (!resetDome(DIRECTION_POSITIVE))//resetCW()
                    {
                        DomeMotionSP3.s = IPS_ALERT;
                        LOG_ERROR("Failed to rotate Dome CW to Reset.");
                    }
                    else
                    {
                        DomeResetS[DIRECTION_POSITIVE].s = ISS_ON;
                        DomeResetSP.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(DomeResetS[DIRECTION_NEGATIVE].name, names[i]))
                {
                    if (!resetDome(DIRECTION_NEGATIVE))//resetCCW
                    {
                        DomeResetSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to rotate Dome CCW to Reset.");
                    }
                    else
                    {
                        DomeResetS[DIRECTION_NEGATIVE].s = ISS_ON;
                        DomeResetSP.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&DomeResetSP, nullptr);
            return true;
        }
        
        // Roof Type
        if (!strcmp(name, RegimeSP.name))
        {
            IUResetSwitch(&RegimeSP);
            for (int i = 0; i < RegimeSP.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(RegimeS[CLASSIC_DOME].name, names[i]))
                {
                    if (!setMode(CLASSIC_DOME))//dome Mode
                    {
                        RegimeSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to switch regime to Dome mode.");
                    }
                    else
                    {
                        RegimeS[CLASSIC_DOME].s = ISS_ON;
                        RegimeSP.s = IPS_BUSY;
                    }
                }
                else if (!strcmp(RegimeS[ROLL_OFF_ROOF].name, names[i]))
                {
                    if (!setMode(ROLL_OFF_ROOF))//roof Mode
                    {
                        RegimeSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to switch regime to Roof mode.");
                    }
                    else
                    {
                        RegimeS[ROLL_OFF_ROOF].s = ISS_ON;
                        RegimeSP.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&RegimeSP, nullptr);
            return true;
        }
        
        // Calibrate Dome
        if (!strcmp(name, CalibrateSP.name))
        {
            IUResetSwitch(&CalibrateSP);
            for (int i = 0; i < CalibrateSP.nsp; i++)
            {
                if (states[i] != ISS_ON)
                    continue;

                if (!strcmp(CalibrateS[CALIBRATE].name, names[i]))
                {
                    if (!calibrateDome())//calibrateDome()
                    {
                        CalibrateSP.s = IPS_ALERT;
                        LOG_ERROR("Failed to Calibrate dome.");
                    }
                    else
                    {
                        CalibrateS[CALIBRATE].s = ISS_ON;
                        CalibrateSP.s = IPS_BUSY;
                    }
                }
            }

            IDSetSwitch(&CalibrateSP, nullptr);
            return true;
        }
    }

    return INDI::Dome::ISNewSwitch(dev, name, states, names, n);
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////

bool ShelyakDT::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //Shutter Time
        if (!strcmp(name, DomeShutterNP.name))
        {
            bool allSet = true;
            for (int i = 0; i < DomeShutterNP.nnp; i++)
            {
                if (!strcmp(DomeShutterN[DOME_SHUTTER_MOVE_TIME].name, names[i]))
                {
                    if (shutterTime=(static_cast<uint32_t>(values[i])))//set shutter time
                        DomeShutterN[DOME_SHUTTER_MOVE_TIME].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Shutter time.");
                    }
                }
            }
            DomeShutterNP.s = allSet ? IPS_OK : IPS_ALERT;
            IDSetNumber(&DomeShutterNP, nullptr);
            return true;
        }
/*
        ////////////////////////////////////////////////////////////
        // Settings
        ////////////////////////////////////////////////////////////
        if (!strcmp(name, SettingsNP.name))
        {
            bool allSet = true;
            for (int i = 0; i < SettingsNP.nnp; i++)
            {
                if (!strcmp(SettingsN[SETTINGS_AZ_CPR].name, names[i]))
                {
                    if (setDomeAzCPR(static_cast<uint32_t>(values[i])))
                        SettingsN[SETTINGS_AZ_CPR].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Dome AZ CPR.");
                    }
                }
                else if (!strcmp(SettingsN[SETTINGS_AZ_COAST].name, names[i]))
                {
                    if (setDomeAzCoast(static_cast<uint32_t>(values[i])))
                        SettingsN[SETTINGS_AZ_COAST].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Dome AZ Coast.");
                    }
                }
                else if (!strcmp(SettingsN[SETTINGS_AZ_HOME].name, names[i]))
                {
                    if (setDomeHomeAz(static_cast<uint32_t>(values[i])))
                        SettingsN[SETTINGS_AZ_HOME].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Dome AZ Home.");
                    }
                }
                else if (!strcmp(SettingsN[SETTINGS_AZ_PARK].name, names[i]))
                {
                    if (setDomeParkAz(static_cast<uint32_t>(values[i])))
                        SettingsN[SETTINGS_AZ_PARK].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Dome AZ Park.");
                    }
                }
                else if (!strcmp(SettingsN[SETTINGS_AZ_STALL_COUNT].name, names[i]))
                {
                    if (setDomeAzStallCount(static_cast<uint32_t>(values[i])))
                        SettingsN[SETTINGS_AZ_STALL_COUNT].value = values[i];
                    else
                    {
                        allSet = false;
                        LOG_ERROR("Failed to set Dome AZ Stall Count.");
                    }
                }
            }

            SettingsNP.s = allSet ? IPS_OK : IPS_ALERT;
            IDSetNumber(&SettingsNP, nullptr);
            return true;
        }
*/
    }

    return INDI::Dome::ISNewNumber(dev, name, values, names, n);
}
////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
void ShelyakDT::TimerHit()
{
    if (!isConnected())
        return;

    double currentAz = DomeAbsPosN[0].value;
    std::string domeStatus = StatusT[STATUS_DOME].text;
    std::string shutterStatus = StatusT[STATUS_SHUTTER].text;
    if(getStatus() && (strcmp(domeStatus.c_str(), StatusT[STATUS_DOME].text) != 0 ||
            strcmp(shutterStatus.c_str(), StatusT[STATUS_SHUTTER].text) != 0))
    {
        if (getDomeState() == DOME_MOVING || getDomeState() == DOME_PARKING)
        {

            if (!strcmp(StatusT[STATUS_DOME].text, "Idle"))
            {
                if (getDomeState() == DOME_PARKING)
                    SetParked(true);

                setDomeState(DOME_IDLE);

                if (HomeSP.s == IPS_BUSY)
                {
                    IUResetSwitch(&HomeSP);
                    HomeSP.s = IPS_IDLE;
                    IDSetSwitch(&HomeSP, nullptr);
                }
            }
        }

        if (getShutterState() == SHUTTER_MOVING)
        {
            int statusVal = processShutterStatus();
            if (statusVal == 0x01)
                setShutterState(SHUTTER_UNKNOWN);
            else if (statusVal == 0x10)
                setShutterState(SHUTTER_MOVING);
            else if (statusVal == 0x11)
                setShutterState(SHUTTER_OPENED);
            else if (statusVal == 0x20)
                setShutterState(SHUTTER_MOVING);
            else if (statusVal == 0x21)
                setShutterState(SHUTTER_CLOSED);
        }

        IDSetText(&StatusTP, nullptr);
    }
    //if(getDomeAzPos() && std::abs(currentAz - DomeAbsPosN[0].value) > DOME_AZ_THRESHOLD)
    //    IDSetNumber(&DomeAbsPosNP, nullptr);

    SetTimer(POLLMS);
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::MoveAbs(double az)
{
    //INDI_UNUSED(az);
    uint32_t steps = static_cast<uint32_t>(az * (SettingsN[SETTINGS_AZ_CPR].value / 360));
    if (gotoDome(steps))
        return IPS_BUSY;
    return IPS_ALERT;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::MoveRel(double azDiff)
{
    targetAz = DomeAbsPosN[0].value + azDiff;

    if (targetAz < DomeAbsPosN[0].min)
        targetAz += DomeAbsPosN[0].max;
    if (targetAz > DomeAbsPosN[0].max)
        targetAz -= DomeAbsPosN[0].max;

    // It will take a few cycles to reach final position
    return MoveAbs(targetAz);
}

/////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::Sync(double az)
{
    INDI_UNUSED(az);
    if(calibrateDome())
    //if(calibrateDomeAz(az))
        return true;
    return false;
}

/////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::Move(DomeDirection dir, DomeMotionCommand operation)
{
    //    INDI_UNUSED(dir);
    //    INDI_UNUSED(operation);
    if (operation == MOTION_STOP)
    {
        if(moveDome(DOM_IDLE))
            return IPS_OK;
    }
    else if (dir == DOME_CCW)
    {
        if(moveDome(DOM_CCW))
            return IPS_BUSY;
    }
    else if (dir == DOME_CW)
    {
        if(moveDome(DOM_CW))
            return IPS_BUSY;
    }

    return IPS_ALERT;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::Park()
{
    targetAz = GetAxis1Park();
    //return (gotoDomePark() ? IPS_BUSY : IPS_ALERT);
    return MoveAbs(targetAz);
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::UnPark()
{
    return IPS_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
IPState ShelyakDT::ControlShutter(ShutterOperation operation)
{
    if (moveTID)
    {
        IERmTimer(moveTID);
        moveTID = 0;
    }
    if (operation == SHUTTER_OPEN)
    {
        if (moveShutter(SHUT_OPEN))
        {
            moveTID = IEAddTimer(static_cast<int>(shutterTime), moveHelper, this);
            return IPS_BUSY;
        }            
    }
    else if (operation == SHUTTER_CLOSE)
    {
        if (moveShutter(SHUT_CLOSE))
        {
            moveTID = IEAddTimer(static_cast<int>(shutterTime), moveHelper, this);
            return IPS_BUSY;
        }    
    }
    return IPS_ALERT;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
void ShelyakDT::moveHelper(void *p)
{
    static_cast<ShelyakDT *>(p)->moveTimeout();
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
void ShelyakDT::moveTimeout()
{
    if(moveShutter(SHUT_IDLE))
    {
        DomeShutterS3[SHUT_OPEN].s = ISS_OFF;
        DomeShutterS3[SHUT_CLOSE].s = ISS_OFF;
        DomeShutterS3[SHUT_IDLE].s = ISS_ON;
        DomeShutterSP3.s = IPS_IDLE;
        IDSetSwitch(&DomeShutterSP3, nullptr);
        LOGF_DEBUG("Sutter movement stopped after %d ms.", shutterTime);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
void ShelyakDT::moveEndstop()
{
    if(moveShutter(SHUT_IDLE))
    {
        DomeShutterS3[SHUT_OPEN].s = ISS_OFF;
        DomeShutterS3[SHUT_CLOSE].s = ISS_OFF;
        DomeShutterS3[SHUT_IDLE].s = ISS_ON;
        DomeShutterSP3.s = IPS_IDLE;
        IDSetSwitch(&DomeShutterSP3, nullptr);
        LOG_DEBUG("Sutter movement stopped in Endstop.");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::Abort()
{
    if(!abortDome())
        return false;
    //return true;
    /*
    if (!killDomeAzMovement())
        return false;

    if (getShutterState() == SHUTTER_MOVING && killDomeShutterMovement())
    {
        setShutterState(SHUTTER_UNKNOWN);
    }
    */
    if (ParkSP.s == IPS_BUSY)
    {
        SetParked(false);
    }

    if (HomeSP.s == IPS_BUSY)
    {
        IUResetSwitch(&HomeSP);
        HomeSP.s = IPS_IDLE;
        IDSetSwitch(&HomeSP, nullptr);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::saveConfigItems(FILE *fp)
{
    INDI::Dome::saveConfigItems(fp);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::SetCurrentPark()
{
    SetAxis1Park(DomeAbsPosN[0].value);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::SetDefaultPark()
{
    // By default set position to 90
    SetAxis1Park(90);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ShelyakDT::processShutterStatus()
{
    if (!strcmp(StatusT[STATUS_SHUTTER].text, "Idle"))
        return 0x01;
    else if (!strcmp(StatusT[STATUS_SHUTTER].text, "Opening"))
        return 0x10;
    else if (!strcmp(StatusT[STATUS_SHUTTER].text, "Opened"))
        return 0x11;
    else if (!strcmp(StatusT[STATUS_SHUTTER].text, "Closing"))
        return 0x20;
    else if (!strcmp(StatusT[STATUS_SHUTTER].text, "Closed"))
        return 0x21;
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t ShelyakDT::processDomeStatus()
{
    if (!strcmp(StatusT[STATUS_DOME].text, "Idle"))
        return 0x00;
    else if (!strcmp(StatusT[STATUS_DOME].text, "Reseting +"))
        return 0x01;
    else if (!strcmp(StatusT[STATUS_DOME].text, "Reseting -"))
        return 0x02;
    else if (!strcmp(StatusT[STATUS_DOME].text, "Moving +"))
        return 0x04;
    else if (!strcmp(StatusT[STATUS_DOME].text, "Moving -"))
        return 0x08;
    else if (!strcmp(StatusT[STATUS_DOME].text, "Counting steps"))
        return 0x09;
    
    return 0;
}

//helpers
bool ShelyakDT::crc(char *res)
{
    int i = 0;
    uint8_t sum = 0;
    while(sum!=uint8_t(res[i]))
    {
        sum += uint8_t(res[i]);
        ++i;
        if (i>20)
            return false;
    }
    return true;
}

uint8_t ShelyakDT::getCRC(char *cmd, uint8_t lenght)
{
    uint8_t sum = 0;
    for(uint8_t i = 0; i<lenght; ++i)
        sum += uint8_t(cmd[i]);
    
    return sum;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getFwConf()
{
    char res[DRIVER_LEN] = {0};
    //char res[6] = {0};
    char cmd[4] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x02);
    cmd[2] = uint8_t(0xca);
    cmd[3] = getCRC(cmd, 3);
    if (sendCommand(cmd, res, 4, 6) == false)
        return false;

    if (crc(res))
    {
        char versionString[DRIVER_LEN] = {0};
        snprintf(versionString, DRIVER_LEN, "%d.%d", res[2],res[3]);
        IUSaveText(&VersionT[VERSION_FIRMWARE], versionString);
        char configString[DRIVER_LEN] = {0};
        roofType = res[4];
        snprintf(configString, DRIVER_LEN, "%s", DomeHardware.at(roofType).c_str());
        IUSaveText(&VersionT[VERSION_HARDWARE], configString);
        return true;
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[5]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getStatus()
{
    char res[DRIVER_LEN] = {0};
    //char res[16] = {0};
    char cmd[4] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x02);
    cmd[2] = uint8_t(0xcb);
    cmd[3] = getCRC(cmd, 3);
    if (sendCommand(cmd, res, 4, 16) == false)
        return false;

    if (crc(res))
    {
        int32_t encoder = uint8_t(res[5]);  
        encoder += uint8_t(res[4]) << 8;      
        encoder += uint8_t(res[3]) << 16;
        encoder += uint8_t(res[2]) << 24;        
        char tmp[DRIVER_LEN] = {0};
        snprintf(tmp, DRIVER_LEN, "%d", encoder);
        SettingsN[SETTINGS_AZ_CPR].value = double(encoder);
        IUSaveText(&StatusT[STATUS_DOME], DomeStatus.at(res[6]).c_str());
        IUSaveText(&StatusT[STATUS_SHUTTER], ShutterStatus.at(res[7]).c_str());
        if((res[7] == 0x11)||(res[7] == 0x21))
            moveEndstop();
        IUSaveText(&StatusT[ENCODER_VALUE], tmp);
        bool indicatorSwitchClosed = bool(res[8]);
        bool indicatorSwitchOpened = bool(res[9]);
        if(!(indicatorSwitchClosed^indicatorSwitchOpened))
            IUSaveText(&StatusT[INDICATOR_SWITCH], "Undefined");
        else if(indicatorSwitchClosed&&(!indicatorSwitchOpened))
            IUSaveText(&StatusT[INDICATOR_SWITCH], "Closed");
        else    //(indicatorSwitchOpened&&(!indicatorSwitchClosed))
            IUSaveText(&StatusT[INDICATOR_SWITCH], "Opened");
        bool emergencySwitch = bool(res[10]);
        if(emergencySwitch)
            IUSaveText(&StatusT[EMERGENCY_CLOSE_SW], "Closed");
        else
            IUSaveText(&StatusT[EMERGENCY_CLOSE_SW], "Opened");
        bool telescopeSecured = bool(res[11]);
        if(telescopeSecured)
            IUSaveText(&StatusT[TELESCOPE_SECURED], "Closed");
        else
            IUSaveText(&StatusT[TELESCOPE_SECURED], "Opened");
        bool resetSwitch = bool(res[12]);
        if(resetSwitch)
            IUSaveText(&StatusT[RESET_SWITCH], "Closed");
        else
            IUSaveText(&StatusT[RESET_SWITCH], "Opened");
        uint8_t value0 = res[13];
        uint8_t value1 = res[14];
        return true;
    }
    LOGF_WARN("Bad answer, CRC failed %X", res[15]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setMode(Regime r)
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[5] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x03);
    cmd[2] = uint8_t(0xef);
    if(r==CLASSIC_DOME)
        cmd[3] = uint8_t(0xAA);
    if(r==ROLL_OFF_ROOF)
        cmd[3] = uint8_t(0xCC);
    cmd[4] = getCRC(cmd, 4);
    if (sendCommand(cmd, res, 5, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return getFwConf();
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::resetDome(ResetDir r)
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[5] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x03);
    cmd[2] = uint8_t(0xf2);
    if(r==DIRECTION_POSITIVE)
        cmd[3] = uint8_t(0x1A);
    if(r==DIRECTION_NEGATIVE)
        cmd[3] = uint8_t(0x1B);
    cmd[4] = getCRC(cmd, 4);
    if (sendCommand(cmd, res, 5, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return true;
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::moveShutter(ShutStatus d)
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[6] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x04);
    cmd[2] = uint8_t(0xf1);
    if(d==SHUT_OPEN)
        cmd[3] = uint8_t(0x01);
    if(d==SHUT_CLOSE)
        cmd[4] = uint8_t(0x01);
    if(d==SHUT_IDLE)
    {
        cmd[3] = uint8_t(0x00);
        cmd[4] = uint8_t(0x00);
    }
    cmd[5] = getCRC(cmd, 5);
    if (sendCommand(cmd, res, 6, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return true;
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::moveDome(DomMotion d)
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[6] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x04);
    cmd[2] = uint8_t(0xf0);
    if(d==DOM_CW)
        cmd[3] = uint8_t(0x01);
    if(d==DOM_CCW)
        cmd[4] = uint8_t(0x01);
    if(d==DOM_IDLE)
    {
        cmd[3] = uint8_t(0x00);
        cmd[4] = uint8_t(0x00);
    }
    cmd[5] = getCRC(cmd, 5);
    if (sendCommand(cmd, res, 6, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return true;
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::gotoDome(int32_t value)
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[9] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x07);
    cmd[2] = uint8_t(0xf3);
    if(value>=0)
        cmd[3] = uint8_t(0x1A);
    else
        cmd[3] = uint8_t(0x1B);
    uint32_t tmp = abs(value);
    cmd[4] = (tmp&0x7F000000)>>24;
    cmd[5] = (tmp&0xFF0000)>>16;
    cmd[6] = (tmp&0xFF00)>>8;
    cmd[7] = (tmp&0xFF);
    cmd[8] = getCRC(cmd, 8);
    if (sendCommand(cmd, res, 9, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return true;
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::calibrateDome()
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[4] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x02);
    cmd[2] = uint8_t(0xf6);
    cmd[3] = getCRC(cmd, 3);
    if (sendCommand(cmd, res, 4, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return getFwConf();
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::abortDome()
{
    char res[DRIVER_LEN] = {0};
    //char res[4] = {0};
    char cmd[4] = {0};
    cmd[0] = uint8_t(0x1d);
    cmd[1] = uint8_t(0x02);
    cmd[2] = uint8_t(0xf5);
    cmd[3] = getCRC(cmd, 3);
    if (sendCommand(cmd, res, 4, 4) == false)
        return false;

    if (crc(res))
    {
        if((res[1]==0x02)&&(res[2]==cmd[2]))
            return getFwConf();
        else
        {
            LOGF_WARN("Bad answer, mismatch command %X", res[2]);
            return false;
        }
    }

    LOGF_WARN("Bad answer, CRC failed %X", res[3]);
    return false;
}

/*
////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeAzCPR()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGcp", res) == false)
        return false;

    uint32_t cpr = 0;
    if (sscanf(res, "%X", &cpr) == 1)
    {
        SettingsN[SETTINGS_AZ_CPR].value = cpr;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeAzCPR(uint32_t cpr)
{
    char cmd[DRIVER_LEN] = {0};
    if (cpr < 0x20 || cpr > 0x40000000)
    {
        LOG_ERROR("CPR value out of bounds (32 to 1,073,741,824)");
        return false;
    }
    else if (cpr % 2 != 0)
    {
        LOG_ERROR("CPR value must be an even number");
        return false;
    }
    snprintf(cmd, DRIVER_LEN, "DScp0x%08X", cpr);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeAzCoast()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGco", res) == false)
        return false;

    uint32_t coast = 0;
    if (sscanf(res, "%X", &coast) == 1)
    {
        SettingsN[SETTINGS_AZ_COAST].value = coast * (360 / SettingsN[SETTINGS_AZ_CPR].value);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeAzCoast(uint32_t az)
{
    char cmd[DRIVER_LEN] = {0};
    uint32_t steps = static_cast<uint32_t>(az * (SettingsN[SETTINGS_AZ_CPR].value / 360));
    //    if (coast > 0x4000)
    //    {
    //        LOG_ERROR("Coast value out of bounds (0 to 16,384)");
    //        return false;
    //    }
    snprintf(cmd, DRIVER_LEN, "DSco0x%08X", steps);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeHomeAz()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGha", res) == false)
        return false;

    uint32_t home = 0;
    if (sscanf(res, "%X", &home) == 1)
    {
        SettingsN[SETTINGS_AZ_HOME].value = home * (360 / SettingsN[SETTINGS_AZ_CPR].value);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeHomeAz(uint32_t az)
{
    char cmd[DRIVER_LEN] = {0};
    uint32_t steps = static_cast<uint32_t>(az * (SettingsN[SETTINGS_AZ_CPR].value / 360));
    //    if (home >= SettingsN[SETTINGS_AZ_CPR].value)
    //    {
    //        char err[DRIVER_LEN] = {0};
    //        snprintf(err, DRIVER_LEN, "Home value out of bounds (0 to %.f, value has to be less than CPR)",
    //                 SettingsN[SETTINGS_AZ_CPR].value - 1);
    //        LOG_ERROR(err);
    //        return false;
    //    }
    snprintf(cmd, DRIVER_LEN, "DSha0x%08X", steps);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeParkAz()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGpa", res) == false)
        return false;

    uint32_t park = 0;
    if (sscanf(res, "%X", &park) == 1)
    {
        SettingsN[SETTINGS_AZ_PARK].value = park * (360 / SettingsN[SETTINGS_AZ_CPR].value);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeParkAz(uint32_t az)
{
    char cmd[DRIVER_LEN] = {0};
    uint32_t steps = static_cast<uint32_t>(az * (SettingsN[SETTINGS_AZ_CPR].value / 360));
    //    if (park >= SettingsN[SETTINGS_AZ_CPR].value)
    //    {
    //        char err[DRIVER_LEN] = {0};
    //        snprintf(err, DRIVER_LEN, "Park value out of bounds (0 to %.f, value has to be less than CPR)",
    //                 SettingsN[SETTINGS_AZ_CPR].value - 1);
    //        LOG_ERROR(err);
    //        return false;
    //    }
    snprintf(cmd, DRIVER_LEN, "DSpa0x%08X", steps);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::calibrateDomeAz(double az)
{
    char cmd[DRIVER_LEN] = {0};
    uint32_t steps = static_cast<uint32_t>(az * (SettingsN[SETTINGS_AZ_CPR].value / 360));
    //    if (steps >= SettingsN[SETTINGS_AZ_CPR].value)
    //    {
    //        char err[DRIVER_LEN] = {0};
    //        snprintf(err, DRIVER_LEN, "Degree value out of bounds");
    //        LOG_ERROR(err);
    //        return false;
    //    }
    snprintf(cmd, DRIVER_LEN, "DSca0x%08X", steps);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeAzStallCount()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGas", res) == false)
        return false;

    uint32_t count = 0;
    if (sscanf(res, "%X", &count) == 1)
    {
        SettingsN[SETTINGS_AZ_STALL_COUNT].value = count;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeAzStallCount(uint32_t count)
{
    char cmd[DRIVER_LEN] = {0};
    snprintf(cmd, DRIVER_LEN, "DSha0x%08X", count);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::getDomeAzPos()
{
    char res[DRIVER_LEN] = {0};
    if (sendCommand("DGap", res) == false)
        return false;

    uint32_t pos = 0;
    if (sscanf(res, "%X", &pos) == 1)
    {
        DomeAbsPosN[0].value = pos * (360 / SettingsN[SETTINGS_AZ_CPR].value);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::gotoDomeAz(uint32_t az)
{
    char cmd[DRIVER_LEN] = {0};
    if (az >= SettingsN[SETTINGS_AZ_CPR].value)
        return false;
    snprintf(cmd, DRIVER_LEN, "DSgo0x%08X", az);
    if (sendCommand(cmd) == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::gotoDomePark()
{
    if (sendCommand("DSgp") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::killDomeAzMovement()
{
    if (sendCommand("DXxa") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::killDomeShutterMovement()
{
    if (sendCommand("DXxs") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::openDomeShutters()
{
    if (sendCommand("DSso") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::closeDomeShutters()
{
    if (sendCommand("DSsc") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::gotoHomeDomeAz()
{
    if (sendCommand("DSah") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::discoverHomeDomeAz()
{
    if (sendCommand("DSdh") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeLeftOn()
{
    if (sendCommand("DSol") == false)
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::setDomeRightOn()
{
    if (sendCommand("DSor") == false)
        return false;
    return true;
}

*/
/////////////////////////////////////////////////////////////////////////////
/// Send Command
/////////////////////////////////////////////////////////////////////////////
bool ShelyakDT::sendCommand(const char * cmd, char * res, int cmd_len, int res_len)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;

    tcflush(PortFD, TCIOFLUSH);

    if (cmd_len > 0)
    {
        char hex_cmd[DRIVER_LEN * 3] = {0};
        hexDump(hex_cmd, cmd, cmd_len);
        LOGF_DEBUG("CMD <%s>", hex_cmd);
        rc = tty_write(PortFD, cmd, cmd_len, &nbytes_written);
    }
    else
    {
        LOGF_DEBUG("CMD <%s>", cmd);

        char formatted_command[DRIVER_LEN] = {0};
        snprintf(formatted_command, DRIVER_LEN, "!%s;", cmd);
        rc = tty_write_string(PortFD, formatted_command, &nbytes_written);
    }

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial write error: %s.", errstr);
        return false;
    }

    if (isSimulation())
        return true;

    if (res == nullptr)
        return true;

    //if (res_len > 0)
    rc = tty_read(PortFD, res, res_len, DRIVER_TIMEOUT, &nbytes_read);
        
    //else
        //rc = tty_nread_section(PortFD, res, DRIVER_LEN, DRIVER_STOP_CHAR, DRIVER_TIMEOUT, &nbytes_read);
    //rc = tty_nread_section(PortFD, res, DRIVER_LEN, 0x07, DRIVER_TIMEOUT, &nbytes_read);

    if (rc != TTY_OK)
    {
        char errstr[MAXRBUF] = {0};
        tty_error_msg(rc, errstr, MAXRBUF);
        LOGF_ERROR("Serial read error: %s.", errstr);
        LOGF_DEBUG("RES <%s> %d", res, res_len);
        return false;
    }

    if (res_len > 0)
    {
        char hex_res[DRIVER_LEN * 3] = {0};
        hexDump(hex_res, res, res_len);
        LOGF_DEBUG("RES <%s>", hex_res);
    }
    else
    {
        // Remove extra \r
        res[nbytes_read - 1] = 0;
        LOGF_DEBUG("RES <%s>", res);
    }

    tcflush(PortFD, TCIOFLUSH);

    return true;
}

/////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////
void ShelyakDT::hexDump(char * buf, const char * data, int size)
{
    for (int i = 0; i < size; i++)
        sprintf(buf + 3 * i, "%02X ", static_cast<uint8_t>(data[i]));

    if (size > 0)
        buf[3 * size - 1] = '\0';
}

/////////////////////////////////////////////////////////////////////////////
///
/////////////////////////////////////////////////////////////////////////////
std::vector<std::string> ShelyakDT::split(const std::string &input, const std::string &regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
    first{input.begin(), input.end(), re, -1},
          last;
    return {first, last};
}
