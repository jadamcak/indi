/*******************************************************************************
  Copyright(c) 2021 Jan Adamcak. All rights reserved.

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

    IUFillSwitch(&Socket1S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket1S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket1SP, Socket1S, 2, getDeviceName(), "SOCKET 1", "Socket 1", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillSwitch(&Socket2S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket2S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket2SP, Socket2S, 2, getDeviceName(), "SOCKET 2", "Socket 2", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillSwitch(&Socket3S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket3S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket3SP, Socket3S, 2, getDeviceName(), "SOCKET 3", "Socket 3", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);
    IUFillSwitch(&Socket4S[OFF], "OFF", "OFF", ISS_OFF);    
    IUFillSwitch(&Socket4S[ON], "ON", "ON", ISS_OFF);
    IUFillSwitchVector(&Socket4SP, Socket4S, 2, getDeviceName(), "SOCKET 4", "S4ocket 4", 
                        "SOCKETS", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    IUFillTextVector(&Name1TP, Name1T, 1, getDeviceName(), "USERNAME", "Username", "Main Control", IP_RW, 0, IPS_IDLE);
    IUFillTextVector(&Pass1TP, Pass1T, 1, getDeviceName(), "PASSWORD", "Password", "Main Control", IP_RW, 0, IPS_IDLE);

    addAuxControls();
    addDebugControl();
    addSimulationControl();

    defineText(&Name1TP);
    defineText(&Pass1TP);
    
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

bool Netio::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineSwitch(&Socket1SP);
        defineSwitch(&Socket2SP);
        defineSwitch(&Socket3SP);
        defineSwitch(&Socket4SP);
    }
    else
    {
        deleteProperty(Socket1SP.name);      
        deleteProperty(Socket2SP.name);      
        deleteProperty(Socket3SP.name);      
        deleteProperty(Socket4SP.name);       
        //deleteProperty(Name1TP.name);
        //deleteProperty(Pass1TP.name);
    }

    return true;
}

const char *Netio::getDefaultName()
{
    return static_cast<const char *>("NETIO");
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
    sendCommand(buff);  
    sprintf(buff, "port list\r\n");
    char resp[255];
    sendCommand(buff, resp); 
    sprintf(buff, "quit\r\n");
    sendCommand(buff); 
    parse(resp);

    return true;
}

bool Netio::Disconnect()
{
    //char buff[255];
    //sprintf(buff, "quit\r\n");
    //sendCommand(buff);

    return INDI::DefaultDevice::Disconnect();
}

bool Netio::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
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
                else Socket1SP.s = IPS_IDLE;
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
                else Socket1SP.s = IPS_OK;
            }
            IUResetSwitch(&Socket1SP);         
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
                else Socket2SP.s = IPS_IDLE;
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
                else Socket2SP.s = IPS_OK;
            }
            IUResetSwitch(&Socket2SP);         
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
                else Socket3SP.s = IPS_IDLE;
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
                else Socket3SP.s = IPS_OK;
            }
            IUResetSwitch(&Socket3SP);    
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
                else Socket4SP.s = IPS_IDLE;
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
                else Socket4SP.s = IPS_OK;
            }
            IUResetSwitch(&Socket4SP);
            IDSetSwitch(&Socket4SP, nullptr);
            return true;
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
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool Netio::sendCommand(const char *cmd, char *resp)
{
    int nbytes_read=0, nbytes_written=0, tty_rc = 0;
    
    char tmp[255];
    int tmpi = sprintf(tmp,"%s",cmd);
    tmp[tmpi-2]= '\0';
    LOGF_DEBUG("COMMAND: <%s>", tmp);

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

    resp[nbytes_read - 1] = '\0';
    LOGF_DEBUG("RESPONSE: <%s>", resp);

    return true;
}

bool Netio::sendCommand(const char *cmd){
    char resp[30]={0};
    bool succes = sendCommand(cmd, resp);
    return succes;
}

// Method for turning on of socket number i
bool Netio::TurnOn(const int i)
{
    char buff[255];
    sprintf(buff, "login %s %s\r\n", netioName, netioPass);
    sendCommand(buff); 
    sprintf(buff, "port %d %d\r\n", i, 1);
    sendCommand(buff); 
    sprintf(buff, "quit\r\n");
    return sendCommand(buff); 
 }

// Method for turning off of socket number i
 bool Netio::TurnOff(const int i)
{
    char buff[255];
    sprintf(buff, "login %s %s\r\n", netioName, netioPass);
    sendCommand(buff); 
    sprintf(buff, "port %d %d\r\n", i, 0);
    sendCommand(buff); 
    sprintf(buff, "quit\r\n");
    return sendCommand(buff); 
 }

// Method for parsing response with actual states of sockets
 void Netio::parse(const char* resp)
 {
    LOGF_DEBUG("RESP: <%s>", resp);
    //parse response to LIST command
    int inc = 0;
    int idx = 0;
    switch (sizeof(resp))
    {
        case 4:
        case 10:
            inc = 1;
            break;
        case 7:
        case 13:
            inc = 2;
            break;
        case 6:
        default:
            break;
    }
    if(resp[idx] == '0')
        Socket1SP.s = IPS_IDLE;
    else if(resp[idx] == '1')
        Socket1SP.s = IPS_OK;
    idx += inc;
    if(resp[idx] == '0')
        Socket2SP.s = IPS_IDLE;
    else if(resp[idx] == '1')
        Socket2SP.s = IPS_OK;
    idx += inc;
    if(resp[idx] == '0')
        Socket3SP.s = IPS_IDLE;
    else if(resp[idx] == '1')
        Socket3SP.s = IPS_OK;
    idx += inc;
    if(resp[idx] == '0')
        Socket4SP.s = IPS_IDLE;
    else if(resp[idx] == '1')
        Socket4SP.s = IPS_OK;
 }