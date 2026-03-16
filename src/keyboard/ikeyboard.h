#include <functional>
#include <vector>
#include <iiohal.h>
#include <eventstream.h>

using namespace std;


class IKeyBoard{
    struct KeyState{
        IIOHAL_IO_ID_TYPE key;
        bool rawState;
        bool debauncedState;
        int currentStateTime;
        int startTime;
    };

//must be implemented by the keyboard implementation {
    public:
        virtual vector<IIOHAL_IO_ID_TYPE> GetKeyIOS() = 0;
    protected:
        virtual bool IsKeyPressed(char key) = 0;
//}

//can be overridden (methods with defualt implementation){
    public:
        struct PressEvent{
            IIOHAL_IO_ID_TYPE key;
            int time;
        };

        EventStream<PressEvent> OnKeyPress;
        EventStream<PressEvent> OnKeyPressing;

        virtual KeyState *GetKeyState(IIOHAL_IO_ID_TYPE key) = 0;
    protected:
        bool started = false;
        

        vector<KeyState> lastKeysStates;
        //this method should be called periodicaly by the keyboard implementation, it is used to check the state of the keys and call the appropriate callbacks
        virtual void default_work_step(){
            auto keyIOs = GetKeyIOS();
            if (!started){
                started = true;
                for (auto io: keyIOs)
                    lastKeysStates.push_back(
                        KeyState{
                            .key = io,
                            .rawState = false,
                            .debauncedState = false,
                            .currentStateTime = 0,
                            .startTime = 0
                        }
                    );
            }

            for (auto key: keyIOs) {
                bool rawState = IsKeyPressed(key);
                auto &keyState = lastKeysStates[key];
                if (rawState != keyState.rawState){
                    keyState.rawState = rawState;
                    keyState.currentStateTime = 0;
                    if (rawState)
                        keyState.startTime = millis();
                }
                else{
                    keyState.currentStateTime += millis() - keyState.startTime; //this method should be called periodicaly with a period of 100ms, so we can use this value to calculate the current state time

                    if (keyState.currentStateTime < 10)
                        return;

                    keyState.debauncedState = rawState;

                    if (rawState){
                        OnKeyPressing.emit(
                            PressEvent{
                                .key = key,
                                .time = keyState.currentStateTime
                            }

                        );
                    }
                    else if (keyState.currentStateTime > 0){
                        OnKeyPress.emit(
                            PressEvent{
                                .key = key,
                                .time = keyState.currentStateTime
                            }
                        );
                    }
                }
            }
        }


//}
};