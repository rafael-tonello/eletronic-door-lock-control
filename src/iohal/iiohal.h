#ifndef IIOHAL_H 
#define IIOHAL_H 

#include <Arduino.h>
#include <errors/errors.h>

using namespace std;

#define IIOHAL_MAX_ANALOGIC 1023
#define IIOHAL_IO_ID_TYPE int
#define IOHAL_INVALID_IO -1

//SHOULD BE override by implementations
//#define IOHAL_D0 0x0
//#define IOHAL_D1 0x0
//#define IOHAL_D... 0x0


//SHOULD BE override/created by implementations (just a model)
//#define IOHAL_A0 0x0
//#define IOHAL_A1 0x0
//#define IOHAL_A... 0x0
    
    

extern Error ERROR_InvalidIO;

//the configuration of the pins
enum DESIRED_PIN_MODE {
    DPT_INPUT, //pin can be only readed
    DPT_OUTPUT, //pin can be only writed
    DPT_DUPLEX, //Pin can be changed between input and output using SetRawPinMode
};

//the current real pin mode
enum PHYSICAL_PIN_MODE{
    PPM_UNKNOWN,
    PPM_INPUT,
    PPM_OUTPUT,
};

struct IHAL_PIN_INFO{
    IIOHAL_IO_ID_TYPE id;
    DESIRED_PIN_MODE desiredType;
    PHYSICAL_PIN_MODE currentPhysicalPinMode;
};

//class IClient;
//class IServer(){
//public:
//    virtual Error broadcast()
//    virtual vector<IClient&> GetConnectedClients();
//    virtual vector<function<void(IClient&)>> OnClientConnected;
//    virtual vector<function<void(IClient&)>> OnClientDisconnected;
//    virtual vector<function<void(IClient&, char*, size_t)>> Sniffer;
//    virtual void forEachClient(function<void<IClient&>> f);
//
//}
//
//class IClient(){
//public:
//    IClient(
//        function<Error(char*, size_t)> sendFunction
//        function<Error()> closeConnectionFunction
//    )
//
//    virtual Error Send(String data);
//    virtual Error Send(char* data, size_t size);
//
//    virtual vector<function<void(String)>> OnDataStr;
//    virtual vector<function<void(char*, size_t)>> OnDataStr;
//    virtual vector<function<void(IClient&)>> OnConnected;
//    virtual vector<function<void(IClient&)>> OnDisconnected;
//
//    virtual Error Close();
//}
//
//class IWifi {
//public:
//    virtual Error StartAp(String ssid, String password);
//    virtual Error StartClient(String ssid, String password);
//    
//    virtual tuple<IServer, Error> StartTcpServer(int port);
//    virtual tuple<IServer, Error> StartUdpServer(int port);
//
//    virtual tuple<IClient, Error> ConnectTCP(string host, int port);
//    virtual tuple<IClient, Error> ConnectUdp(string host, int port);
//
//}
//
//class IBluetooth{
//public:
//    virtual Promise<tuple<vector<tuple<string, string>>>, Error> discoverDevices(CT cancelationToken);
//    virtual Promise<Error> Connect(string address);
//
//    virtual void CreateCOM(function<void(IClient&)> onConnected, function<void()> disconnected);
//}

class IIOHal {
protected:
    virtual tuple<bool, Error> InternalDigitalRead(IIOHAL_IO_ID_TYPE ioNumber) = 0;
    virtual tuple<int, Error> InternalAnalogRead(IIOHAL_IO_ID_TYPE ioNumber) = 0;

    virtual Error InternalDigitalWrite(IIOHAL_IO_ID_TYPE ioNumber, bool value) = 0;
    virtual Error InternalAnalogWrite(IIOHAL_IO_ID_TYPE ioNumber, int value) = 0;

public: 
    //returns the current real state and the configured desired state of an IO
    virtual tuple<IHAL_PIN_INFO, Error> GetIOInfo(IIOHAL_IO_ID_TYPE ioNumber) = 0;
    virtual vector<IHAL_PIN_INFO> GetAllAvailableIOSInfos() = 0;
    
    //set the physical pin mode, for example, if a pin is configured as duplex, this function can be used to change it between input and output
    virtual Error SetPhysicalPinMode(IIOHAL_IO_ID_TYPE ioNumber, PHYSICAL_PIN_MODE mode) = 0;
    
    virtual tuple<bool, Error> IsAnalogic(IIOHAL_IO_ID_TYPE ioNumber) = 0;



    //return an analogic pin id (or IOHAL_INVALID_IO if unavailable)
    virtual IIOHAL_IO_ID_TYPE GetIdAn(size_t index) = 0;

    //return a digital pin id (or IOHAL_INVALID_IO if unavailable)
    virtual IIOHAL_IO_ID_TYPE GetIdDi(size_t index) = 0;

    //IWifi wifi()
    //IBlurtooth bluetooth()
public:

    // #region optionals {
        struct READ_WRITE_OPTIONS{
            bool autoChangeIOMOde;
            int maxValueForAnalogic;
            int value;
        };

        auto WithAutoChangeIOMOde(bool autoChange){
            return [=](READ_WRITE_OPTIONS* options){
                options->autoChangeIOMOde = autoChange;
            };
        }

