#include <Arduino.h>
#include <ArduinoOTA.h>
#include <errors/errors.h>

#include <scheduler.h>

#include <ikeyboard.h>
#include <keyboard/impl/GenericIIOHalKeyboard.h>

#include <iiohal.h>
#include <iohal/impl/esp8266IOHal.h>

#include <ilogger.h>

#include <istorage.h>
#include <storage/impl/preferenceslibrary/preferenceslibrarystorage.h>

#include <inetwork.h>

#include <network/inetwork.h>
#include <network/impl/wfservice.h>

#include <telnet/telnetserver.h>

#include "main.telnetui.h"

Scheduler scheduler;

IIOHal *hal;
ILogger *logService;
IStorage *storage;
INetwork *conService;
IKeyboard* keyboard; //= new KeyBoardImplementation();
TelnetServer *telnetInterface; //= new TelnetServer(*logService, *storage, *conService, scheduler);
MainTelnetUI* mainTelnetUI;
bool otaStarted = false;

bool tmp = false;

void setup() {
    String initailTelnetBanner = "\n";
    initailTelnetBanner += "===========================\n"; 
    initailTelnetBanner += "     Door lock control     \n"; 
    initailTelnetBanner += "===========================\n";



    Serial.begin(115200);
    delay(1000);

    ArduinoOTA.setHostname("espota");
    ArduinoOTA.begin();

    hal = new ESP8266IOHal();
    keyboard = new GenericIIOHalKeyboard(*hal, scheduler, {IOHAL_D1});
    logService = new DefaultLogger();
    storage = new PreferencesLibraryStorage(scheduler);
    conService = new WFService(scheduler, *logService, *storage);
    Serial.println("conservice address: " + String((uintptr_t)conService));

    telnetInterface = new TelnetServer(initailTelnetBanner, "humandis", scheduler, *storage, *conService, *logService);



    mainTelnetUI = new MainTelnetUI(*hal, *logService, *storage, *conService, *keyboard, *telnetInterface);

    ((WFService*)conService)->connectToNetwork("Tonello", "tetelateteka")->then([&](Error err){
        
    });


    scheduler.periodicTask(100 MilliSeconds, [&](){
        auto v = hal->Read(IOHAL_D3).Digital();
        if (v != tmp){
            tmp = v;
            if (v)
                logService->log(LogLevel::INFO, "debuging", "HIGH");
            else
                logService->log(LogLevel::INFO, "debuging", "LOW");
        }
    });


    //((WFService*)conService)->autoConnectOrCreateAp();
    

}

void loop() {
    scheduler.run_one_round_of_tasks();
    ArduinoOTA.handle();
}


// KeyBoard.ListenPress(KEY_LOCK, [](int key, int totalTime){
//     LockUnlock(true)->then([](Error err){ });
// }

// KeyBoard.ListenPress(KEY_UNLOCK, [](int key, int totalTime){
//     LockUnlock(true)->then([](Error err){ });
// }

// //ListePressing is called periodicaly ~100ms while akey is pressed
// KeyBoard.ListenPressing(KEY_ANY, [](int key, int currentPressingTime){
//     LockUnlock(true)->then([](Error err){ });
// }



// #define VSS_LOCK_STATE_UNLOCKED "0"
// #define VSS_LOCK_STATE_LOCKED "1"
// #define VSS_LOCK_STATE_UNKNOWN "2"
// #define VSS_LOCK_STATE_UNLOCKREQUESTED "10"
// #define VSS_LOCK_STATE_LOCKREQUESTED "11"
// #define VSS_LOCK_STATE_UNLOCKING "20"
// #define VSS_LOCK_STATE_LOCKING "21"
// #define VSS_LOCK_STATE_UNLOCKING_ERROR "30"
// #define VSS_LOCK_STATE_LOCKING_ERROR "31"


//Promise<Error>::smp_t LockUnLock(bool lock){
//vss.set("lockStateError", "");
//if locker.isWorking(){
//    return Errors::AlreadyWorking;
//    //abort lock/unlock
//    locker.abort();
//vss.set("lockState", VSS_LOCK_STATE_UNKNOWN);
//}
//    auto ret = Promise<Error>::getSmp();
//
//    auto finish = [=](Error err){
//        if (err != Errors::NoError){
//            auto err = Errors.New("Failed to "+(lock ? "lock" : "unlock")+": " + String(err));
//            if lock {
//                vss.set("lockState", VSS_LOCK_STATE_LOCKING_ERROR);
//            } else {
//                vss.set("lockState", VSS_LOCK_STATE_UNLOCKING_ERROR);
//            }
//
//            vss.set("lockStateError", err);
//            ret->post();
//            return;
//        }
//
//        if lock {
//            vss.set("lockState", VSS_LOCK_STATE_LOCKED);
//        } else {
//            vss.set("lockState", VSS_LOCK_STATE_UNLOCKED);
//        }
//        ret->post(Errors::NoError);
//        
//    };
//
//    if lock (){
//        vss.set("lockState", VSS_LOCK_STATE_LOCKING)
//        locker.lock()->then(finish);
//    } else {
//        vss.set("lockState", VSS_LOCK_STATE_UNLOCKING)
//        locker.unlock()->then(finish);
//    }
//}


