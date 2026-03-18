#ifndef GENERICIIOHALKEYBOARD_H 
#define GENERICIIOHALKEYBOARD_H 

#include <ikeyboard.h>
#include <scheduler.h>

class GenericIIOHalKeyboard: public IKeyboard{
private:
    IIOHal &iohal;
    vector<IIOHAL_IO_ID_TYPE> keyIOs;
public:
    GenericIIOHalKeyboard(
        IIOHal &iohal, 
        Scheduler &scheduler,
        vector<IIOHAL_IO_ID_TYPE> keyIOs
    );

/*IKeyboard interface */
    public:
        vector<IIOHAL_IO_ID_TYPE> GetKeyIOS() override;
    protected:
        bool IsKeyPressed(IIOHAL_IO_ID_TYPE key) override;
};
#endif