        //is used in convertions between digital and analogic values, it is used to determine the threshold between high and low digital values, and also the maximum value for analogic values
        auto WithMaxAnalogicValue(int maxValue){
            return [=](READ_WRITE_OPTIONS *options){
                options->maxValueForAnalogic = maxValue;
            };
        }
    // #endregion }

    

    // #region read mechanism {
        class ReadResult{
        public:
            int value;
            Error err;
            int _maxValueForAnalogic = IIOHAL_MAX_ANALOGIC;

            ReadResult(int value, Error err, int maxValueForAnalogic): value(value), err(err), _maxValueForAnalogic(maxValueForAnalogic){}

            bool Digital()
            {
                return value >= _maxValueForAnalogic / 2;
            }

            int Analogic()
            {
                return value;
            }

            //operators for easy use of ReadResult
            operator bool(){
                return Digital();
            }

            operator int(){
                return Analogic();
            }
        };

        ReadResult Read(IIOHAL_IO_ID_TYPE ioNumber, vector<function<void(READ_WRITE_OPTIONS*)>> options = {}){
            READ_WRITE_OPTIONS defaultOptions = {true, IIOHAL_MAX_ANALOGIC, 0};

            for (auto opt : options){
                opt(&defaultOptions);
            }

            //check if io is setted to be an output and autoChangeIOMode is true, if so change it to input
            if (defaultOptions.autoChangeIOMOde){
                //read current mode of io
                auto [ioInfo, err] = GetIOInfo(ioNumber);
                if (err != Errors::NoError){
                    return ReadResult(0, err, defaultOptions.maxValueForAnalogic);
                }

                if (ioInfo.desiredType == DPT_OUTPUT){
                    return ReadResult(0, Errors::createError("IO is configured as output, cannot read"), defaultOptions.maxValueForAnalogic);
                }

                if (ioInfo.currentPhysicalPinMode != PPM_INPUT){
                    SetPhysicalPinMode(ioNumber, PPM_INPUT);
                }
            }

            //read value from io
            int analogValue = 0;
            Error err = Errors::NoError;

            //if io is an analogic input, read it as an analogic input
            auto [isAnalogic, err2] = IsAnalogic(ioNumber);
            if (err2 != Errors::NoError){
                return ReadResult(0, err2, defaultOptions.maxValueForAnalogic);
            }

            if (isAnalogic){
                auto [analogValueTmp, err] = InternalAnalogRead(ioNumber);
                if (err != Errors::NoError){
                    return ReadResult(0, err, defaultOptions.maxValueForAnalogic);
                }

                analogValue = analogValueTmp;
            }
            else{
                auto [digitalValue, err2] = InternalDigitalRead(ioNumber);
                if (err2 != Errors::NoError){
                    return ReadResult(0, err2, defaultOptions.maxValueForAnalogic);
                }

                analogValue = digitalValue ? defaultOptions.maxValueForAnalogic : 0;
            }

            return ReadResult(analogValue, Errors::NoError, defaultOptions.maxValueForAnalogic);
        }
    // #endregio }

    /* #region write mechanism { */
        class WriteValue{
        public:
            int value;

            WriteValue(bool value){
                if (value){
                    this->value = IIOHAL_MAX_ANALOGIC;
                }
                else{
                    this->value = 0;
                }
            }

            WriteValue(int value){
                this->value = value;
            }
        
        }; 
        #define WV WriteValue
        virtual Error Write(IIOHAL_IO_ID_TYPE ioNumber, WV value, vector<function<void(READ_WRITE_OPTIONS*)>> options = {})
        {
            READ_WRITE_OPTIONS defaultOptions = {true, IIOHAL_MAX_ANALOGIC, 0};

            for (auto opt : options){
                opt(&defaultOptions);
            }

            //check if io is setted to be an input and autoChangeIOMode is true, if so change it to output
            if (defaultOptions.autoChangeIOMOde){
                //read current mode of io
                auto [ioInfo, err] = GetIOInfo(ioNumber);
                if (err != Errors::NoError){
                    return err;
                }

                if (ioInfo.desiredType == DPT_INPUT){
                    return Errors::createError("IO is configured as input, cannot write");
                }

                if (ioInfo.currentPhysicalPinMode != PPM_OUTPUT){
                    SetPhysicalPinMode(ioNumber, PPM_OUTPUT);
                }
            }

            //write value to io
            Error err = Errors::NoError;

            //if io is an analogic output, write it as an analogic output
            auto [isAnalogic, err2] = IsAnalogic(ioNumber);
            if (err2 != Errors::NoError){
                return err2;
            }

            if (isAnalogic){

                err = InternalAnalogWrite(ioNumber, value.value);
                if (err != Errors::NoError){
                    return err;
                }
            }
            else{
                bool digitalValue = value.value >= defaultOptions.maxValueForAnalogic / 2;
                err = InternalDigitalWrite(ioNumber, digitalValue);
                if (err != Errors::NoError){
                    return err;
                }
            }

            return Errors::NoError;
        }
    /* #endregio } */

    //int readValue = io->read(DIO1);
    //io->write(DAN01, 512);

};

#endif