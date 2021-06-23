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
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);
}

Astelco::~Astelco()
{

}

void Astelco::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    INDI::Focuser::ISGetProperties(dev);

    //defineProperty(&ModeSP);
    //loadConfig(true, "Mode");
}

bool Astelco::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE|FOCUSER_INTERFACE|DOME_INTERFACE);//|DUSTCAP_INTERFACE);
    IUFillText(&LoginT[0], "NAME", "Username", "username");
    IUFillText(&LoginT[1], "PASS", "Password", "password");
    IUFillTextVector(&LoginTP, LoginT, 2, getDeviceName(), "Get Login", "Login", CONNECTION_TAB, IP_RW, 0, IPS_IDLE);
    
    IUFillSwitch(&OnOffS[ON], "ON", "On", ISS_OFF);
    IUFillSwitch(&OnOffS[OFF], "OFF", "Off", ISS_OFF);
    IUFillSwitch(&OnOffS[GET], "GET", "Get State", ISS_OFF);
    IUFillSwitchVector(&OnOffSP, OnOffS, ON_OFF_COUNT, getDeviceName(), "On/Off", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    
    //IUFillSwitch(&DeviceS[DOME], "DOME", "Dome", ISS_OFF);
    //IUFillSwitch(&DeviceS[FOCUS], "FOCUS", "Focus", ISS_OFF);
    //IUFillSwitch(&DeviceS[COVER], "COVER", "Cover", ISS_OFF);
    //IUFillSwitchVector(&DeviceSP, DeviceS, DEVICE_COUNT, getDeviceName(), "Device", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    
    IUFillText(&StatusMainT[UPTIME], "UPTIME", "Uptime", "NA");
    IUFillText(&StatusMainT[TELESCOPE_READY], "TELESCOPE_READY", "Ready?", "NA");
    IUFillTextVector(&StatusMainTP, StatusMainT, STATUS_MAIN_COUNT, getDeviceName(), "STATUS", "Status", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillText(&StatusT[POWER_STATE], "POWER_STATE", "State", "NA");
    IUFillText(&StatusT[REAL_POSITION], "REAL_POSITION", "State", "NA");
    IUFillText(&StatusT[LIMIT_STATE], "LIMIT_STATE", "State", "NA");
    IUFillText(&StatusT[MOTION_STATE], "MOTION_STATE", "State", "NA");
    IUFillText(&StatusT[TARGET_POSITION], "TARGET_POSITION", "State", "NA");
    IUFillText(&StatusT[OFFSET], "OFFSET", "State", "NA");
    IUFillText(&StatusT[TARGET_DISTANCE], "TARGET_DISTANCE", "State", "NA");
    IUFillTextVector(&StatusTP, StatusT, STATUS_COUNT, getDeviceName(), "STATUS", "Status", FOCUS_TAB, IP_RO, 60, IPS_IDLE);
    
    IUFillNumber(&TargetPositionN[0], "GO TO", "Go To", "%4.2f", -9999., 9999., 0., 0.);
    IUFillNumberVector(&TargetPositionNP, TargetPositionN, 1, getDeviceName(), "GOTO", "GoTo", FOCUS_TAB, IP_RW, 0, IPS_IDLE);
    IUFillNumber(&TargetOffsetN[0], "OFFSET", "Offset", "%4.2f", -9999., 9999., 0., 0.);
    IUFillNumberVector(&TargetOffsetNP, TargetOffsetN, 1, getDeviceName(), "TARGET OFFSET", "TargetOffset", FOCUS_TAB, IP_RW, 0, IPS_IDLE);
    
    IUFillSwitch(&PositionS[0], "GETP", "Get Position", ISS_OFF);
    IUFillSwitchVector(&PositionSP, PositionS, 1, getDeviceName(), "Get Positions", "", FOCUS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillText(&PositionT[REAL], "REAL_POSITION", "Real Position", "NA");
    IUFillText(&PositionT[MIN], "MIN_POSITION", "Min Position", "NA");
    IUFillText(&PositionT[MAX], "MAX_POSITION", "Max Position", "NA");
    IUFillTextVector(&PositionTP, PositionT, POSITION_COUNT, getDeviceName(), "POSITON", "Position", FOCUS_TAB, IP_RO, 60, IPS_IDLE);
    
    addAuxControls();
    defineProperty(&LoginTP);
    //deviceSet = false;
    initHistory(255);
    tcpConnection = new Connection::TCP(this);
    tcpConnection->registerHandshake([&]() { return Handshake_tcp(); });
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
        //defineProperty(&DeviceSP);
        defineProperty(&PositionSP);
        defineProperty(&PositionTP);
        defineProperty(&TargetPositionNP);
        defineProperty(&TargetOffsetNP);
        defineProperty(&StatusMainTP);
        defineProperty(&StatusTP);
    }
    else
    {
        deleteProperty(OnOffSP.name);
        //deleteProperty(DeviceSP.name);
        deleteProperty(PositionSP.name);
        deleteProperty(PositionTP.name);
        deleteProperty(TargetPositionNP.name); 
        deleteProperty(TargetOffsetNP.name); 
        deleteProperty(StatusMainTP.name);
        deleteProperty(StatusTP.name);
    }

    return true;
}

const char *Astelco::getDefaultName()
{
    return static_cast<const char *>("Astelco");
}

void Astelco::initHistory(uint8_t val)
{
    int i = 0;
    while(i<1000)
    {
        history2[i] = val;
        ++i;
    }
}

void Astelco::setHistories(char *txt, uint8_t val, DeviceE dev, PositionE pos)
{
    int idx = cmdDeviceInt%1000;
    history[idx] = txt;
    history2[idx] = val;
    history3[idx] = dev;
    history4[idx] = pos;
}

void Astelco::thread2()
{
    bool thOn = false;
    thOn = (threadOn.load(std::memory_order_acquire) == true);
    while(thOn)
    {
        int cmd = 0;
        char resp[256] = {0};
        char value[256] = {0};
        //read
        if(readResponse(resp))
        {
            //parse
            cmd =  parseAnswer(resp, value);
            //set
            setAnswer(cmd, value);
        }
        thOn = (threadOn.load(std::memory_order_acquire) == true);
    }
}

bool Astelco::createThread()
{
    std::thread t2(&Astelco::thread2, this);
    t2.detach();
    threadOn.store(true, std::memory_order_release);
    return true;
}

bool Astelco::killThread()
{
    threadOn.store(false, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::seconds(2*ASTELCO_TIMEOUT));
    return false;
}

bool Astelco::Handshake_tcp()
{
    PortFD = tcpConnection->getPortFD();  
    char resp[256]={0};
    bool succes = sendCommand("", resp);
    if(!succes)
    {
        succes = sendCommand("", resp);
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
    readResponse(resp);
    readResponse(resp);
    readResponse(resp);
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
            if(!createThread())
                return killThread();
            TimerHit();
        }
        else
        {
            return false; 
        } 
        return succes;    
    }    
    return false;
}

