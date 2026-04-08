//usage example
#include <scheduler/misc/promise.h>
#include <iiohal.h>
#include <scheduler/scheduler.h>

using namespace ProcessChain;

class MainLockControl {
public:
    enum WorkingState{
        UNKNOWN,
        ABORT_REQUESTED,
        ABORTED,
        LOCKING,
        UNLOCKING,
        LOCKED,
        UNLOCKED,
        PANIC
    };  
private:
    enum Direction{
        DLock,
        DUnlock
    };

    WorkingState currentState = UNKNOWN;

    Scheduler &scheduler;
    IIOHal &hal;
    NamedLog nLog;

    IIOHAL_IO_ID_TYPE LOCK_OUTPUT_PIN;
    IIOHAL_IO_ID_TYPE UNLOCK_OUTPUT_PIN;
    IIOHAL_IO_ID_TYPE LOCK_LIMIT_INPUT_PIN;
    IIOHAL_IO_ID_TYPE UNLOCK_LIMIT_INPUT_PIN;
    IIOHAL_IO_ID_TYPE ENABLE_MOTOR_PIN;
    
public:

    struct LockUnlockResult{
        Error err;
        uint timeTaken;
    };
  

    MainLockControl (
        Scheduler &scheduler,
        IIOHal &iohal,
        ILogger &logger, 
        IIOHAL_IO_ID_TYPE PLOCK_OUTPUT_PIN,
        IIOHAL_IO_ID_TYPE PUNLOCK_OUTPUT_PIN,
        IIOHAL_IO_ID_TYPE PLOCK_LIMIT_INPUT_PIN,
        IIOHAL_IO_ID_TYPE PUNLOCK_LIMIT_INPUT_PIN,
        IIOHAL_IO_ID_TYPE PENABLE_MOTOR_PIN
    ):  scheduler(scheduler), 
        hal(iohal), 
        LOCK_OUTPUT_PIN(PLOCK_OUTPUT_PIN), 
        UNLOCK_OUTPUT_PIN(PUNLOCK_OUTPUT_PIN), 
        LOCK_LIMIT_INPUT_PIN(PLOCK_LIMIT_INPUT_PIN), 
        UNLOCK_LIMIT_INPUT_PIN(PUNLOCK_LIMIT_INPUT_PIN),
        ENABLE_MOTOR_PIN(PENABLE_MOTOR_PIN)
    {
        nLog = logger.getNLog("Main::LockControl");
    }


    Promise<LockUnlockResult>::type Lock(uint timeout){
        return lockUnlock(DLock, timeout);
    }

    Promise<LockUnlockResult>::type Unlock(uint timeout){
        return lockUnlock(DUnlock, timeout);
    }

    EventStream<std::tuple<WorkingState, String>> OnStateChange;

