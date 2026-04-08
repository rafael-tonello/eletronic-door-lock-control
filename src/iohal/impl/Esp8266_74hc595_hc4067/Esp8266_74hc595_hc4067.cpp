#include "Esp8266_74hc595_hc4067.h"

extern Error ERROR_InvalidIO;

ESP8266_74hc595_hc4067::ESP8266_74hc595_hc4067() 
    : muxSIG(A0), muxS0(D6), muxS1(D5), muxS2(D4), muxS3(D3)
{
    // Initialize 74HC595 digital outputs (D0-D7)
    availableIos[IOHAL_D0] = IHAL_PIN_INFO{IOHAL_D0, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D1] = IHAL_PIN_INFO{IOHAL_D1, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D2] = IHAL_PIN_INFO{IOHAL_D2, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D3] = IHAL_PIN_INFO{IOHAL_D3, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D4] = IHAL_PIN_INFO{IOHAL_D4, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D5] = IHAL_PIN_INFO{IOHAL_D5, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D6] = IHAL_PIN_INFO{IOHAL_D6, DPM_OUTPUT, PPM_UNKNOWN};
    availableIos[IOHAL_D7] = IHAL_PIN_INFO{IOHAL_D7, DPM_OUTPUT, PPM_UNKNOWN};

    // Initialize multiplexed analog inputs (16 channels)
    availableIos[IOHAL_A0] = IHAL_PIN_INFO{IOHAL_A0, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A1] = IHAL_PIN_INFO{IOHAL_A1, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A2] = IHAL_PIN_INFO{IOHAL_A2, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A3] = IHAL_PIN_INFO{IOHAL_A3, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A4] = IHAL_PIN_INFO{IOHAL_A4, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A5] = IHAL_PIN_INFO{IOHAL_A5, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A6] = IHAL_PIN_INFO{IOHAL_A6, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A7] = IHAL_PIN_INFO{IOHAL_A7, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A8] = IHAL_PIN_INFO{IOHAL_A8, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A9] = IHAL_PIN_INFO{IOHAL_A9, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A10] = IHAL_PIN_INFO{IOHAL_A10, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A11] = IHAL_PIN_INFO{IOHAL_A11, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A12] = IHAL_PIN_INFO{IOHAL_A12, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A13] = IHAL_PIN_INFO{IOHAL_A13, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A14] = IHAL_PIN_INFO{IOHAL_A14, DPM_DUPLEX, PPM_UNKNOWN};
    availableIos[IOHAL_A15] = IHAL_PIN_INFO{IOHAL_A15, DPM_DUPLEX, PPM_UNKNOWN};

    // Build the index list
    for (auto &io : availableIos){
        availabelIosByIndex.push_back(io.first);
    }

    // Initialize multiplexer control pins
    pinMode(muxS0, OUTPUT);
    pinMode(muxS1, OUTPUT);
    pinMode(muxS2, OUTPUT);
    pinMode(muxS3, OUTPUT);

    pinMode(hc595_dataPin, OUTPUT);
    pinMode(hc595_clockPin, OUTPUT);
    pinMode(hc595_latchPin, OUTPUT);

    //put all digital (74HC595) outputs to low
    //the entire vector is initialized to false, so, only one write is needed to set all outputs to low
    this->InternalDigitalWrite(IOHAL_D0, false);
}

tuple<bool, Error> ESP8266_74hc595_hc4067::InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {false, Errors::DerivateError(ERROR_InvalidIO, "Cannot read as digital from this IO ("+String(ioNumber)+")")};
}

tuple<int, Error> ESP8266_74hc595_hc4067::InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue)
{
    //set A0 pin as input
    if (A0_mode != PPM_INPUT){
        pinMode(muxSIG, INPUT);
        A0_mode = PPM_INPUT;
    }
    
    int channel = ioNumber - IOHAL_A0;
    SetReadMuxChannel(channel);
    delay(1);  // Small delay for MUX to settle
    int rawValue = analogRead(muxSIG);
    
    if (maxValue <= 0) {
        return {0, Errors::createError("maxValue must be greater than zero")};
    }

    // Scale raw value (0-1023) to requested maxValue
    int scaledValue = (int)(((long)rawValue * (long)maxValue) / 1023L);
    return {scaledValue, Errors::NoError};
}

