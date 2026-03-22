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

String ILogger::MountLineHeader(LogLevel level, String name)
{
    String header = "";
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
        header = MountLineHeader(level, name);
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