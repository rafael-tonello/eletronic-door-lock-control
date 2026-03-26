#include "esp8266IOHal.h"

Error ERROR_InvalidIO = "Invalid IO";



tuple<bool, Error> ESP8266IOHal::InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {digitalRead(ioNumber), Errors::NoError};

}

tuple<int, Error> ESP8266IOHal::InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue)
{
    int rawValue = analogRead(ioNumber);
    
    // Scale the raw value (0-1023) to the requested maxValue
    if (maxValue <= 0) {
        return {0, Errors::createError("maxValue must be greater than zero")};
    }
    
    int scaledValue = (int)(((long)rawValue * (long)maxValue) / 1023L);
    return {scaledValue, Errors::NoError};
}

tuple<int, Error> ESP8266IOHal::InternalPwmRead(IIOHAL_IO_ID_TYPE ioNumber, int maxValue)
{
    return {0, Errors::DerivateError(ERROR_InvalidIO, "PWM is not supported by this IOHAL implementation")};
}

Error ESP8266IOHal::InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value)
{
    digitalWrite(ioNumber, value);
    return Errors::NoError;
}



Error ESP8266IOHal::InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue)
{
    // Scale the value from maxValue range to 0-1023 range for analogWrite (ESP8266 uses 0-1023, not 0-255)
    if (maxValue <= 0) {
        return Errors::createError("maxValue must be greater than zero");
    }
    
    int scaledValue = (int)(((long)value * 1023L) / (long)maxValue);
    if (scaledValue < 0) scaledValue = 0;
    if (scaledValue > 1023) scaledValue = 1023;
    
    analogWrite(ioNumber, scaledValue);
    return Errors::NoError;
}

Error ESP8266IOHal::InternalPwmWrite(IIOHAL_IO_ID_TYPE ioNumber, int value, int maxValue)
{
    return Errors::DerivateError(ERROR_InvalidIO, "PWM is not supported by this IOHAL implementation");
}

tuple<IHAL_PIN_INFO, Error> ESP8266IOHal::GetIOInfo(IIOHAL_IO_ID_TYPE ioNumber)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return {IHAL_PIN_INFO{ioNumber, DPT_INPUT, PPM_INPUT}, Errors::DerivateError(ERROR_InvalidIO, "IO number not found")};
    }

    return {availableIos[ioNumber], Errors::NoError};
}

tuple<bool, Error> ESP8266IOHal::IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return {false, Errors::DerivateError(ERROR_InvalidIO, "IO number not found")};
    }

    return {ioNumber == IOHAL_A0, Errors::NoError};
}

tuple<bool, Error> ESP8266IOHal::IsPwm(IIOHAL_IO_ID_TYPE ioNumber)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return {false, Errors::DerivateError(ERROR_InvalidIO, "IO number not found")};
    }

    return {false, Errors::NoError};
}

Error ESP8266IOHal::SetPhysicalPinMode(IIOHAL_IO_ID_TYPE ioNumber, PHYSICAL_PIN_MODE mode)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return Errors::DerivateError(ERROR_InvalidIO, "IO number not found");
    }

    //set pin mode using arduino framework
    switch (mode){
        case PPM_INPUT:
            pinMode(ioNumber, INPUT);
            break;
        case PPM_OUTPUT:
            pinMode(ioNumber, OUTPUT);
            break;
        default:
            return Errors::createError("Invalid PHYSICAL_PIN_MODE");
    }

    //update current physical pin mode
    availableIos[ioNumber].currentPhysicalPinMode = mode;

    return Errors::NoError;

}

vector<IHAL_PIN_INFO> ESP8266IOHal::GetAllAvailableIOSInfos()
{
    auto result = vector<IHAL_PIN_INFO>();
    for (auto ioMode : availableIos){
        result.push_back(ioMode.second);
    }

    return result;
}


IIOHAL_IO_ID_TYPE ESP8266IOHal::GetIdAn(size_t index){
    if (index == 0)
        return IOHAL_A0;

    return IOHAL_INVALID_IO;
}

IIOHAL_IO_ID_TYPE ESP8266IOHal::GetIdDi(size_t index){
    if (index >= 0 && index < availableIos.size()-1)
        return availabelIosByIndex[index+1];

    return IOHAL_INVALID_IO;
}

IIOHAL_IO_ID_TYPE ESP8266IOHal::GetIdPwm(size_t index){
    return IOHAL_INVALID_IO;
}
