#include  "ilogger.h" 

void ILogger::log(LogLevel level, String name, String msg, bool breakLine)
{
    this->logIt(level, name, msg, breakLine);
    OnLog.emit(ILoggerObservingItem{
        .name = name,
        .level = level,
        .msg = msg,
        .breakLine = breakLine
    });
}

void ILogger::log(String name, String msg, bool breakLine)
{
    this->log(LogLevel::INFO, name, msg, breakLine);
}

NamedLog ILogger::getNLog(String name)
{
    return NamedLog(name, this);
}

shared_ptr<NamedLog> ILogger::getNLogP(String name)
{
    shared_ptr<NamedLog> ret = make_shared<NamedLog>(name, this);
    return ret;
}

String ILogger::LogLevelToString(LogLevel logLevel)
{
    switch (logLevel){
        case DEBUG:
            return "DEBUG";
        break;
        case WARNING:
            return "WARNING";
        break;
        case ERROR:
            return "ERROR";
        break;
        case CRITICAL:
            return "CRITICAL";
        break;
        default:
            return "INFO";
        break;
    };

    return "!!!INVALID!!!";
}

String ILogger::getDateTimeInfo()
{
    return String(millis()) + " ms";
    //return String("");
}

String ILogger::identMsg(String msg, int identationSize)
{
    String identationChars ="                                                                                                    ";
    //identationPrefix.reserve(identationSize);


    for (int c = identationChars.length(); c < identationSize; c++)
        identationChars+=' ';


    identationChars = identationChars.substring(0, identationSize);
    msg.replace("\n", "\n"+identationChars);

    return msg;
}

String ILogger::MountLineHeader(LogLevel level, String name, bool alloLinuxColors)
{
    String header = "";
    if (LOGGER_ENABLE_LINUX_COLORS)
    {
        //close previous color (if any)
        header += "\033[0m";

        switch (level){
            case LogLevel::DEBUG:
                //grayed out
                header += "\033[90m"; // Bright Black (Gray)
            break;
            break;
            case LogLevel::WARNING:
                header += "\033[33m"; // Yellow
            break;
            case LogLevel::ERROR:
                header += "\033[31m"; // Red
            break;
            case LogLevel::CRITICAL:
                //white with red background
                header += "\033[41m\033[97m"; // Red background + Bright White text
                //header += "\033[1m\033[31m"; // Bold Red
            default:
                //default terminal color
            break;
        };
    }

    String dateTime = getDateTimeInfo();
    if (dateTime != "")
        header = "["+dateTime+"] "+ header;

    header += "["+LogLevelToString(level)+"] ";
    header += "["+name+"] ";

    return header;
}

void DefaultLogger::logIt(LogLevel level, String name, String msg, bool breakLine)
{
    if (!isANewLine && newLineOwnerName != name)
    {
        Serial.println("");
        isANewLine = false;
    }

    String header = "";
    int headerSize = 0;
    if (isANewLine == true)
    {
        header = MountLineHeader(level, name, allowLinuxColors);
        lastHeader = header;
    }

    headerSize = lastHeader.length();


    msg = identMsg(msg, headerSize);

    msg = header + msg;

    Serial.print(msg.c_str());
    //msg.clear();

    if (breakLine == true)
        Serial.println("");

    if (!breakLine)
        newLineOwnerName = name;

    isANewLine = breakLine;
}