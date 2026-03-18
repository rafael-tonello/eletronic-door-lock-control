#include "GenericIIOHalKeyboard.h"

GenericIIOHalKeyboard::GenericIIOHalKeyboard(
    IIOHal &iohal,
    Scheduler &scheduler,
    vector<IIOHAL_IO_ID_TYPE> keyIOs
):
    iohal(iohal),
    keyIOs(keyIOs)
{ 
    scheduler.periodicTask(15, [this](){
        default_work_step();
    });

}

vector<IIOHAL_IO_ID_TYPE> GenericIIOHalKeyboard::GetKeyIOS(){
    return keyIOs;
}

bool GenericIIOHalKeyboard::IsKeyPressed(IIOHAL_IO_ID_TYPE key){
    auto result = iohal.Read(key);
    if (result.err != Errors::NoError){
        Serial.print("Error reading key state: ");
        return false;
    }
    return result.Digital();
}
