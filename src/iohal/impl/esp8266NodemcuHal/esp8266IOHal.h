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
    std::map<IIOHAL_IO_ID_TYPE, IHAL_PIN_INFO> availableIos;
    vector<IIOHAL_IO_ID_TYPE> availabelIosByIndex;

public:
    ESP8266IOHal() {
        availableIos[IOHAL_A0] = IHAL_PIN_INFO{IOHAL_A0, DPT_INPUT, PPM_UNKNOWN};
        availableIos[IOHAL_D0] = IHAL_PIN_INFO{IOHAL_D0, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D1] = IHAL_PIN_INFO{IOHAL_D1, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D2] = IHAL_PIN_INFO{IOHAL_D2, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D3] = IHAL_PIN_INFO{IOHAL_D3, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D4] = IHAL_PIN_INFO{IOHAL_D4, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D5] = IHAL_PIN_INFO{IOHAL_D5, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D6] = IHAL_PIN_INFO{IOHAL_D6, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D7] = IHAL_PIN_INFO{IOHAL_D7, DPT_DUPLEX, PPM_UNKNOWN};
        availableIos[IOHAL_D8] = IHAL_PIN_INFO{IOHAL_D8, DPT_DUPLEX, PPM_UNKNOWN};

        for (auto &io : availableIos){
            availabelIosByIndex.push_back(io.first);
        }
    }



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

    //returns the current real state and the configured desired state of an IO
    tuple<IHAL_PIN_INFO, Error> GetIOInfo(IIOHAL_IO_ID_TYPE ioNumber) override;
    vector<IHAL_PIN_INFO> GetAllAvailableIOSInfos() override;
    
    //set the physical pin mode, for example, if a pin is configured as duplex, this function can be used to change it between input and output
    Error SetPhysicalPinMode(IIOHAL_IO_ID_TYPE ioNumber, PHYSICAL_PIN_MODE mode) override;

    tuple<bool, Error> IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber) override;
    tuple<bool, Error> IsPwm(IIOHAL_IO_ID_TYPE ioNumber) override;
};


#endif