bool Astelco::Disconnect()
{
    killThread(); 
    char resp[256] = {0};
    sendCommand("DISCONNECT", resp);
    return INDI::DefaultDevice::Disconnect();
}

bool Astelco::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, TargetPositionNP.name))
        {
            IUUpdateNumber(&TargetPositionNP, values, names, n);
            TargetPositionNP.s = IPS_OK;
            IDSetNumber(&TargetPositionNP, nullptr);
            return SetPosition(TargetPositionN[0].value, FOCUS);
        }

        if (!strcmp(name, TargetOffsetNP.name))
        {
            IUUpdateNumber(&TargetOffsetNP, values, names, n);
            TargetOffsetNP.s = IPS_OK;
            IDSetNumber(&TargetOffsetNP, nullptr);
            return SetPosition(TargetOffsetN[0].value, FOCUS);
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

/*
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
 /**/

    if (!strcmp(name, PositionSP.name))
    {
        IUResetSwitch(&PositionSP);
        if (!strcmp(PositionS[0].name, names[0]))
        {   if (!GetPosition(FOCUS))
            {
                PositionSP.s = IPS_ALERT;
                LOGF_ERROR("Failed to get position(limits) of %s.",GetDevice(FOCUS));
                IDSetSwitch(&PositionSP, nullptr);
                return false;
            }
            else
            {
                //PositionS[0].s = ISS_ON;
                PositionSP.s = IPS_OK;
                IDSetSwitch(&PositionSP, nullptr);
                return true;
            }
        }
    }
 
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

void Astelco::TimerHit()
{
    if (!isConnected())
        return;
    GetUptime();
    OnOff(GET);
    if(turnedOn>=1)
    {
        GetStatus(POWER_STATE, FOCUS);
        GetStatus(REAL_POSITION, FOCUS);
        GetStatus(LIMIT_STATE, FOCUS);
        GetStatus(MOTION_STATE, FOCUS);
        GetStatus(TARGET_POSITION, FOCUS);
        GetStatus(OFFSET, FOCUS);
        GetStatus(TARGET_DISTANCE, FOCUS);
    }
    SetTimer(getPollingPeriod());
}

bool Astelco::GetStatus(StatusE e, DeviceE dev)
{
    char cmd[255] = {0};
    sprintf(cmd, "%d GET POSITION.INSTRUMENTAL.%s.%s", cmdDeviceInt, GetDevice(dev), GetStatusE(e));
    setHistories(StatusT[e].text, e+10, dev);
    bool succes = sendCommand(cmd);
    return succes;
}

bool Astelco::OnOff(OnOffE e)
{
    char cmd[255] = {0};
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
    setHistories(StatusMainT[TELESCOPE_READY].text, e+20);
    bool succes = sendCommand(cmd);
    return succes;
}

bool Astelco::SetPosition(const float pos, DeviceE dev)
{
    bool success = false;
    if(0<sprintf(cmdString, "%d SET POSITION.INSTRUMENTAL.%s.TARGETPOS=%f\n", cmdDeviceInt, GetDevice(dev), pos))
    {
        setHistories(StatusT[TARGET_POSITION].text, 255, dev);
        success = sendCommand(cmdString);
        return success;
    }
    return false;
}

bool Astelco::SetOffsetPosition(const float pos, DeviceE dev)
{
    bool success = false;
    if(0<sprintf(cmdString, "%d SET POSITION.INSTRUMENTAL.%s.OFFSET=%f\n", cmdDeviceInt, GetDevice(dev), pos))
    {
        setHistories(StatusT[OFFSET].text, 255, dev);
        success = sendCommand(cmdString);
        return success;
    }
    return false;
}

bool Astelco::SetLogin(const char* usr, const char* pas)
{
    if(0<sprintf(loginString, "AUTH PLAIN \"%s\" \"%s\" ", usr, pas))
        return true;
    return false;
}

/*
bool Astelco::SetDevice(const char* device)
{
    if(0<sprintf(cmdDevice, "%s", device))
        return true;
    return false;
}
/**/

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

int Astelco::GetValue(const char* cmd, char *word)
{
    int i = 0;
    int j = 0;
    while(cmd[i]!='=')
    {
        i++;
        if(cmd[i]=='\0')
        {
            word[j]='\0';
            return j;
        }
    }
    ++i;
    while(cmd[i]!='\0')
    {
        word[j]=cmd[i];
        i++;
        j++;
    }
    word[j]='\0';
    return j;
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
        case OFFSET:
            return "OFFSET";
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
    sprintf(cmd, "%d %s", cmdDeviceInt, "GET SERVER.UPTIME");
    setHistories(StatusMainT[UPTIME].text);
    bool succes = sendCommand(cmd);//increments cmdDeviceInt internally
    return succes;
}

bool Astelco::GetPosition(DeviceE dev)
{
    bool succes = false;
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS\n", cmdDeviceInt, GetDevice(dev)))
    {    
        setHistories(PositionT[REAL].text, REAL+30, dev, REAL);          
        succes = sendCommand(cmdString);
        succes = false;
    }
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS!MIN\n", cmdDeviceInt, GetDevice(dev)))
    {    
        setHistories(PositionT[MIN].text, MIN+30, dev, MIN);           
        succes = sendCommand(cmdString);
        succes = false;
    }
    if(0<sprintf(cmdString, "%d GET POSITION.INSTRUMENTAL.%s.REALPOS!MAX\n", cmdDeviceInt, GetDevice(dev)))
    {    
        setHistories(PositionT[MAX].text, MAX+30, dev, MAX);             
        succes = sendCommand(cmdString);
    }

    return succes;
}

bool Astelco::sendCommand(const char *cmd, char *resp)
{
    int nbytes_read=0, nbytes_written=0, tty_rc = 0;
    LOGF_DEBUG("CMD <%s>", cmd);
    char cmd2[256] = {0};
    //int i = 
    sprintf(cmd2, "%s \r\n", cmd);
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

    return true;
}

bool Astelco::sendCommand(const char *cmd){

    int nbytes_written=0, tty_rc = 0;
    LOGF_DEBUG("CMD <%s>", cmd);
    char cmd2[256] = {0};
    sprintf(cmd2, "%s\r\n", cmd);
    cmdDeviceInt++;
    //if(cmd[0]!='\n')
    {
        if (!isSimulation())
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
    return true;
}

bool Astelco::readResponse(char *resp)
{   
    char tmp[256]={0};
    int i = 0, nbytes_read = 0, tty_rc = 0;
    resp[0] = '\0';
    if (!isSimulation())
    {
        if ((tty_rc = tty_read_section(PortFD, tmp, '\n', ASTELCO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_WARN("Telnet read error: %s", errorMessage);
            return false;
        }
    }
    tmp[nbytes_read - 1] = '\0';
    while((tmp[i]<'0')||(tmp[i]>'9'))
        i++;
    sprintf(resp, "%s", &tmp[i]);
    LOGF_INFO("RESPONSE: <%s>", resp);

    return true;
}

void Astelco::setAnswer(const int i, const char *value)
{
    LOGF_DEBUG("%d %s", i, value);
    if(i > -1)
    {
        LOGF_DEBUG("%d %s", i, value);
        int j = i%1000;
        int myenum = history2[j];
        if(myenum<255)
        {
            if((myenum-10)==POWER_STATE)
            {
                if(strcmp(value,"-1.0")==0)
                    sprintf(history[j], "EMERGENCY STOP");
                else if(strcmp(value,"0.0")==0)
                    sprintf(history[j], "OFF");
                else if(strcmp(value,"1.0")==0)
                    sprintf(history[j], "ON");
                else
                    sprintf(history[j], "%s", value);
            }
            else if((myenum-10)==LIMIT_STATE)
            {   
                char msg1[25] = {0};
                char msg2a[25] = {0};
                char msg2b[25] = {0};
                char msg3[25] = {0};
                char msg4a[25] = {0};
                char msg4b[25] = {0};
                int ans = atoi(value);
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
                sprintf(history[j], "%s (%s %s) %s (%s %s)", msg1, msg2a, msg2b, msg3, msg4a, msg4b);
            }
            else if((myenum-10)==MOTION_STATE)
            {
                char msg1[25] = {0};
                char msg2[25] = {0};
                char msg3[25] = {0};
                char msg4[25] = {0};
                char msg5[25] = {0};
                char msg6[25] = {0};
                int ans = atoi(value);
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
                sprintf(history[j], "%s %s %s %s %s %s", msg1, msg2, msg3, msg4, msg5, msg6);
            }
            else if((myenum-10)==REAL_POSITION)
            {
                axis[history3[j]][REAL] = atof(value);
                sprintf(history[j], "%s", value);
            } 
            else if((myenum-10)==OFFSET)
            {
                axis[history3[j]][REAL] = atof(value);
                sprintf(history[j], "%s", value);
            } 
            else if((myenum-20)==GET)
            {
                if(strcmp(value,"-3.0")==0)
                    sprintf(history[j], "LOCAL MODE");
                else if(strcmp(value,"-2.0")==0)
                    sprintf(history[j], "EMERGENCY STOP");
                else if(strcmp(value,"-1.0")==0)
                    sprintf(history[j], "ERRORS block operation");
                else if(strcmp(value,"0.0")==0)
                {
                    sprintf(history[j], "SHUT DOWN");
                    turnedOn = 0;
                }
                else if(strcmp(value,"1.0")==0)
                {
                    sprintf(history[j], "FULLY OPERATIONAL");
                    turnedOn = 1;
                }
                else
                    sprintf(history[j], "%s", value);
            //parse somhow 0.0 to 1.0
            } 
            else if((myenum-30)==history3[j])
            {
                axis[history3[j]][history4[j]] = atof(value);
                sprintf(history[j], "%s", value);
            } 
        }
        else
            sprintf(history[j], "%s", value);
        // update fields
        IDSetText(&StatusTP, nullptr);
        IDSetText(&PositionTP, nullptr);
    }
}

int Astelco::parseAnswer(const char *resp, char *value)
{
    char word[256] = {0};
    int i = GetWord(resp, word);
    int idx = atoi(word);
    if(idx == 0)
    {
        LOG_WARN(&(resp[++i]));
        sprintf(value, "EE");
        return -1;
    }
    i = GetValue(&(resp[++i]), word);
    if(i>0)
    {
        sprintf(value, "%s", word);
        return idx;
    }
    return -1;
}

IPState Astelco::MoveAbsFocuser(uint32_t targetTicks)
{
    float target = (float)targetTicks;
    return MoveAbsFocuser(target/ticks_mm);
}

IPState Astelco::MoveRelFocuser(FI::FocusDirection dir, uint32_t ticks)
{
    float target{0};
    if(dir==FI::FOCUS_INWARD)
        target = (float)ticks * (-1.0);
    if(dir==FI::FOCUS_OUTWARD)
        target = (float)ticks;
    return MoveRelFocuser(target/ticks_mm);
}

IPState Astelco::MoveAbsFocuser(const float mm)
{
    if(SetPosition(mm, FOCUS))
        return IPS_BUSY;
    return IPS_ALERT;
}

IPState Astelco::MoveRelFocuser(const float mm)
{
    if(GetStatus(REAL_POSITION, FOCUS))
    {
        int idx = cmdDeviceInt -1;
        sleep(5*ASTELCO_TIMEOUT);
        float new_mm = axis[history3[idx]][REAL] + mm;
        if(SetPosition(new_mm, FOCUS))
            return IPS_BUSY;
    }
    return IPS_ALERT;
}

IPState Astelco::OffsetAbsFocuser(const float mm)
{
    if(SetOffsetPosition(mm, FOCUS))
        return IPS_BUSY;
    return IPS_ALERT;
}

IPState Astelco::OffsetRelFocuser(const float mm)
{
     if(GetStatus(OFFSET, FOCUS))
    {
        int idx = cmdDeviceInt -1;
        sleep(5*ASTELCO_TIMEOUT);
        float new_mm = axis[history3[idx]][REAL] + mm;
        if(SetOffsetPosition(new_mm, FOCUS))
            return IPS_BUSY;
    }
    return IPS_ALERT;
}