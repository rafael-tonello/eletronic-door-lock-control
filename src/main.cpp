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

#include "main.telnetui.h"

#include "main.lockcontrol.h"

using namespace std;



namespace {
    String VERSION = "v0.0.1";
    const String DEFAULT_PREFIX = "n1.sys.doorlock.";
    const String WIFI_HOSTNAME = "ESP8266-GENIO";

    const auto ENDLINE_LOCK_INPUT_PIN = IOHAL_A14;
    const auto ENDLINE_UNLOCK_INPUT_PIN = IOHAL_A15;
    const auto MOTOR_ENABLE_OUTPUT_PIN = IOHAL_D0;
    const auto MOTOR_UNLOCK_OUTPUT_PIN = IOHAL_D1;
    const auto MOTOR_LOCK_OUTPUT_PIN = IOHAL_D2;

    const auto KEY_LOCK = IOHAL_A13;
    const auto KEY_UNLOCK = IOHAL_A12;


    Scheduler scheduler;

    IIOHal *hal = nullptr;
    ILogger *logService = nullptr;
    IStorage *storage = nullptr;
    INetwork *conService = nullptr;
    IKeyboard *keyboard = nullptr;
    TelnetServer *telnetInterface = nullptr;
    MainTelnetUI *mainTelnetUI = nullptr;
    VSTP::VstpClient *vssCli = nullptr;

    NamedLog nLog;
    MainLockControl *lockControl = nullptr;
}

bool tmp = true;
uint timeout = 26500;

void setup()
{
    /* #region esp setup */
    Serial.begin(115200);
    delay(1000);

    ArduinoOTA.setHostname("espota");
    ArduinoOTA.begin();
    /* #endregion */

    String initialTelnetBanner = "\n";
    initialTelnetBanner += "===========================\n";
    initialTelnetBanner += "     Door lock control     \n";
    initialTelnetBanner += "===========================\n";
    String telnetHelpTitleLine = "Door lock control system, version " + VERSION;

    /* #region modules initialization { */
        hal = new ESP8266_74hc595_hc4067();

        keyboard = new GenericIIOHalKeyboard(*hal, scheduler, {KEY_LOCK, KEY_UNLOCK});
        
        logService = new DefaultLogger(true);
        nLog = logService->getNLog("Main");

        storage = new PreferencesLibraryStorage(scheduler);
        
        conService = new WFService(scheduler, *logService, *storage, WIFI_HOSTNAME);
        
        telnetInterface = new TelnetServer(initialTelnetBanner, "humandis", scheduler, *storage, *conService, *logService);

        auto vstpPrefix = storage->get("vstp.prefix", DEFAULT_PREFIX);
        vssCli = new VSTP::VstpClient(scheduler, *conService, *logService, vstpPrefix);

        mainTelnetUI = new MainTelnetUI(telnetHelpTitleLine, *hal, *logService, *storage, *conService, *keyboard, vssCli, *telnetInterface);
    /* #endregion } */
    
    /* #region wifi startup */
        ((WFService*)conService)->autoConnectOrCreateAp()->then([&](Error err) {
            if (err != Errors::NoError) {
                nLog.error(Errors::DerivateError(err, "Error while starting wifi management"));
            } else {
                nLog.info("Wifi management started successfully");
            }
        });
    /* #endregion */

    /* #region vss startup { */
        vssCli->onConnectionStateChange.listen([&](VSTP::VstpClientState newState) {
            if (newState == VSTP::VstpClientState::CONNECTED) {
                auto wifiInfo = static_cast<WFService *>(conService)->getWifiCliAddrInfo();
                vssCli->setVar("deviceInfo.network.ssid", wifiInfo.networkInfo.ssid);
                vssCli->setVar("deviceInfo.network.ip", wifiInfo.ip);
                vssCli->setVar("deviceInfo.network.macAddress", wifiInfo.macAddress);
                vssCli->setVar("deviceInfo.firmwareVersion", VERSION);
            }
        });


        auto vstpServer = storage->get("vstp.server", "");
        auto vstpPortRaw = storage->get("vstp.port", "5032");
        auto vstpPort = atoi(vstpPortRaw.c_str());

        logService->log(LogLevel::INFO, "Main", "Starting VSS connection to server '" + vstpServer + "' on port " + String(vstpPort) + "'");
        
        auto err = vssCli->start(vstpServer, vstpPort);
        if (err != Errors::NoError) {
            nLog.error(Errors::DerivateError(err, "error starting VSTP client"));
        } else {
            nLog.info("VSTP client started successfully");
        }
    /* #endregion */

    auto runs = storage->get("misc.runs", "0");
    storage->set("misc.runs", String((runs.toInt() + 1)));
    nLog.info("Run count: " + runs);

    nLog.info("Main setup complete. Version: " + VERSION);

    lockControl = new MainLockControl(
        scheduler, 
        *hal, 
        *logService, 
        MOTOR_LOCK_OUTPUT_PIN, 
        MOTOR_UNLOCK_OUTPUT_PIN, 
        ENDLINE_LOCK_INPUT_PIN, 
        ENDLINE_UNLOCK_INPUT_PIN, 
        MOTOR_ENABLE_OUTPUT_PIN
    );

    lockControl->OnStateChange.listen([&](std::tuple<MainLockControl::WorkingState, String> stateInfo){

        auto state = MainLockControl::stateToString(std::get<0>(stateInfo));
        auto description = std::get<1>(stateInfo);
        vssCli->setVar("lockControl.state", state);
        vssCli->setVar("lockControl.description", description);
    });

    keyboard->OnSpecificKeyPress(KEY_LOCK)->listen([&](IKeyboard::PressEvent event){
        lockControl->Lock(timeout)->then([](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error locking the door"));
            } else {
                nLog.debug("door locked successfully. Time taken: " + String(result.timeTaken) + "ms");
            }
        });
    });

    keyboard->OnSpecificKeyPress(KEY_UNLOCK)->listen([&](IKeyboard::PressEvent event){
        lockControl->Unlock(timeout)->then([](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error unlocking the door"));
            } else {
                nLog.debug("door unlocked successfully. Time taken: " + String(result.timeTaken) + "ms");
            }
        });
    });
}

void loop()
{
    scheduler.run_one_round_of_tasks();
    ArduinoOTA.handle();
}
