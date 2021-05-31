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

//CMakeLists.txt -> 1537-1548
//divers.xml -> 522-525

#include "netio.h"

#include "indicom.h"
//#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <termios.h>
#include <sys/ioctl.h>

// We declare an auto pointer to Netio.
std::unique_ptr<Netio> netio(new Netio());

#define FLAT_TIMEOUT 3

void ISGetProperties(const char *dev)
{
    netio->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    netio->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    netio->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    netio->ISNewNumber(dev, name, values, names, n);
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
    netio->ISSnoopDevice(root);
}


Netio::Netio()
{
    setVersion(1, 0);
    netioName[0] ='\0';
    netioPass[0] ='\0';
}


bool Netio::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE);

    IUFillSwitch(&StatusS[0], "Get", "GET", ISS_OFF);
    IUFillSwitchVector(&StatusSP, StatusS, 1, getDeviceName(), "STATUS", "Status", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    
    IUFillTextVector(&NameS1TP, NameS1T, 1, getDeviceName(), "SOCKET 1 NAME", "Socket 1 Name", "SOCKETS", IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&Socket1S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket1S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket1SP, Socket1S, 2, getDeviceName(), "SOCKET 1", "Socket 1", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillTextVector(&NameS2TP, NameS2T, 1, getDeviceName(), "SOCKET 2 NAME", "Socket 2 Name", "SOCKETS", IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&Socket2S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket2S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket2SP, Socket2S, 2, getDeviceName(), "SOCKET 2", "Socket 2", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillTextVector(&NameS3TP, NameS3T, 1, getDeviceName(), "SOCKET 3 NAME", "Socket 3 Name", "SOCKETS", IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&Socket3S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket3S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket3SP, Socket3S, 2, getDeviceName(), "SOCKET 3", "Socket 3", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillTextVector(&NameS4TP, NameS4T, 1, getDeviceName(), "SOCKET 4 NAME", "Socket 4 Name", "SOCKETS", IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&Socket4S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket4S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket4SP, Socket4S, 2, getDeviceName(), "SOCKET 4", "Socket 4", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    IUFillTextVector(&Name1TP, Name1T, 1, getDeviceName(), "USERNAME", "Username", "Main Control", IP_RW, 0, IPS_IDLE);
    IUFillTextVector(&Pass1TP, Pass1T, 1, getDeviceName(), "PASSWORD", "Password", "Main Control", IP_RW, 0, IPS_IDLE);

    addAuxControls();
    addDebugControl();
    addSimulationControl();

    defineProperty(&Name1TP);
    defineProperty(&Pass1TP);
    defineProperty(&StatusSP);
    defineProperty(&NameS1TP);
    defineProperty(&Socket1SP);
    defineProperty(&NameS2TP);
    defineProperty(&Socket2SP);
    defineProperty(&NameS3TP);
    defineProperty(&Socket3SP);
    defineProperty(&NameS4TP);
    defineProperty(&Socket4SP);
    
    //serialConnection = new Connection::Serial(this);
    //serialConnection->registerHandshake([&]() { return Handshake_serial(); });
    //serialConnection->setDefaultBaudRate(Connection::Serial::B_57600);
    // Arduino default port
    //serialConnection->setDefaultPort("/dev/ttyUSB0");
    //registerConnection(serialConnection);

    tcpConnection = new Connection::TCP(this);
    tcpConnection->registerHandshake([&]() { return Handshake_tcp(); });
    tcpConnection->setDefaultPort(23);
    tcpConnection->setConnectionType(Connection::TCP::ConnectionType::TYPE_TCP);
    registerConnection(tcpConnection);

    return true;
}

void Netio::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    defineProperty(&StatusSP);

    defineProperty(&NameS1TP);
    loadConfig(true, "SOCKET_1_NAME");
    defineProperty(&Socket1SP);

    defineProperty(&NameS2TP);
    loadConfig(true, "SOCKET_2_NAME");
    defineProperty(&Socket2SP);

    defineProperty(&NameS3TP);
    loadConfig(true, "SOCKET_3_NAME");
    defineProperty(&Socket3SP);

    defineProperty(&NameS4TP);
    loadConfig(true, "SOCKET_4_NAME");
    defineProperty(&Socket4SP);
    
}

bool Netio::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);

    IUSaveConfigText(fp, &NameS1TP);
    IUSaveConfigText(fp, &NameS2TP);
    IUSaveConfigText(fp, &NameS3TP);
    IUSaveConfigText(fp, &NameS4TP);

    return true;
}

bool Netio::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineProperty(&StatusSP);
        defineProperty(&NameS1TP);
        defineProperty(&Socket1SP);
        defineProperty(&NameS2TP);
        defineProperty(&Socket2SP);
        defineProperty(&NameS3TP);
        defineProperty(&Socket3SP);
        defineProperty(&NameS4TP);
        defineProperty(&Socket4SP);
    }
    else
    {
        //deleteProperty(StatusSP.name);
        //deleteProperty(Socket1SP.name);      
        //deleteProperty(Socket2SP.name);      
        //deleteProperty(Socket3SP.name);      
        //deleteProperty(Socket4SP.name); 
        //deleteProperty(NameS1TP.name);        
        //deleteProperty(NameS2TP.name);        
        //deleteProperty(NameS3TP.name);        
        //deleteProperty(NameS4TP.name);        
        //deleteProperty(Name1TP.name);
        //deleteProperty(Pass1TP.name);
    }

    return true;
}

