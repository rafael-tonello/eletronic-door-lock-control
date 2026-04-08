#include  "ilogger.h" 

String FF::Reset = "\033[0m";

String FF::cl_Red = "\033[31m";
String FF::cl_Green = "\033[32m";
String FF::cl_Yellow = "\033[33m";
String FF::cl_Blue = "\033[34m";
String FF::cl_Magenta = "\033[35m";
String FF::cl_Cyan = "\033[36m";
String FF::cl_White = "\033[37m";
String FF::cl_BrightRed = "\033[91m";
String FF::cl_BrightGreen = "\033[92m";
String FF::cl_BrightYellow = "\033[93m";
String FF::cl_BrightBlue = "\033[94m";
String FF::cl_BrightMagenta = "\033[95m";
String FF::cl_BrightCyan = "\033[96m";
String FF::cl_BrightWhite = "\033[97m";

String FF::bg_Red = "\033[41m";
String FF::bg_Green = "\033[42m";
String FF::bg_Yellow = "\033[43m";
String FF::bg_Blue = "\033[44m";
String FF::bg_Magenta = "\033[45m";
String FF::bg_Cyan = "\033[46m";
String FF::bg_White = "\033[47m";
String FF::bg_BrightRed = "\033[101m";
String FF::bg_BrightGreen = "\033[102m";
String FF::bg_BrightYellow = "\033[103m";
String FF::bg_BrightBlue = "\033[104m";
String FF::bg_BrightMagenta = "\033[105m";
String FF::bg_BrightCyan = "\033[106m";
String FF::bg_BrightWhite = "\033[107m";

String FF::st_Bold = "\033[1m";
String FF::st_Dim = "\033[2m";
String FF::st_Underlined = "\033[4m";
String FF::st_Blink = "\033[5m";
String FF::st_Inverted = "\033[7m";
String FF::st_Hidden = "\033[8m";
String FF::st_Strikethrough = "\033[9m";
String FF::st_Bright = "\033[1m";
String FF::st_DimBright = "\033[2m";
String FF::st_UnderlinedBright = "\033[4m";
String FF::st_BlinkBright = "\033[5m";
String FF::st_InvertedBright = "\033[7m";
String FF::st_HiddenBright = "\033[8m";

shared_ptr<ILogger> DefaultLogger::__defaultInstance = nullptr;

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

String ILogger::identMsg(String msg, unsigned int identationSize)
{
    //pre allocate identation chars
    String identationChars = "                                                   ";
    //identationPrefix.reserve(identationSize);


    while (identationChars.length() < identationSize)
        identationChars += "    ";

    identationChars = identationChars.substring(0, identationSize);

    msg.replace("\n", "\n"+identationChars);

    return msg;
}

String ILogger::MountLineHeader(LogLevel level, String name, bool alloLinuxColors)
{
    String header = "";
    if (alloLinuxColors)
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
        header += "["+dateTime+"] "+ header;

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

    if (allowLinuxColors)
        msg += FF::Reset;

    Serial.print(msg.c_str());
    //msg.clear();

    if (breakLine == true)
        Serial.println("");

    if (!breakLine)
        newLineOwnerName = name;

    isANewLine = breakLine;

    
}

DefaultLogger::DefaultLogger(
    bool allowLinuxColors,
    bool setAsDefaultIntance
): allowLinuxColors(allowLinuxColors) {
    if (setAsDefaultIntance)
        DefaultLogger::__defaultInstance = shared_ptr<ILogger>(this);
}

shared_ptr<ILogger> DefaultLogger::DefaultInstance(bool allowLinuxColors)
{
    if (__defaultInstance == nullptr)
        __defaultInstance = make_shared<DefaultLogger>(allowLinuxColors);

    return __defaultInstance;
}

