#ifndef __LOGSERVICE__H__ 
#define __LOGSERVICE__H__ 
#include <WString.h>
#include <functional>
#include <HardwareSerial.h>
#include <memory>
#include <eventstream.h>

using namespace std;
 
enum LogLevel {DEBUG, INFO, WARNING, ERROR, CRITICAL};

class NamedLog;

#define LOGGER_ENABLE_LINUX_COLORS true

struct ILoggerObservingItem{
    String name;
    LogLevel level;
    String msg;
    bool breakLine;
};

class ILogger { 
protected:
    String LogLevelToString(LogLevel logLevel);
    String getDateTimeInfo();

    
    bool isANewLine = true;
    String newLineOwnerName = "";
    
    //must be override by implementations
    virtual void logIt(LogLevel level, String name, String msg, bool breakLine = true) = 0;
    

    
public: 
    EventStream<ILoggerObservingItem> OnLog;
    virtual ~ILogger() = default;

    //can be override by implementations, but it is not mandatory. It must call OnLog.emit to do the actual logging
    virtual void log(LogLevel level, String name, String msg, bool breakLine = true);
    //can be override by implementations, but it is not mandatory. Is must call OnLog.emit to do the actual logging
    virtual void log(String name, String msg, bool breakLine = true);
    
    //a helper that generate a line header in the format [date time] [log level] [name]
    virtual String MountLineHeader(LogLevel level, String name, bool alloLinuxColors);
    
    //a helper that ident the message with the specified number of spaces (usefull to align multiline messages)
    String identMsg(String msg, int identationSize);

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
    String lastHeader = "";
    bool allowLinuxColors = true;

public:
    DefaultLogger(bool allowLinuxColors = true): allowLinuxColors(allowLinuxColors) {}
    void logIt(LogLevel level, String name, String msg, bool breakLine = true) override;

};
 
#endif 
