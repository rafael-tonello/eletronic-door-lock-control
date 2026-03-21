#include "esp8266IOHal.h"

Error ERROR_InvalidIO = "Invalid IO";



tuple<bool, Error> ESP8266IOHal::InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {digitalRead(ioNumber), Errors::NoError};

}

tuple<int, Error> ESP8266IOHal::InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber)
{
    return {analogRead(ioNumber), Errors::NoError};
}

Error ESP8266IOHal::InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value)
{
    digitalWrite(ioNumber, value);
    return Errors::NoError;
}



Error ESP8266IOHal::InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value)
{
    analogWrite(ioNumber, value);
    return Errors::NoError;
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