const char *Netio::getDefaultName()
{
    return static_cast<const char *>("NETIO 4");
}

/*
bool Netio::Handshake_serial()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfuly to simulated %s.", getDeviceName());
        return true;
    }

    //PortFD = serialConnection->getPortFD();
    sendCommand("CONNECT#");     

    return true;
}
*/

bool Netio::Handshake_tcp()
{
    if (isSimulation())
    {
        LOGF_INFO("Connected successfuly to simulated %s.", getDeviceName());
        //return true;
    }

    PortFD = tcpConnection->getPortFD();  
        
    char buff[255];
    sprintf(buff, "login %s %s\r\n", netioName, netioPass);
    bool succes = sendCommand(buff);
    sendCommand("\0");

    TimerHit();

    return succes;
}

bool Netio::Disconnect()
{
    char buff[255];
    sprintf(buff, "quit\r\n");
    sendCommand(buff);

    return INDI::DefaultDevice::Disconnect();
}

void Netio::TimerHit()
{
    if (!isConnected())
        return;

    GetStatus();
    SetTimer(getPollingPeriod());
}

bool Netio::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if(isConnected())
    {
        if (!strcmp(name, StatusSP.name))
        {
            if (IUUpdateSwitch(&StatusSP, states, names, n) < 0)
                return false;

            if (StatusS[0].s == ISS_ON)
            {
                if( ! GetStatus() )
                {
                    IUResetSwitch(&StatusSP);
                    StatusSP.s = IPS_ALERT;
                    LOG_ERROR(" Socket status failed. ");
                    IDSetSwitch(&StatusSP, nullptr);
                    return false;
                }
                else StatusSP.s = IPS_OK;
            }
            IUResetSwitch(&StatusSP);         
            IDSetSwitch(&StatusSP, nullptr);
            return true;
        }

        if (!strcmp(name, Socket1SP.name))
        {
            if (IUUpdateSwitch(&Socket1SP, states, names, n) < 0)
                return false;

            if (Socket1S[OFF].s == ISS_ON)
            {
                if( ! TurnOff(1) )
                {
                    IUResetSwitch(&Socket1SP);
                    Socket1SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOff Socket 1 failed. ");
                    IDSetSwitch(&Socket1SP, nullptr);
                    return false;
                }
                //else Socket1SP.s = IPS_IDLE;
            }
            else if (Socket1S[ON].s == ISS_ON)
            {
                if( ! TurnOn(1) )
                {
                    IUResetSwitch(&Socket1SP);
                    Socket1SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOn Socket 1 failed. ");
                    IDSetSwitch(&Socket1SP, nullptr);
                    return false;
                }
                //else Socket1SP.s = IPS_OK;
            }
            //IUResetSwitch(&Socket1SP);         
            IDSetSwitch(&Socket1SP, nullptr);
            return true;
        }

        if (!strcmp(name, Socket2SP.name))
        {
            if (IUUpdateSwitch(&Socket2SP, states, names, n) < 0)
                return false;

            if (Socket2S[OFF].s == ISS_ON)
            {
                if( ! TurnOff(2) )
                {
                    IUResetSwitch(&Socket2SP);
                    Socket2SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOff Socket 2 failed. ");
                    IDSetSwitch(&Socket2SP, nullptr);
                    return false;
                }
                //else Socket2SP.s = IPS_IDLE;
            }
            else if (Socket2S[ON].s == ISS_ON)
            {
                if( ! TurnOn(2) )
                {
                    IUResetSwitch(&Socket2SP);
                    Socket2SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOn Socket 2 failed. ");
                    IDSetSwitch(&Socket2SP, nullptr);
                    return false;
                }
                //else Socket2SP.s = IPS_OK;
            }
            //IUResetSwitch(&Socket2SP);         
            IDSetSwitch(&Socket2SP, nullptr);
            return true;
        }

        if (!strcmp(name, Socket3SP.name))
        {
            if (IUUpdateSwitch(&Socket3SP, states, names, n) < 0)
                return false;

            if (Socket3S[OFF].s == ISS_ON)
            {
                if( ! TurnOff(3) )
                {
                    IUResetSwitch(&Socket3SP);
                    Socket3SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOff Socket 3 failed. ");
                    IDSetSwitch(&Socket3SP, nullptr);
                    return false;
                }
                //else Socket3SP.s = IPS_IDLE;
            }
            else if (Socket3S[ON].s == ISS_ON)
            {
                if( ! TurnOn(3) )
                {
                    IUResetSwitch(&Socket3SP);
                    Socket3SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOn Socket 3 failed. ");
                    IDSetSwitch(&Socket3SP, nullptr);
                    return false;
                }
                //else Socket3SP.s = IPS_OK;
            }
            //IUResetSwitch(&Socket3SP);    
            IDSetSwitch(&Socket3SP, nullptr);
            return true;
        }

        if (!strcmp(name, Socket4SP.name))
        {
            if (IUUpdateSwitch(&Socket4SP, states, names, n) < 0)
                return false;

            if (Socket4S[OFF].s == ISS_ON)
            {
                if( ! TurnOff(4) )
                {
                    IUResetSwitch(&Socket4SP);
                    Socket4SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOff Socket 4 failed. ");
                    IDSetSwitch(&Socket4SP, nullptr);
                    return false;
                }
                //else Socket4SP.s = IPS_IDLE;
            }
            else if (Socket4S[ON].s == ISS_ON)
            {
                if( ! TurnOn(4) )
                {
                    IUResetSwitch(&Socket4SP);
                    Socket4SP.s = IPS_ALERT;
                    LOG_ERROR(" TurnOn Socket 4 failed. ");
                    IDSetSwitch(&Socket4SP, nullptr);
                    return false;
                }
                //else Socket4SP.s = IPS_OK;
            }
            //IUResetSwitch(&Socket4SP);
            IDSetSwitch(&Socket4SP, nullptr);
            return true;
        }
    }
    
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool Netio::ISNewNumber(const char *dev, const char *name, double *vals, char *names[], int n)
{
    return INDI::DefaultDevice::ISNewNumber(dev, name, vals, names, n);
}

