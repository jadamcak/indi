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

#pragma once

#include "indibase/indidome.h"
#include <map>

class ShelyakDT : public INDI::Dome
{
    public:
        ShelyakDT();
        virtual ~ShelyakDT() override = default;

        virtual const char *getDefaultName() override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool saveConfigItems(FILE *fp) override;


    protected:
        virtual bool initProperties() override;
        virtual bool updateProperties() override;

        virtual bool Handshake() override;
        virtual void TimerHit() override;

        virtual IPState MoveRel(double azDiff) override;
        virtual IPState MoveAbs(double az) override;
        virtual IPState Move(DomeDirection dir, DomeMotionCommand operation) override;
        virtual IPState ControlShutter(ShutterOperation operation) override;
        virtual bool Abort() override;
        virtual bool Sync(double az) override;

        // Parking
        virtual IPState Park() override;
        virtual IPState UnPark() override;
        virtual bool SetCurrentPark() override;
        virtual bool SetDefaultPark() override;

    private:

        ShutterOperation m_TargetShutter { SHUTTER_OPEN };
        double targetAz {0};
        int moveTID {0};

        ///////////////////////////////////////////////////////////////////////////////
        /// Setup Functions
        ///////////////////////////////////////////////////////////////////////////////
        bool setupInitialParameters();
        uint8_t processShutterStatus();
        uint8_t processDomeStatus();


        ///////////////////////////////////////////////////////////////////////////////
        /// Query Functions
        ///////////////////////////////////////////////////////////////////////////////
        //unused
        bool getFirmwareVersion();
        bool getHardwareConfig();
        bool getDomeStatus();
        bool getShutterStatus();
        bool getDomeAzCPR();
        bool getDomeAzCoast();
        bool getDomeAzPos();
        bool getDomeHomeAz();
        bool getDomeParkAz();
        bool getDomeAzStallCount();


        ///////////////////////////////////////////////////////////////////////////////
        /// Set Functions
        ///////////////////////////////////////////////////////////////////////////////
        bool setDomeAzCPR(uint32_t cpr);
        bool gotoDomeAz(uint32_t az);
        bool setDomeAzCoast(uint32_t coast);
        bool killDomeAzMovement();
        bool killDomeShutterMovement();
        bool setDomeHomeAz(uint32_t az);
        bool setDomeParkAz(uint32_t az);
        bool setDomeAzStallCount(uint32_t steps);
        bool calibrateDomeAz(double az);
        bool gotoDomePark();
        bool openDomeShutters();
        bool closeDomeShutters();
        bool gotoHomeDomeAz();
        bool discoverHomeDomeAz();
        bool setDomeLeftOn();
        bool setDomeRightOn();


        ///////////////////////////////////////////////////////////////////////////////
        /// Parking, Homing, and Calibration
        ///////////////////////////////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////////////////////////////
        /// Communication Functions
        ///////////////////////////////////////////////////////////////////////////////

        /**
                 * @brief sendCommand Send a string command to device.
                 * @param cmd Command to be sent. Can be either NULL TERMINATED or just byte buffer.
                 * @param res If not nullptr, the function will wait for a response from the device. If nullptr, it returns true immediately
                 * after the command is successfully sent.
                 * @param cmd_len if -1, it is assumed that the @a cmd is a null-terminated string. Otherwise, it would write @a cmd_len bytes from
                 * the @a cmd buffer.
                 * @param res_len if -1 and if @a res is not nullptr, the function will read until it detects the default delimeter DRIVER_STOP_CHAR
                 *  up to DRIVER_LEN length. Otherwise, the function will read @a res_len from the device and store it in @a res.
                 * @return True if successful, false otherwise.
        */
        bool sendCommand(const char * cmd, char * res = nullptr, int cmd_len = -1, int res_len = -1);
        void hexDump(char * buf, const char * data, int size);
        std::vector<std::string> split(const std::string &input, const std::string &regex);

        ///////////////////////////////////////////////////////////////////////////////////
        /// Properties
        ///////////////////////////////////////////////////////////////////////////////////
        ITextVectorProperty VersionTP;
        IText VersionT[2] {};
        enum
        {
            VERSION_FIRMWARE,
            VERSION_HARDWARE
        };

        ISwitchVectorProperty HomeSP;
        ISwitch HomeS[2] {};
        enum
        {
            HOME_DISCOVER,
            HOME_GOTO
        };

