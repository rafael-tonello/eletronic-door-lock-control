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
    class LinuxFontFeatures {
    public:
        static String Reset;

        static String cl_Red;
        static String cl_Green;
        static String cl_Yellow;
        static String cl_Blue;
        static String cl_Magenta;
        static String cl_Cyan;
        static String cl_White;
        static String cl_BrightRed;
        static String cl_BrightGreen;
        static String cl_BrightYellow;
        static String cl_BrightBlue;
        static String cl_BrightMagenta;
        static String cl_BrightCyan;
        static String cl_BrightWhite;

        static String bg_Red;
        static String bg_Green;
        static String bg_Yellow;
        static String bg_Blue;
        static String bg_Magenta;
        static String bg_Cyan;
        static String bg_White;
        static String bg_BrightRed;
        static String bg_BrightGreen;
        static String bg_BrightYellow;
        static String bg_BrightBlue;
        static String bg_BrightMagenta;
        static String bg_BrightCyan;
        static String bg_BrightWhite;

        static String st_Bold;
        static String st_Dim;
        static String st_Underlined;
        static String st_Blink;
        static String st_Inverted;
        static String st_Hidden;
        static String st_Strikethrough;
        static String st_Bright;
        static String st_DimBright;
        static String st_UnderlinedBright;
        static String st_BlinkBright;
        static String st_InvertedBright;
        static String st_HiddenBright;
        static String st_StrikethroughBright;
    };

    EventStream<ILoggerObservingItem> OnLog;
    virtual ~ILogger() = default;

    //can be override by implementations, but it is not mandatory. It must call OnLog.emit to do the actual logging
    virtual void log(LogLevel level, String name, String msg, bool breakLine = true);
    //can be override by implementations, but it is not mandatory. Is must call OnLog.emit to do the actual logging
    virtual void log(String name, String msg, bool breakLine = true);
    
    //a helper that generate a line header in the format [date time] [log level] [name]
    virtual String MountLineHeader(LogLevel level, String name, bool alloLinuxColors);
    
    //a helper that ident the message with the specified number of spaces (usefull to align multiline messages)
    String identMsg(String msg, unsigned int identationSize);

    NamedLog getNLog(String name);
    shared_ptr<NamedLog> getNLogP(String name);
};

#define FF ILogger::LinuxFontFeatures

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

    static shared_ptr<ILogger> __defaultInstance;
public:

    DefaultLogger(bool allowLinuxColors = true, bool setAsDefaultIntance = true);

    void logIt(LogLevel level, String name, String msg, bool breakLine = true) override;

    static shared_ptr<ILogger> DefaultInstance(bool allowLinuxColors = true);

};
 
#endif 