bool Netio::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (!strcmp(name, Name1TP.name))
    {
        if (IUUpdateText(&Name1TP, texts, names, n) < 0)
            return false;
        sprintf(netioName, "%s", texts[0]);
        IDSetText(&Name1TP, nullptr);
    }
    if (!strcmp(name, Pass1TP.name))
    {
        if (IUUpdateText(&Pass1TP, texts, names, n) < 0)
                return false;
        sprintf(netioPass, "%s", texts[0]);
        IDSetText(&Pass1TP, nullptr);
    }
    if (!strcmp(name, NameS1TP.name))
    {
        if (IUUpdateText(&NameS1TP, texts, names, n) < 0)
            return false;
        //sprintf(netioName, "%s", texts[0]);
        IDSetText(&NameS1TP, nullptr);
    }
    if (!strcmp(name, NameS2TP.name))
    {
        if (IUUpdateText(&NameS2TP, texts, names, n) < 0)
            return false;
        //sprintf(netioName, "%s", texts[0]);
        IDSetText(&NameS2TP, nullptr);
    }
    if (!strcmp(name, NameS3TP.name))
    {
        if (IUUpdateText(&NameS3TP, texts, names, n) < 0)
            return false;
        //sprintf(netioName, "%s", texts[0]);
        IDSetText(&NameS3TP, nullptr);
    }
    if (!strcmp(name, NameS4TP.name))
    {
        if (IUUpdateText(&NameS4TP, texts, names, n) < 0)
            return false;
        //sprintf(netioName, "%s", texts[0]);
        IDSetText(&NameS4TP, nullptr);
    }
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool Netio::sendCommand(const char *cmd, char *resp)
{
    int nbytes_read=0, nbytes_written=0, tty_rc = 0;
    
    char tmp[255];
    int tmpi = sprintf(tmp,"%s",cmd);
    tmp[tmpi-2]= '\0';
    LOGF_DEBUG("COMMAND: <%s>", tmp);
    
    if(PortFD == -1)
        PortFD = tcpConnection->getPortFD();

    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);
        if ( (tty_rc = tty_write_string(PortFD, cmd, &nbytes_written)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial write error: %s", errorMessage);
            return false;
        }
    }

    if (isSimulation())
    {
        strncpy(resp, "250 OK\r", 8);
        nbytes_read = 7;
    }
    else
    {
        if ( (tty_rc = tty_read_section(PortFD, resp, '\n', COM_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial read error: %s", errorMessage);
            return false;
        }
    }

    resp[nbytes_read - 2] = '\0';
    LOGF_DEBUG("RESPONSE: <%s>", resp);

    return true;
}

bool Netio::sendCommand(const char *cmd){
    char resp[30]={0};
    bool succes = sendCommand(cmd, resp);
    return succes;
}

// Method for initial status
bool Netio::GetStatus()
{
    char buff[255];
    sprintf(buff, "port list\r\n");
    char resp[255] = "\0";
    sendCommand(buff, resp); 
    if (isSimulation())
        parse("0110");
    else
        parse(&resp[4]);
    return true;
}

// Method for turning on of socket number i
bool Netio::TurnOn(const int i)
{
    char buff[255];
    char resp[255];
    sprintf(buff, "port %d %d\r\n", i, 1);
    bool ret = sendCommand(buff, resp); 
    if (isSimulation())
        parse("1", i, true);
    else
        parse(&resp[4], i, true);
    return ret; 
 }

// Method for turning off of socket number i
 bool Netio::TurnOff(const int i)
{
    char buff[255];
    char resp[255];
    sprintf(buff, "port %d %d\r\n", i, 0);
    bool ret = sendCommand(buff, resp); 
    if (isSimulation())
        parse("0", i, false);
    else
        parse(&resp[4], i, false);
    return ret;  
 }

// Method for parsing response with actual states of sockets
 void Netio::parse(const char* resp)
 {
    //LOGF_DEBUG("RESP: <%s>", resp);
    LOGF_INFO("STATUS: <%s>", resp);
    LOG_INFO(resp);
    //parse response to LIST command
    if(resp[0] == '0')
        Socket1SP.s = IPS_IDLE;
    else if(resp[0] == '1')
        Socket1SP.s = IPS_OK;
    if(resp[1] == '0')
        Socket2SP.s = IPS_IDLE;
    else if(resp[1] == '1')
        Socket2SP.s = IPS_OK;
    if(resp[2] == '0')
        Socket3SP.s = IPS_IDLE;
    else if(resp[2] == '1')
        Socket3SP.s = IPS_OK;
    if(resp[3] == '0')
        Socket4SP.s = IPS_IDLE;
    else if(resp[3] == '1')
        Socket4SP.s = IPS_OK;
    IDSetSwitch(&Socket1SP, nullptr);
    IDSetSwitch(&Socket2SP, nullptr);
    IDSetSwitch(&Socket3SP, nullptr);
    IDSetSwitch(&Socket4SP, nullptr);
 }

 void Netio::parse(const char *resp, const int i, const bool b)
 {
    LOGF_DEBUG("RESP: <%s> Socket %d %d", resp, i, b);
    IPState tmp = IPS_ALERT;
    if((resp[0] == 'O')&&(resp[1]=='K'))
    {    
        if(b)
            tmp = IPS_OK;
        else 
            tmp = IPS_IDLE;
    }
    switch(i){
        case 1:
            Socket1SP.s = tmp;
            break;
        case 2:
            Socket2SP.s = tmp;
            break;
        case 3:
            Socket3SP.s = tmp;
            break;
        case 4:
            Socket4SP.s = tmp;
            break;
        default:
            break;
    }
 }