//usage example
#include <scheduler/misc/promise.h>
#include <iiohal.h>
#include <scheduler/scheduler.h>

using namespace ProcessChain;

class LockControl {
private:
    enum WorkingState{
        IDLE,
        LOCKING,
        UNLOCKING
    };    

    WorkingState currentState = IDLE;

    Scheduler &scheduler;
    IIOHal &iohal;

    IIOHAL_IO_ID_TYPE lockIoOut;
    IIOHAL_IO_ID_TYPE unlockIoOut;
    IIOHAL_IO_ID_TYPE lockLimitSwitchIoIn;
    IIOHAL_IO_ID_TYPE unlockLimitSwitchIoIn;
public:
    LockControl (
        Scheduler &scheduler,
        IIOHal &iohal, 
        IIOHAL_IO_ID_TYPE lockIoOut,
        IIOHAL_IO_ID_TYPE unlockIoOut,
        IIOHAL_IO_ID_TYPE lockLimitSwitchIoIn,
        IIOHAL_IO_ID_TYPE unlockLimitSwitchIoIn
    ):  scheduler(scheduler), 
        iohal(iohal), 
        lockIoOut(lockIoOut), 
        unlockIoOut(unlockIoOut), 
        lockLimitSwitchIoIn(lockLimitSwitchIoIn), 
        unlockLimitSwitchIoIn(unlockLimitSwitchIoIn)
    {
        //constructor code here
    }


    Promise<Error>::type Lock(uint timeout){
        return LockUnlock(true, timeout);
    }

    Promise<Error>::type Unlock(uint timeout){
        return LockUnlock(false, timeout);
    }

    Error AbortCurrentOperation(){
        //abort current lock/unlock operation
        auto err1 = iohal.Write(lockIoOut, false);
        auto err2 = iohal.Write(unlockIoOut, false);

        if (err1 != Errors::NoError && err2 != Errors::NoError)
            return Errors::DerivateError(err1, "Failed to abort lock and unlock operations");
        else if (err1 != Errors::NoError)
            return Errors::DerivateError(err1, "Failed to abort lock operation");
        else if (err2 != Errors::NoError)            
            return Errors::DerivateError(err2, "Failed to abort unlock operation");

        return Errors::NoError;
    }

    //returns true if there is a lock or unlock operation in progress
    bool IsWorking(){
        return currentState != WorkingState::IDLE;
    }
private:
    Promise<Error>::type LockUnlock(bool lock, uint timeout){
        auto ret = Promise<Error>::instance();
        if (lock)
        {
            iohal.Write(lockIoOut, true);
            currentState = WorkingState::LOCKING;
        } else {
            iohal.Write(unlockIoOut, true);
            currentState = WorkingState::UNLOCKING; 
        }

        scheduler.periodicTask(100, [=](std::shared_ptr<TimedTask> task){
            bool lockLimitSwitchState = iohal.Read(lockLimitSwitchIoIn).Digital();
            bool unlockLimitSwitchState = iohal.Read(unlockLimitSwitchIoIn).Digital();

            if (lock && lockLimitSwitchState){
                ret->post(Errors::NoError);
                currentState = WorkingState::IDLE;
                task->abort();

            }
            else if (!lock && unlockLimitSwitchState){
                ret->post(Errors::NoError);
                currentState = WorkingState::IDLE;

                task->abort();
            }

            //check timeout
            if (task->taskAge >= timeout){
                auto err = this->AbortCurrentOperation();
                currentState = WorkingState::IDLE;
                if (err != Errors::NoError){
                    ret->post(Errors::DerivateError(err, Errors::TimeoutError + " + Failed to abort lock/unlock operation after timeout"));
                    return;
                }
                ret->post(Errors::TimeoutError);
                task->abort();
            }
        }, "LockUnlockTimeoutCheck", 'm');

        return ret;
    }
};