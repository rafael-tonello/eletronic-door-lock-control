#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include <Arduino.h>
#include <ArduinoOTA.h>

#include <vector>

#include <errors/errors.h>
#include <scheduler.h>

#include <iiohal.h>
#include <iohal/impl/Esp8266_74hc595_hc4067/Esp8266_74hc595_hc4067.h>

#include <ikeyboard.h>
#include <keyboard/impl/GenericIIOHalKeyboard.h>

#include <ilogger.h>

#include <istorage.h>
#include <storage/impl/preferenceslibrary/preferenceslibrarystorage.h>

#include <inetwork.h>
#include <network/impl/wfservice.h>

#include <telnet/telnetserver.h>

#include <vssClient/vstpclient.h>

#include <configs/configs.h>

#include "main.lockcontrol.h"

class MainTelnetUI;

namespace AppMain {
    extern const String VERSION;
    extern const String DEFAULT_PREFIX;
    extern const String WIFI_HOSTNAME;

    extern const IIOHAL_IO_ID_TYPE ENDLINE_LOCK_INPUT_PIN;
    extern const IIOHAL_IO_ID_TYPE ENDLINE_UNLOCK_INPUT_PIN;
    extern const IIOHAL_IO_ID_TYPE MOTOR_ENABLE_OUTPUT_PIN;
    extern const IIOHAL_IO_ID_TYPE MOTOR_UNLOCK_OUTPUT_PIN;
    extern const IIOHAL_IO_ID_TYPE MOTOR_LOCK_OUTPUT_PIN;

    extern const IIOHAL_IO_ID_TYPE KEY_LOCK;
    extern const IIOHAL_IO_ID_TYPE KEY_UNLOCK;

    extern Scheduler scheduler;

    extern IIOHal *hal;
    extern ILogger *logService;
    extern IStorage *storage;
    extern INetwork *conService;
    extern IKeyboard *keyboard;
    extern TelnetServer *telnetInterface;
    extern MainTelnetUI *mainTelnetUI;
    extern VSTP::VstpClient *vssCli;
    extern Configs *configs;
    extern NamedLog nLog;
    extern MainLockControl *lockControl;

    extern const uint DEFAULT_LOCK_TIMEOUT_MS;
    extern const char CONFIG_LOCK_TIMEOUT[];

    uint getConfiguredLockTimeoutMs();
}

void setup();
void loop();

#endif
