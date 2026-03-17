#ifndef ESP8266IOHAL_H 
#define ESP8266IOHAL_H 

#include <iiohal.h>
#include <map>

using namespace std;

//map between IO definitions and actual pin numbers of the ESP8266 (arduino framework)
const IIOHAL_IO_ID_TYPE IOHAL_A0 = A0;

const IIOHAL_IO_ID_TYPE IOHAL_D0  = D0;
const IIOHAL_IO_ID_TYPE IOHAL_D1  = D1;
const IIOHAL_IO_ID_TYPE IOHAL_D2  = D2;
const IIOHAL_IO_ID_TYPE IOHAL_D3  = D3;
const IIOHAL_IO_ID_TYPE IOHAL_D4  = D4;
const IIOHAL_IO_ID_TYPE IOHAL_D5  = D5;
const IIOHAL_IO_ID_TYPE IOHAL_D6  = D6;
const IIOHAL_IO_ID_TYPE IOHAL_D7  = D7;
const IIOHAL_IO_ID_TYPE IOHAL_D8  = D8;

    

class ESP8266IOHal: public IIOHal{
private:
    //pin and its modes
    std::map<IIOHAL_IO_ID_TYPE, IOMODE> availableIos;
    vector<IIOHAL_IO_ID_TYPE> availabelIosByIndex;

public:
    ESP8266IOHal() {
        availableIos[IOHAL_A0] = IOM_INPUT;
        availableIos[IOHAL_D0] = IOM_DUPLEX;
        availableIos[IOHAL_D1] = IOM_DUPLEX;
        availableIos[IOHAL_D2] = IOM_DUPLEX;
        availableIos[IOHAL_D3] = IOM_DUPLEX;
        availableIos[IOHAL_D4] = IOM_DUPLEX;
        availableIos[IOHAL_D5] = IOM_DUPLEX;
        availableIos[IOHAL_D6] = IOM_DUPLEX;
        availableIos[IOHAL_D7] = IOM_DUPLEX;
        availableIos[IOHAL_D8] = IOM_DUPLEX;

        for (auto &io : availableIos){
            availabelIosByIndex.push_back(io.first);
        }
    }



/* IIOHal interface */
protected:

    IIOHAL_IO_ID_TYPE GetIdAn(size_t index) override;
    IIOHAL_IO_ID_TYPE GetIdDi(size_t index) override;

    tuple<bool, Error> InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber) override;
    tuple<int, Error> InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber) override;

    Error InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value) override;
    Error InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value) override;

    tuple<IOMODE, Error> GetIOMode(IIOHAL_IO_ID_TYPE ioNumber) override;
    tuple<bool, Error> IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber) override;
    Error SetRawPinMode(IIOHAL_IO_ID_TYPE ioNumber, RawPinMode mode) override;

    vector<tuple<IIOHAL_IO_ID_TYPE, IOMODE>> GetAvailableIOS() override;
};


#endif