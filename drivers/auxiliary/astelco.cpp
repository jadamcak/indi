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


#include "astelco.h"

#include "indicom.h"
//#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

// We declare an auto pointer to Astelco.
std::unique_ptr<Astelco> astelco(new Astelco());

void ISGetProperties(const char *dev)
{
    astelco->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    astelco->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    astelco->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    astelco->ISNewNumber(dev, name, values, names, n);
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
    astelco->ISSnoopDevice(root);
}

Astelco::Astelco()
{
    setVersion(1, 0);
}

bool Astelco::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE|FOCUSER_INTERFACE|DOME_INTERFACE|DUSTCAP_INTERFACE);
    IUFillText(&LoginT[0], "NAME", "Username", "username");
    IUFillText(&LoginT[1], "PASS", "Password", "password");
    IUFillTextVector(&LoginTP, LoginT, 2, getDeviceName(), "Get Login", "Login", CONNECTION_TAB, IP_RW, 0, IPS_IDLE);
    
    IUFillSwitch(&OnOffS[ON], "ON", "On", ISS_OFF);
    IUFillSwitch(&OnOffS[OFF], "OFF", "Off", ISS_OFF);
    IUFillSwitch(&OnOffS[GET], "GET", "Get State", ISS_OFF);
    IUFillSwitchVector(&OnOffSP, OnOffS, ON_OFF_COUNT, getDeviceName(), "On/Off", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    
    IUFillSwitch(&DeviceS[DOME], "DOME", "Dome", ISS_OFF);
    IUFillSwitch(&DeviceS[FOCUS], "FOCUS", "Focus", ISS_OFF);
    IUFillSwitch(&DeviceS[COVER], "COVER", "Cover", ISS_OFF);
    IUFillSwitchVector(&DeviceSP, DeviceS, DEVICE_COUNT, getDeviceName(), "Device", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    
    IUFillText(&StatusT[UPTIME], "UPTIME", "Uptime", "NA");
    IUFillText(&StatusT[TELESCOPE_READY], "TELESCOPE_READY", "Ready?", "NA");
    IUFillText(&StatusT[POWER_STATE], "POWER_STATE", "State", "NA");
    IUFillText(&StatusT[REAL_POSITION], "REAL_POSITION", "State", "NA");
    IUFillText(&StatusT[LIMIT_STATE], "LIMIT_STATE", "State", "NA");
    IUFillText(&StatusT[MOTION_STATE], "MOTION_STATE", "State", "NA");
    IUFillText(&StatusT[TARGET_POSITION], "TARGET_POSITION", "State", "NA");
    IUFillText(&StatusT[TARGET_DISTANCE], "TARGET_DISTANCE", "State", "NA");
    IUFillTextVector(&StatusTP, StatusT, STATUS_COUNT, getDeviceName(), "STATUS", "Status", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    
    IUFillNumber(&TargetPositionN[0], "GO TO", "Go To", "%3.2f", -50., 70., 0., 0.);
    IUFillNumberVector(&TargetPositionNP, TargetPositionN, 1, getDeviceName(), "GOTO", "GoTo", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);
    
    IUFillSwitch(&PositionS[0], "GETP", "Get Position", ISS_OFF);
    IUFillSwitchVector(&PositionSP, PositionS, 1, getDeviceName(), "Get Positions", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillText(&PositionT[REAL], "REAL_POSITION", "Real Position", "NA");
    IUFillText(&PositionT[MIN], "MIN_POSITION", "Min Position", "NA");
    IUFillText(&PositionT[MAX], "MAX_POSITION", "Max Position", "NA");
    IUFillTextVector(&PositionTP, PositionT, POSITION_COUNT, getDeviceName(), "POSITON", "Position", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    
    addAuxControls();
    defineProperty(&LoginTP);
    deviceSet = false;
    tcpConnection = new Connection::TCP(this);
    tcpConnection->registerHandshake([&]() { return Handshake_tcp(); });
    //tcpConnection->setDefaultPort(23);
    //tcpConnection->setConnectionType(Connection::TCP::ConnectionType::TYPE_TCP);
    registerConnection(tcpConnection);

    return true;
}

bool Astelco::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    defineProperty(&LoginTP);
    if (isConnected())
    {
        defineProperty(&OnOffSP);
        defineProperty(&DeviceSP);
        defineProperty(&PositionSP);
        defineProperty(&PositionTP);
        defineProperty(&TargetPositionNP);
        defineProperty(&StatusTP);
    }
    else
    {
        deleteProperty(OnOffSP.name);
        deleteProperty(DeviceSP.name);
        deleteProperty(PositionSP.name);
        deleteProperty(PositionTP.name);
        deleteProperty(TargetPositionNP.name); 
        deleteProperty(StatusTP.name);
    }

    return true;
}

const char *Astelco::getDefaultName()
{
    return static_cast<const char *>("Astelco");
}

bool Astelco::Handshake_tcp()
{
    PortFD = tcpConnection->getPortFD();  
    char resp[256]={0};
    bool succes = sendCommand("\0", resp);
    if(!succes)
    {
        succes = sendCommand("\0", resp);
        if(!succes)
            return false;
    } 
    if(strlen(resp)<5)
    {
        LOGF_INFO("TPL2: %d",strlen(resp));
        return false;
    }
    char resp2[256]={0};
    SetLogin(LoginT[0].text, LoginT[1].text);
    succes = sendCommand(loginString, resp);
    if(succes)
    {
        int i = GetWord(resp, resp2);
        i += GetWord(&resp[++i], &resp2[50]);
        i += GetWord(&resp[++i], &resp2[100]);
        i += GetWord(&resp[++i], &resp2[200]);
        readRules = atoi(&resp2[100]);
        writeRules = atoi(&resp2[200]);
        if((resp2[50]=='O')||(resp2[51]=='K'))
        {
            //connected = true;
            LOGF_INFO("%s",&resp2[50]);
            TimerHit();
        }
        else
            return false;        
    }    
     
    return succes;
}

bool Astelco::Disconnect()
{
    sendCommand("DISCONNECT");
    //connected = false;    
    return INDI::DefaultDevice::Disconnect();
}

bool Astelco::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, TargetPositionNP.name))
        {
            IUUpdateNumber(&TargetPositionNP, values, names, n);
            if(deviceSet)
            {
                TargetPositionNP.s = IPS_OK;
                IDSetNumber(&TargetPositionNP, nullptr);
                return SetPosition(TargetPositionN[0].value);
            }
            else
            {
                LOG_WARN("NEED to choose DEVICE");
                TargetPositionNP.s = IPS_ALERT;
                return false;
            }
        }
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool Astelco::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, LoginTP.name))
        {
            IUUpdateText(&LoginTP, texts, names, n);
            if (SetLogin(LoginT[0].text, LoginT[1].text))
                LoginTP.s = IPS_OK;
            else
                LoginTP.s = IPS_ALERT;
            IDSetText(&LoginTP, nullptr);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool Astelco::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (!strcmp(name, OnOffSP.name))
    {
        IUResetSwitch(&OnOffSP);
        for (int i = 0; i < OnOffSP.nsp; i++)
        {
            if (states[i] != ISS_ON)
                continue;
            if (!strcmp(OnOffS[ON].name, names[i]))
            {
                if (!OnOff(ON))
                {
                    OnOffSP.s = IPS_ALERT;
                    LOG_ERROR("Failed to Turn On Astelco.");
                }
                else
                {
                    OnOffS[ON].s = ISS_ON;
                    OnOffS[OFF].s = ISS_OFF;
                    OnOffSP.s = IPS_BUSY;
                }
            }
            else if (!strcmp(OnOffS[OFF].name, names[i]))
            {
                if (!OnOff(OFF))
                {
                    OnOffSP.s = IPS_ALERT;
                    LOG_ERROR("Failed to Turn On Astelco.");
                }
                else
                {
                    OnOffS[ON].s = ISS_OFF;
                    OnOffS[OFF].s = ISS_ON;
                    OnOffSP.s = IPS_BUSY;
                }
            }
            else if (!strcmp(OnOffS[GET].name, names[i]))
            {
                if (!OnOff(GET))
                {
                    OnOffSP.s = IPS_ALERT;
                    LOG_ERROR("Failed to Get State of Astelco.");
                }
                else
                {
                    OnOffS[GET].s = ISS_OFF;
                    OnOffSP.s = IPS_BUSY;
                }
            }
        
            IDSetSwitch(&OnOffSP, nullptr);
            return true;
        }
    }

    if (!strcmp(name, DeviceSP.name))
    {
        IUResetSwitch(&DeviceSP);
        for (int i = 0; i < DeviceSP.nsp; i++)
        {
            if (states[i] != ISS_ON)
                continue;                
            if (!strcmp(DeviceS[DOME].name, names[i]))
            {
                if (!SetDevice(GetDevice(DOME)))
                {
                    DeviceSP.s = IPS_ALERT;
                    LOGF_ERROR("Failed to switch to %s.",GetDevice(DOME));
                    deviceSet = false;
                }
                else
                {
                    DeviceS[DOME].s = ISS_ON;
                    DeviceS[FOCUS].s = ISS_OFF;
                    DeviceS[COVER].s = ISS_OFF;
                    DeviceSP.s = IPS_BUSY;
                    deviceSet = true;
                }
            }
            else if (!strcmp(DeviceS[FOCUS].name, names[i]))
            {
                if (!SetDevice(GetDevice(FOCUS)))
                {
                    DeviceSP.s = IPS_ALERT;
                    LOGF_ERROR("Failed to switch to %s.", GetDevice(FOCUS));
                    deviceSet = false;
                }
                else
                {
                    DeviceS[FOCUS].s = ISS_ON;
                    DeviceS[DOME].s = ISS_OFF;                    
                    DeviceS[COVER].s = ISS_OFF;
                    DeviceSP.s = IPS_BUSY;
                    deviceSet = true;
                }
            }
            else if (!strcmp(DeviceS[COVER].name, names[i]))
            {
                if (!SetDevice(GetDevice(COVER)))
                {
                    DeviceSP.s = IPS_ALERT;
                    LOGF_ERROR("Failed to switch to %s.", GetDevice(COVER));
                    deviceSet = false;
                }
                else
                {
                    DeviceS[COVER].s = ISS_ON;
                    DeviceS[DOME].s = ISS_OFF;  
                    DeviceS[FOCUS].s = ISS_OFF;
                    DeviceSP.s = IPS_BUSY;
                    deviceSet = true;
                }
            }
            IDSetSwitch(&DeviceSP, nullptr);
            return true;
        }
    }
 
    if (!strcmp(name, PositionSP.name))
    {
        IUResetSwitch(&PositionSP);
        if (!strcmp(PositionS[0].name, names[0]))
        {   if(deviceSet)
            {
                char real[256], min_pos[256], max_pos[256];
                if (!GetPosition(real, min_pos, max_pos))
                {
                    PositionSP.s = IPS_ALERT;
                    LOGF_ERROR("Failed to switch to %s.",GetDevice(DOME));
                }
                else
                {
                    sprintf(PositionT[REAL].text, "%s", real);
                    sprintf(PositionT[MIN].text, "%s", min_pos);
                    sprintf(PositionT[MAX].text, "%s", max_pos);
                    //PositionS[0].s = ISS_ON;
                    PositionSP.s = IPS_OK;
                }
            }
            else
            {
                LOG_WARN("NEED to choose DEVICE");
                PositionSP.s = IPS_ALERT;
                IDSetSwitch(&PositionSP, nullptr);
                return false;
            }
        }
            
        IDSetSwitch(&PositionSP, nullptr);
        return true;
    }
 
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

void Astelco::TimerHit()
{
    if (!isConnected())
        return;
    //if (!connected)
    //    return;
    GetUptime();
    OnOff(GET);
    if((turnedOn>=1)&&(deviceSet))
    {
        GetStatus(POWER_STATE);
        GetStatus(REAL_POSITION);
        GetStatus(LIMIT_STATE);
        GetStatus(MOTION_STATE);
        GetStatus(TARGET_POSITION);
        GetStatus(TARGET_DISTANCE);
    }
    SetTimer(getPollingPeriod());
}

bool Astelco::GetStatus(StatusE e)
{
    char cmd[255] = {0};
    char resp[255] = {0};
    sprintf(cmd, "%d GET POSITION.INSTRUMENTAL.%s.%s", cmdDeviceInt, cmdDevice, GetStatusE(e));
    bool succes = sendCommand(cmd, resp);
    if(e==POWER_STATE)
    {
        if(strcmp(resp,"-1.0")==0)
            sprintf(StatusT[e].text, "EMERGENCY STOP");
        else if(strcmp(resp,"0.0")==0)
            sprintf(StatusT[e].text, "OFF");
        else if(strcmp(resp,"1.0")==0)
            sprintf(StatusT[e].text, "ON");
        else
            sprintf(StatusT[e].text, "%s", resp);
    }
    else if(e==LIMIT_STATE)
    {   
        char msg1[25] = {0};
        char msg2a[25] = {0};
        char msg2b[25] = {0};
        char msg3[25] = {0};
        char msg4a[25] = {0};
        char msg4b[25] = {0};
        int ans = atoi(resp);
        if(ans&0x0080)
            sprintf(msg1, "HW LIMIT BLOCKING");
        if(ans&0x0001)
            sprintf(msg2a, "MIN-HW");
        if(ans&0x0002)
            sprintf(msg2b, "MAX-HW");
        if(ans&0x8000)
            sprintf(msg3, "SW LIMIT BLOCKING");
        if(ans&0x0100)
            sprintf(msg4a, "MIN-SW");
        if(ans&0x0200)
            sprintf(msg4b, "MAX-SW");
        sprintf(StatusT[e].text, "%s (%s %s) %s (%s %s)", msg1, msg2a, msg2b, msg3, msg4a, msg4b);
    }
    else if(e==MOTION_STATE)
    {
        char msg1[25] = {0};
        char msg2[25] = {0};
        char msg3[25] = {0};
        char msg4[25] = {0};
        char msg5[25] = {0};
        char msg6[25] = {0};
        int ans = atoi(resp);
        if(ans&0x01)
            sprintf(msg1, "Axis_Moving");
        if(ans&0x02)
            sprintf(msg2, "Trajectory");
        if(ans&0x04)
            sprintf(msg3, "LIMIT_STATE");
        if(ans&0x08)
            sprintf(msg4, "Target");
        if(ans&0x10)
            sprintf(msg5, "Too_Fast_Move");
        if(ans&0x20)
            sprintf(msg6, "Unparking/ed");
        if(ans&0x40)
            sprintf(msg6, "Parking/ed");
        sprintf(StatusT[e].text, "%s %s %s %s %s %s", msg1, msg2, msg3, msg4, msg5, msg6);
    }
    else
        StatusT[e].text = resp;
    return succes;
}

bool Astelco::OnOff(OnOffE e)
{
    char cmd[255] = {0};
    char resp[255] = {0};
    switch (e)
    {
        case ON:
            sprintf(cmd, "%d SET TELESCOPE.READY=%d", cmdDeviceInt, 1);
            break;
        case OFF:
            sprintf(cmd, "%d SET TELESCOPE.READY=%d", cmdDeviceInt, 0);
            break;
        case GET:
            sprintf(cmd, "%d GET TELESCOPE.READY_STATE", cmdDeviceInt);
            break;
        default:
        break;
    }
    bool succes = sendCommand(cmd, resp);
    if(e==GET)
    {
        if(strcmp(resp,"-3.0")==0)
            sprintf(StatusT[TELESCOPE_READY].text, "LOCAL MODE");
        else if(strcmp(resp,"-2.0")==0)
            sprintf(StatusT[TELESCOPE_READY].text, "EMERGENCY STOP");
        else if(strcmp(resp,"-1.0")==0)
            sprintf(StatusT[TELESCOPE_READY].text, "ERRORS block operation");
        else if(strcmp(resp,"0.0")==0)
            sprintf(StatusT[TELESCOPE_READY].text, "SHUT DOWN");
        else if(strcmp(resp,"1.0")==0)
            sprintf(StatusT[TELESCOPE_READY].text, "FULLY OPERATIONAL");
        else
            sprintf(StatusT[TELESCOPE_READY].text, "%s", resp);
        //parse somhow 0.0 to 1.0
    }
    return succes;
}

bool Astelco::SetPosition(const float pos)
{
    if(0<sprintf(cmdString, "%d SET POSITION.INSTRUMENTAL.%s.TARGETPOS=%f\n", cmdDeviceInt, cmdDevice, pos))
        return sendCommand(cmdString);
    return false;
}

bool Astelco::SetLogin(const char* usr, const char* pas)
{
    if(0<sprintf(loginString, "AUTH PLAIN \"%s\" \"%s\"", usr, pas))
        return true;
    return false;
}

bool Astelco::SetDevice(const char* device)
{
    if(0<sprintf(cmdDevice, "%s", device))
        return true;
    return false;
}

int Astelco::GetWord(const char* cmd, char *word)
{
    int i = 0;
    while(cmd[i]!=' ')
    {
        word[i]=cmd[i];
        i++;
        if(cmd[i]=='\n')
            break;
    }
    word[i]='\0';
    return i;
}

const char* Astelco::GetDevice(DeviceE e)
{
    switch (e)
    {
        case DOME:
            return "DOME[0]";
        break;
        case FOCUS:
            return "FOCUS";
        break;
        case COVER:
            return "COVER";
        break;
        default:
            return "";
        break;
    }
    return "";
}

const char* Astelco::GetStatusE(StatusE e)
{
    switch (e)
    {
        case POWER_STATE:
            return "POWER_STATE";
        break;
        case REAL_POSITION:
            return "REALPOS";
        break;
        case LIMIT_STATE:
            return "LIMIT_STATE";
        break;
        case MOTION_STATE:
            return "MOTION_STATE";
        break;
        case TARGET_POSITION:
            return "TARGETPOS";
        break;
        case TARGET_DISTANCE:
            return "TARGETDISTANCE";
        break;
        default:
            return "";
        break;
    }
    return "";
}

bool Astelco::GetUptime()
{
    char cmd[254] = {0};
    char resp[255] = {0};
    sprintf(cmd, "%d %s", cmdDeviceInt, "GET SERVER.UPTIME");
    bool succes = sendCommand(cmd, resp);
    StatusT[UPTIME].text = resp;
    return succes;
}

bool Astelco::GetPosition(char *real, char *min_real, char *max_real)
{
    bool succes = false;
    char resp[256] = {0};
    char resp2[256] = {0};
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS\n", cmdDeviceInt, cmdDevice))
    {    
        succes = sendCommand(cmdString, resp);
        if(succes)
        {
            int i = GetWord(resp, resp2);
            i += GetWord(&resp[++i], real);
            LOGF_INFO("Real position: %s", real);
        }            
        succes = false;
    }
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS!MIN\n", cmdDeviceInt, cmdDevice))
    {    
        succes = sendCommand(cmdString, resp);
        if(succes)
        {
            int i = GetWord(resp, resp2);
            i += GetWord(&resp[++i], min_real);
            LOGF_INFO("Minimal position: %s", min_real);
        }            
        succes = false;
    }
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS!MAX\n", cmdDeviceInt, cmdDevice))
    {    
        succes = sendCommand(cmdString, resp);
        if(succes)
        {
            int i = GetWord(resp, resp2);
            i += GetWord(&resp[++i], max_real);
            LOGF_INFO("Maximal position: %s", max_real);
        }            
        //succes = false;
    }

    return succes;
}

bool Astelco::sendCommand(const char *cmd, char *resp)
{
    int nbytes_read=0, nbytes_written=0, tty_rc = 0;
    LOGF_DEBUG("CMD <%s>", cmd);
    char cmd2[256] = {0};
    sprintf(cmd2, "%s\n", cmd);
    char command[30] = {0};
    cmdDeviceInt++;
    //if(cmd[0]!='\n')
    {
        if (isSimulation())
        {
            GetWord(cmd,command);
            if(cmd[0]=='\0')
                LOGF_INFO("Establishing simulated connection to %s.", getDeviceName());
            if(!strcmp(command, "AUTH"))
                LOGF_INFO("Autenthication simulated to %s.", getDeviceName());
            if(!strcmp(command, "DISCONNECT"))
                LOGF_INFO("Disconnection simulated to %s.", getDeviceName());
        }
        else
        {
            tcflush(PortFD, TCIOFLUSH);
            if ( (tty_rc = tty_write_string(PortFD, cmd2, &nbytes_written)) != TTY_OK)
            {
                char errorMessage[MAXRBUF];
                tty_error_msg(tty_rc, errorMessage, MAXRBUF);
                LOGF_ERROR("Telnet write error: %s", errorMessage);
                return false;
            }
        }
    }
    if (isSimulation())
    {
        if(cmd[0]=='\0')
            nbytes_read = sprintf(resp, "TPL2 %d.%d CONN %d AUTH %s ENC %s %s\n", 2,1,1,"[PLAIN]", "[NONE]", "[MESSAGE Wellcome]"); 
        else if(!strcmp(command, "AUTH"))
            nbytes_read = sprintf(resp, "%s OK %d %d\n", command, 0, 0); 
        else 
            nbytes_read = sprintf(resp, "%s OK\n", command);
    }
    else
    {
        if ((tty_rc = tty_read_section(PortFD, resp, '\n', ASTELCO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Telnet read error: %s", errorMessage);
            return false;
        }
    }

    resp[nbytes_read - 1] = '\0';
    LOGF_INFO("RESPONSE: <%s>", resp);
    LOGF_DEBUG("RES <%s>", resp);

    return true;
}

bool Astelco::sendCommand(const char *cmd){
    char resp[256]={0};
    bool succes = sendCommand(cmd, resp);
    // parse resp
    return succes;
}