    static String stateToString(WorkingState state){
        switch (state)
        {
        case UNKNOWN:
            return "unknown";
        case ABORTED:
            return "aborted";
        case LOCKING:
            return "locking";
        case UNLOCKING:
            return "unlocking";
        case LOCKED:
            return "locked";
        case UNLOCKED:
            return "unlocked";
        case PANIC:
            return "panic";
        default:
            return "invalid_state: " + String((int)state);
        }
    }

protected:
    Promise<LockUnlockResult>::type lockUnlock(Direction direction, uint timeout){
        auto ret = Promise<LockUnlockResult>::get_smp();

        if ((direction == DLock)){
            if (currentState == LOCKED){
                ret->post({Errors::NoError, 0});
                return ret;
            } else if (currentState == LOCKING){
                ret->post({Errors::NoError, 0});
                return ret;
            }

        } else if (direction == DUnlock){
            if (currentState == UNLOCKED){
                ret->post({Errors::NoError, 0});
                return ret;
            } else if (currentState == UNLOCKING){
                ret->post({Errors::NoError, 0});
                return ret;
            }
        } else {
            ret->post({"Invalid Direction", 0});
            return ret;
        }

        //abort a previus operation (if any).
        if (currentState == LOCKING || currentState == UNLOCKING){
            currentState = ABORT_REQUESTED;
            scheduler.periodicTask(10 MiliSeconds, [=](shr_tmdtsk tasks){
                if (currentState == ABORTED){
                    lockUnlock(direction, timeout)->then([=](LockUnlockResult result){
                        ret->post(result);
                    });
                    tasks->abort();
                } else if (tasks->taskAge > 1000){
                    ret->post({"can't abort a previous operation", tasks->taskAge});
                    tasks->abort();
                }
            }, "", DEFAULT_PRIORITIES::HIGH_);
            return ret;
        }

        

        nLog.debug("starting to " + String(direction == DLock ? "lock" : "unlock") + " with timeout " + String(timeout) + "ms");

        auto PIN_TO_ENABLE = direction == DLock ? LOCK_OUTPUT_PIN : UNLOCK_OUTPUT_PIN;
        auto PIN_TO_DISABLE = direction == DLock ? UNLOCK_OUTPUT_PIN : LOCK_OUTPUT_PIN;
        auto ENDLINE_PIN = direction == DLock ? LOCK_LIMIT_INPUT_PIN : UNLOCK_LIMIT_INPUT_PIN;

        

        currentState = direction == DLock ? LOCKING : UNLOCKING;
        OnStateChange.stream(
            std::make_tuple(currentState, String(direction == DLock ? "locking" : "unlocking"))
        );

        hal.Write(PIN_TO_DISABLE, IIOHal::WriteValue::FromDigital(false)); //only for safety.
        hal.Write(PIN_TO_ENABLE, IIOHal::WriteValue::FromDigital(true));

        hal.Write(ENABLE_MOTOR_PIN, IIOHal::WriteValue::FromDigital(true));

        scheduler.periodicTask(5 MiliSeconds, [=](shr_tmdtsk task){
            //check if operation was aborted
            if (currentState == ABORT_REQUESTED){
                hal.Write(ENABLE_MOTOR_PIN, IIOHal::WriteValue::FromDigital(false));
                hal.Write(LOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
                hal.Write(UNLOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
                currentState = ABORTED;
                OnStateChange.stream(std::make_tuple(currentState, "action aborted. Time taken until abort: " + String(task->taskAge) + "ms / " + String(timeout) + "ms"));
                ret->post({"action canceled", task->taskAge});
                task->abort();  
                return;
            }

            //read endline (if any error happens while reading endline, trigger 'panic' mode)
            auto endline = hal.Read(ENDLINE_PIN);
            if (endline.err != Errors::NoError){
                String msg = "failed to "+String(direction == DLock ? "lock" : "unlock")+": error reading endline pin: " + endline.err;
                lockPanic(msg);
                ret->post({msg, task->taskAge});
                return;
            }

            //if endline was reached, gracefully end the current process
            if (endline.Digital()){
                hal.Write(ENABLE_MOTOR_PIN, IIOHal::WriteValue::FromDigital(false));
                hal.Write(PIN_TO_ENABLE, IIOHal::WriteValue::FromDigital(false));
                currentState = direction == DLock ? LOCKED : UNLOCKED;
                ret->post({Errors::NoError, task->taskAge});
                OnStateChange.stream(std::make_tuple(currentState, String(direction == DLock ? "locked" : "unlocked") + ". Time taken: " + String(task->taskAge) + "ms / " + String(timeout) + "ms"));
            } else if (task->taskAge > timeout){
            //if timeout was reached, trigger 'panic' mode
            
                String msg = "failed to "+String(direction == DLock ? "lock" : "unlock")+": timeout (" + String(task->taskAge) + "ms / " + String(timeout) + "ms)";
                lockPanic(msg);
                ret->post({msg, task->taskAge});
            }

            //task only should run while locking or unlocking, if for some reason the state changes, abort the task. If abor was requested while task is running, let it be "gracefull aborted"
            if (currentState != LOCKING && currentState != UNLOCKING && currentState != ABORT_REQUESTED){
                task->abort();
            }
        }, "", DEFAULT_PRIORITIES::HIGH_);

        return ret;

    }

    void abortCurrentAction(const String& reason = "unknown reason"){
        hal.Write(ENABLE_MOTOR_PIN, IIOHal::WriteValue::FromDigital(false));
        hal.Write(LOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
        hal.Write(UNLOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
        currentState = ABORT_REQUESTED;
        OnStateChange.stream(std::make_tuple(currentState, reason));
        nLog.warning("current action aborted: " + reason);
    }

    void lockPanic(const String& reason = "unknown reason"){
        hal.Write(ENABLE_MOTOR_PIN, IIOHal::WriteValue::FromDigital(false));
        hal.Write(LOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
        hal.Write(UNLOCK_OUTPUT_PIN, IIOHal::WriteValue::FromDigital(false));
        currentState = PANIC;
        nLog.critical("panic mode activated: " + reason);
        OnStateChange.stream(std::make_tuple(currentState, reason));
    }

   
private:
    
};
