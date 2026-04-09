#include "main.h"
#include "main.telnetui.h"

using namespace std;
using namespace AppMain;

namespace AppMain {
    const String VERSION = "v0.0.1";
    const String DEFAULT_PREFIX = "n1.sys.doorlock.";
    const String WIFI_HOSTNAME = "ESP8266-GENIO";

    const IIOHAL_IO_ID_TYPE ENDLINE_LOCK_INPUT_PIN = IOHAL_A14;
    const IIOHAL_IO_ID_TYPE ENDLINE_UNLOCK_INPUT_PIN = IOHAL_A15;
    const IIOHAL_IO_ID_TYPE MOTOR_ENABLE_OUTPUT_PIN = IOHAL_D0;
    const IIOHAL_IO_ID_TYPE MOTOR_UNLOCK_OUTPUT_PIN = IOHAL_D1;
    const IIOHAL_IO_ID_TYPE MOTOR_LOCK_OUTPUT_PIN = IOHAL_D2;

    const IIOHAL_IO_ID_TYPE KEY_LOCK = IOHAL_A13;
    const IIOHAL_IO_ID_TYPE KEY_UNLOCK = IOHAL_A12;

    Scheduler scheduler;

    IIOHal *hal = nullptr;
    ILogger *logService = nullptr;
    IStorage *storage = nullptr;
    INetwork *conService = nullptr;
    IKeyboard *keyboard = nullptr;
    TelnetServer *telnetInterface = nullptr;
    MainTelnetUI *mainTelnetUI = nullptr;
    VSTP::VstpClient *vssCli = nullptr;
    Configs *configs = nullptr;

    NamedLog nLog;
    MainLockControl *lockControl = nullptr;

    const uint DEFAULT_LOCK_TIMEOUT_MS = 26500;
    const char CONFIG_LOCK_TIMEOUT[] = "locker.timeoutMs";
}

bool tmp = true;

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

        configs = new Configs(*storage);
        
        conService = new WFService(scheduler, *logService, *storage, WIFI_HOSTNAME);
        
        telnetInterface = new TelnetServer(initialTelnetBanner, "humandis", scheduler, *storage, *conService, *logService);

        auto vstpPrefix = storage->get("vstp.prefix", DEFAULT_PREFIX);
        vssCli = new VSTP::VstpClient(scheduler, *conService, *logService, vstpPrefix);

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

        mainTelnetUI = new MainTelnetUI(telnetHelpTitleLine, *hal, *logService, *storage, *conService, *keyboard, vssCli, *telnetInterface, *lockControl, *configs);
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

    if (configs->get(CONFIG_LOCK_TIMEOUT, "") == "")
        configs->set(CONFIG_LOCK_TIMEOUT, String(DEFAULT_LOCK_TIMEOUT_MS));

    nLog.info("Main setup complete. Version: " + VERSION);

    lockControl->OnStateChange.listen([&](std::tuple<MainLockControl::WorkingState, String> stateInfo){
        auto state = MainLockControl::stateToString(std::get<0>(stateInfo));
        auto description = std::get<1>(stateInfo);
        vssCli->setVar("lockControl.state", state);
        vssCli->setVar("lockControl.description", description);
    });

    vssCli->listenVar("lockControl.state", [=](VSTP::VssVar newState){
        nLog.debug("Received new lock state via VSS: " + newState.value);
        if (newState.value == "lock"){
            lockControl->Lock(configs->get(CONFIG_LOCK_TIMEOUT, String(DEFAULT_LOCK_TIMEOUT_MS)).toInt())->then([=](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error locking the door (requested via VSS)"));
            } else {
                nLog.debug("door locked successfully (requested via VSS). Time taken: " + String(result.timeTaken) + "ms");
            }
        });
        } else if (newState.value == "unlock"){
            lockControl->Unlock(configs->get(CONFIG_LOCK_TIMEOUT, String(DEFAULT_LOCK_TIMEOUT_MS)).toInt())->then([=](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error unlocking the door (requested via VSS)"));
            } else {
                nLog.debug("door unlocked successfully (requested via VSS). Time taken: " + String(result.timeTaken) + "ms");
            }
        });
        }
    });

    keyboard->OnSpecificKeyPress(KEY_LOCK)->listen([&](IKeyboard::PressEvent event){
        auto timeoutMs = configs->get(CONFIG_LOCK_TIMEOUT, String(DEFAULT_LOCK_TIMEOUT_MS)).toInt();
        lockControl->Lock(timeoutMs)->then([=](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error locking the door (requested via keyboard)"));
            } else {
                nLog.debug("door locked successfully (requested via keyboard). Time taken: " + String(result.timeTaken) + "ms");
            }
        });
    });

    keyboard->OnSpecificKeyPress(KEY_UNLOCK)->listen([&](IKeyboard::PressEvent event){
        auto timeoutMs = configs->get(CONFIG_LOCK_TIMEOUT, String(DEFAULT_LOCK_TIMEOUT_MS)).toInt();
        lockControl->Unlock(timeoutMs)->then([=](MainLockControl::LockUnlockResult result){
            if (result.err != Errors::NoError){
                nLog.error(Errors::DerivateError(result.err, "error unlocking the door (requested via keyboard)"));
            } else {
                nLog.debug("door unlocked successfully (requested via keyboard). Time taken: " + String(result.timeTaken) + "ms");
            }
        });
    });
}

void loop()
{
    scheduler.run_one_round_of_tasks();
    ArduinoOTA.handle();
}
