#ifndef __LOGSERVICE__H__ 
#define __LOGSERVICE__H__ 
#include <WString.h>
#include <functional>
#include <HardwareSerial.h>
#include <memory>

using namespace std;
 
enum LogLevel {DEBUG, INFO, WARNING, ERROR, CRITICAL};

class NamedLog;

class ILogger { 
protected:
    String LogLevelToString(LogLevel logLevel);
    String getDateTimeInfo();

    String identMsg(String msg, int identationSize);
public: 
    virtual ~ILogger() = default;


    //must be override by implementations
    virtual void log(LogLevel level, String name, String msg, bool breakLine = true) = 0;

    //can be override by implementations, but it is not mandatory
    virtual void log(String name, String msg, bool breakLine = true);

    NamedLog getNLog(String name);
    shared_ptr<NamedLog> getNLogP(String name);
}; 

class NamedLog{
private:
    ILogger *logService;
    String name;
    bool initialized = false;
public:
    /*NamedLog(NamedLog& toCopy)
    {
        this->logService = toCopy.logService;
        this->name = toCopy.name;
    }*/

    NamedLog(){
        logService = nullptr;
    }

    NamedLog(const NamedLog &toCopy){
        logService = toCopy.logService;
        name = toCopy.name;
        initialized = toCopy.initialized;
    }

    NamedLog(String logName, ILogger *logServiceP)
    {
        this->logService = logServiceP;
        this->name = logName;
        initialized = (logServiceP != nullptr);
    }

    void debug(String message, bool breakLine = true)
    {
        if (!initialized)
            return; 
        logService->log(LogLevel::DEBUG, this->name, message, breakLine);
    }
    void info(String message, bool breakLine = true)
    {
        if (!initialized)
            return; 
        logService->log(LogLevel::INFO, this->name, message, breakLine);
    }
    void warning(String message, bool breakLine = true)
    {
        if (!initialized)
            return; 
        logService->log(LogLevel::WARNING, this->name, message, breakLine);
    }
    void error(String message, bool breakLine = true)
    {
        if (!initialized)
            return; 
        logService->log(LogLevel::ERROR, this->name, message, breakLine);
    }
    void critical(String message, bool breakLine = true)
    {
        if (!initialized)
            return; 
        logService->log(LogLevel::CRITICAL, this->name, message, breakLine);
    }
};

class DefaultLogger: public ILogger{
private:
    bool isANewLine = true;
    String newLineOwnerName = "";
    function<void(String)> logInterceptor = [](String nl){};

public:
    void log(LogLevel level, String name, String msg, bool breakLine = true) override;

};
 
#endif 