tuple<int, Error> ESP8266_74hc595_hc4067::InternalPwmRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue)
{
    return {0, Errors::DerivateError(ERROR_InvalidIO, "Cannot read as pwm from this IO ("+String(ioNumber)+")")};
}

Error ESP8266_74hc595_hc4067::InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value)
{
    digitalOuts[ioNumber] = value;

    uint8_t dataByte = 0;
    for (int i = 0; i < 8; i++) {
        if (digitalOuts[i]) {
            dataByte |= (1 << (7 - i));
        }
    }

    digitalWrite(hc595_latchPin, LOW); 
    shiftOut(hc595_dataPin, hc595_clockPin, LSBFIRST, dataByte);
    digitalWrite(hc595_latchPin, HIGH);

    return Errors::NoError;
}

Error ESP8266_74hc595_hc4067::InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue)
{
    //set A0 pin as output
    if (A0_mode != PPM_OUTPUT){
        pinMode(muxSIG, OUTPUT);
        A0_mode = PPM_OUTPUT;
    }

    int channel = ioNumber - IOHAL_A0;
    SetReadMuxChannel(channel);
    
    int scaledOutValue = (int)(((long)value * 1023L) / (long)maxValue);
    delay(1);  // Small delay for MUX to settle
    analogWrite(muxSIG, scaledOutValue);

    return Errors::NoError;
}

Error ESP8266_74hc595_hc4067::InternalPwmWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue)
{
    return Errors::DerivateError(ERROR_InvalidIO, "Cannot write as pwm to this IO");
}

tuple<IHAL_PIN_INFO, Error> ESP8266_74hc595_hc4067::GetIOInfo(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {availableIos[ioNumber], Errors::NoError};
}

tuple<bool, Error> ESP8266_74hc595_hc4067::IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {ioNumber >= IOHAL_A0 && ioNumber <= IOHAL_A15, Errors::NoError};
}

tuple<bool, Error> ESP8266_74hc595_hc4067::IsPwm(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {false, Errors::NoError};
}

Error ESP8266_74hc595_hc4067::SetPhysicalPinMode(IIOHAL_IO_ID_TYPE ioNumber, PHYSICAL_PIN_MODE mode)
{
    //pins are automatically switched between input and output based on the read/write operations, so this function doesn't need to do anything, but we can use it to track the current physical mode of pins if needed
    return Errors::NoError;
}

vector<IHAL_PIN_INFO> ESP8266_74hc595_hc4067::GetAllAvailableIOSInfos()
{
    auto result = vector<IHAL_PIN_INFO>();
    for (auto ioMode : availableIos){
        result.push_back(ioMode.second);
    }

    return result;
}

IIOHAL_IO_ID_TYPE ESP8266_74hc595_hc4067::GetIdAn(size_t index){
    // Return analog inputs in order: A0, then multiplexed inputs 0-15
    if (index >= 0 && index <= 15) {
        return IOHAL_A0 + index;
    }

    return IOHAL_INVALID_IO;
}

IIOHAL_IO_ID_TYPE ESP8266_74hc595_hc4067::GetIdDi(size_t index){
    // Return digital IOs in order: D0-D7
    if (index >= 0 && index <= 7)
        return IOHAL_D0 + index;

    return IOHAL_INVALID_IO;
}

IIOHAL_IO_ID_TYPE ESP8266_74hc595_hc4067::GetIdPwm(size_t index){
    return IOHAL_INVALID_IO;
}

void ESP8266_74hc595_hc4067::SetReadMuxChannel(uint8_t channel)
{
    // Set the 4-bit channel selector
    digitalWrite(muxS0, bitRead(channel, 0));
    digitalWrite(muxS1, bitRead(channel, 1));
    digitalWrite(muxS2, bitRead(channel, 2));
    digitalWrite(muxS3, bitRead(channel, 3));
}