        ISwitchVectorProperty RegimeSP;
        ISwitch RegimeS[2] {};
        enum Regime
        {
            CLASSIC_DOME,
            ROLL_OFF_ROOF
        };

        ISwitchVectorProperty DomeMotionSP3;
        ISwitch DomeMotionS3[3] {};
        enum DomMotion
        {
            DOM_CW,
            DOM_CCW,
            DOM_IDLE
        };

        ISwitchVectorProperty DomeShutterSP3;
        ISwitch DomeShutterS3[3] {};
        enum ShutStatus
        {
            SHUT_OPEN,
            SHUT_CLOSE,
            SHUT_IDLE
        };
        
        ISwitchVectorProperty DomeResetSP;
        ISwitch DomeResetS[2] {};
        enum ResetDir
        {
            DIRECTION_POSITIVE,
            DIRECTION_NEGATIVE
        };

        ISwitchVectorProperty CalibrateSP;
        ISwitch CalibrateS[1] {};
        enum
        {
            CALIBRATE
        };

        ITextVectorProperty StatusTP;
        IText StatusT[7] {};
        enum
        {
            STATUS_DOME,
            STATUS_SHUTTER,
            ENCODER_VALUE,
            INDICATOR_SWITCH,
            EMERGENCY_CLOSE_SW,
            TELESCOPE_SECURED,
            RESET_SWITCH
        };

        INumberVectorProperty DomeShutterNP;
        INumber DomeShutterN[1];
        enum
        {
            DOME_SHUTTER_MOVE_TIME
        };
        
        INumberVectorProperty SettingsNP;
        INumber SettingsN[5];
        enum
        {
            SETTINGS_AZ_CPR,
            SETTINGS_AZ_COAST,
            SETTINGS_AZ_HOME,
            SETTINGS_AZ_PARK,
            SETTINGS_AZ_STALL_COUNT
        };

        /////////////////////////////////////////////////////////////////////////////
        /// Static Helper Values
        /////////////////////////////////////////////////////////////////////////////
        static constexpr const char * INFO_TAB = "Info";
        static constexpr const char * SETTINGS_TAB = "Settings";
        // 0x3B is the stop char
        //static const char DRIVER_STOP_CHAR { 0x3B };
        // Wait up to a maximum of 3 seconds for serial input
        static constexpr const uint8_t DRIVER_TIMEOUT {1};
        // Maximum buffer for sending/receving.
        static constexpr const uint8_t DRIVER_LEN {32};
        // Dome AZ Threshold
        static constexpr const double DOME_AZ_THRESHOLD {0.01};
        // Dome hardware types
        const std::map<uint8_t, std::string> DomeHardware =
        {
            {0xAA, "ShelyakDT for classic domes"},
            {0xCC, "ShelyakDT for roll-off roof"}
        };
        uint8_t roofType{0xAA};
        uint32_t shutterTime{0};
        // Shutter statuses
        const std::map<uint8_t, std::string> ShutterStatus =
        {
            {0x00, "Undefined"},
            {0x01, "Idle"},
            {0x10, "Opening"},
            {0x11, "Opened"},
            {0x20, "Closing"},
            {0x21, "Closed"}
        };
        // Dome statuses
        const std::map<uint8_t, std::string> DomeStatus =
        {
            {0x00, "Idle"},
            {0x01, "Reseting +"},
            {0x02, "Reseting -"},
            {0x04, "Moving +"},
            {0x08, "Moving -"},
            {0x09, "Counting steps"}
        };

        ///////////////////////////////////////////////////////////////////////////////
        /// Query Functions
        ///////////////////////////////////////////////////////////////////////////////
        uint8_t getCRC(char *cmd, uint8_t lenght);
        bool crc(char *res);
        bool getFwConf();               //call 1d 02 ca crc during connect
        bool getStatus();               //cyclic call 1d 02 cb crc every 1000ms
        bool setMode(Regime r);         //1d 03 ef aa/cc crc an then getFwConf()
        bool calibrateDome();           //1d 02 f6 crc
        bool abortDome();               //1d 02 f5 crc
        bool resetDome(ResetDir r);     //1d 03 f2 1a/1b crc
        bool moveShutter(ShutStatus d); //1d 04 f1 0x 0y crc
        bool moveDome(DomMotion d);     //1d 04 f0 0x 0y crc
        bool gotoDome(int32_t value);   //1d 07 f3 1a/1b 00 00 00 64 95
        void moveTimeout();
        void moveEndstop();
        static void moveHelper(void *p);

};
