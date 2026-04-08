#ifndef ESP8266LIGHTCONTROLHAL_H 
#define ESP8266LIGHTCONTROLHAL_H 

#include <iiohal.h>
#include <map>

using namespace std;

//ios provided by the 74HC595 shift register (8 digital outputs, IDs 0-7)
const IIOHAL_IO_ID_TYPE IOHAL_D0  = 0;
const IIOHAL_IO_ID_TYPE IOHAL_D1  = 1;
const IIOHAL_IO_ID_TYPE IOHAL_D2  = 2;
const IIOHAL_IO_ID_TYPE IOHAL_D3  = 3;
const IIOHAL_IO_ID_TYPE IOHAL_D4  = 4;
const IIOHAL_IO_ID_TYPE IOHAL_D5  = 5;
const IIOHAL_IO_ID_TYPE IOHAL_D6  = 6;
const IIOHAL_IO_ID_TYPE IOHAL_D7  = 7;

// Multiplexed analog inputs (16 channels, IDs 200-215)
const IIOHAL_IO_ID_TYPE IOHAL_A0  = 200;
const IIOHAL_IO_ID_TYPE IOHAL_A1  = 201;
const IIOHAL_IO_ID_TYPE IOHAL_A2  = 202;
const IIOHAL_IO_ID_TYPE IOHAL_A3  = 203;
const IIOHAL_IO_ID_TYPE IOHAL_A4  = 204;
const IIOHAL_IO_ID_TYPE IOHAL_A5  = 205;
const IIOHAL_IO_ID_TYPE IOHAL_A6  = 206;
const IIOHAL_IO_ID_TYPE IOHAL_A7  = 207;
const IIOHAL_IO_ID_TYPE IOHAL_A8  = 208;
const IIOHAL_IO_ID_TYPE IOHAL_A9  = 209;
const IIOHAL_IO_ID_TYPE IOHAL_A10 = 210;
const IIOHAL_IO_ID_TYPE IOHAL_A11 = 211;
const IIOHAL_IO_ID_TYPE IOHAL_A12 = 212;
const IIOHAL_IO_ID_TYPE IOHAL_A13 = 213;
const IIOHAL_IO_ID_TYPE IOHAL_A14 = 214;
const IIOHAL_IO_ID_TYPE IOHAL_A15 = 215;

class ESP8266_74hc595_hc4067: public IIOHal{
private:
    // pin and its modes for regular ESP8266 pins
    std::map<IIOHAL_IO_ID_TYPE, IHAL_PIN_INFO> availableIos;
    vector<IIOHAL_IO_ID_TYPE> availabelIosByIndex;
    
    // Adafruit PWM driver for extended outputs
    
    // Multiplexer control pins for analog inputs
    const int muxSIG;   // Signal input (A0)
    const int muxS0;    // Channel select bit 0 (D3)
    const int muxS1;    // Channel select bit 1 (D4)
    const int muxS2;    // Channel select bit 2 (D5)
    const int muxS3;    // Channel select bit 3 (D6)

    const int hc595_dataPin = D0;  // Data pin for 74HC595
    const int hc595_latchPin = D1; // Latch pin for 74HC595
    const int hc595_clockPin = D2; // Clock pin for 74HC595

    bool digitalOuts[8] = {false, false, false, false, false, false, false, false}; // Track the state of the 8 digital outputs
    
    // Helper methods
    void SetReadMuxChannel(uint8_t channel);
    
    // Track PWM output values
    std::map<int, int> pwmOutputValues;  // channel -> PWM value (0-4095)

    PHYSICAL_PIN_MODE A0_mode = PHYSICAL_PIN_MODE::PPM_UNKNOWN; // Track the current physical mode of A0 (input or output)

public:
    ESP8266_74hc595_hc4067();

/* IIOHal interface */
protected:

    IIOHAL_IO_ID_TYPE GetIdAn(size_t index) override;
    IIOHAL_IO_ID_TYPE GetIdDi(size_t index) override;
    IIOHAL_IO_ID_TYPE GetIdPwm(size_t index) override;

    tuple<bool, Error> InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber) override;
    tuple<int, Error> InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue) override;
    tuple<int, Error> InternalPwmRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue) override;

    Error InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value) override;
    Error InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue) override;
    Error InternalPwmWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue) override;

    // returns the current real state and the configured desired state of an IO
    tuple<IHAL_PIN_INFO, Error> GetIOInfo(IIOHAL_IO_ID_TYPE ioNumber) override;
    vector<IHAL_PIN_INFO> GetAllAvailableIOSInfos() override;
    
    // set the physical pin mode, for example, if a pin is configured as duplex, this function can be used to change it between input and output
    Error SetPhysicalPinMode(IIOHAL_IO_ID_TYPE ioNumber, PHYSICAL_PIN_MODE mode) override;

    tuple<bool, Error> IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber) override;
    tuple<bool, Error> IsPwm(IIOHAL_IO_ID_TYPE ioNumber) override;
};


#endif
