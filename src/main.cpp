#include <Arduino.h>
#include <scheduler.h>
#include <ikeyboard.h>
#include <errors/errors.h>

Scheduler scheduler;

IKeyboard* keyboard; //= new KeyBoardImplementation();

void setup() {
    Serial.begin(115200);
    scheduler.periodicTask(1000, [](){
        Serial.println("Hello world!");
    });
}

void loop() {
    scheduler.run_one_round_of_tasks();
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


