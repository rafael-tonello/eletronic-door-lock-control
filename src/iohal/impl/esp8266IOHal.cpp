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

tuple<IOMODE, Error> ESP8266IOHal::GetIOMode(IIOHAL_IO_ID_TYPE ioNumber)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return {IOM_INPUT, Errors::DerivateError(ERROR_InvalidIO, "IO number not found")};
    }

    return {availableIos[ioNumber], Errors::NoError};
}

tuple<bool, Error> ESP8266IOHal::IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return {false, Errors::DerivateError(ERROR_InvalidIO, "IO number not found")};
    }

    return {availableIos[ioNumber] == IOM_INPUT && ioNumber == IOHAL_A0, Errors::NoError};

}

Error ESP8266IOHal::SetRawPinMode(IIOHAL_IO_ID_TYPE ioNumber, RawPinMode mode)
{
    if (availableIos.find(ioNumber) == availableIos.end()){
        return Errors::DerivateError(ERROR_InvalidIO, "IO number not found");
    }

    //set pin mode using arduino framework
    switch (mode){
        case RPM_INPUT:
            pinMode(ioNumber, INPUT);
            break;
        case RPM_OUTPUT:
            pinMode(ioNumber, OUTPUT);
            break;
        default:
            return Errors::createError("Invalid RawPinMode");
    }

    return Errors::NoError;

}

vector<tuple<IIOHAL_IO_ID_TYPE, IOMODE>> ESP8266IOHal::GetAvailableIOS()
{
    auto result = vector<tuple<IIOHAL_IO_ID_TYPE, IOMODE>>();
    for (auto ioMode : availableIos){
        result.push_back({ioMode.first, ioMode.second});
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
