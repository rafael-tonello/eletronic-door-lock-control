#ifndef IKEYBOARD_H
#define IKEYBOARD_H

#include <functional>
#include <vector>
#include <iiohal.h>
#include <eventstream.h>
#include <memory>

using namespace std;

class IKeyboard{
    struct KeyState{
        IIOHAL_IO_ID_TYPE key;
        bool rawState;
        bool debauncedState;
        unsigned long currentStateTime;
        unsigned long startTime;
    };

//must be implemented by the keyboard implementation {
    public:
        virtual vector<IIOHAL_IO_ID_TYPE> GetKeyIOS() = 0;
    protected:
        virtual bool IsKeyPressed(IIOHAL_IO_ID_TYPE key) = 0;
//}

//can be overridden (methods with defualt implementation){
    public:
        struct PressEvent{
            IIOHAL_IO_ID_TYPE key;
            unsigned long totalTime;
        };

        //called when a key is released (time will be count since the key was pressed until it was released)
        EventStream<PressEvent> OnKeyPress;

        //called when a key is release (time will be 0)
        EventStream<PressEvent> OnKeyUp;

        //called when a key is pressed (time will be 0)
        EventStream<PressEvent> OnKeyDown;

        //called periodicaly while a key is pressed (time will be count since the key was pressed until now)
        EventStream<PressEvent> OnKeyPressing;
        
        shared_ptr<EventStream<PressEvent>> OnSpecificKeyPress(IIOHAL_IO_ID_TYPE key){
            auto stream = make_shared<EventStream<PressEvent>>();
            OnKeyPress.listen([=](PressEvent event){
                if (event.key == key)
                    stream->emit(event);
            });
            return stream;
        }

        shared_ptr<EventStream<PressEvent>> OnSpecificKeyUp(IIOHAL_IO_ID_TYPE key){
            auto stream = make_shared<EventStream<PressEvent>>();
            OnKeyUp.listen([=](PressEvent event){
                if (event.key == key)
                    stream->emit(event);
            });
            return stream;
        }

        shared_ptr<EventStream<PressEvent>> OnSpecificKeyDown(IIOHAL_IO_ID_TYPE key){
            auto stream = make_shared<EventStream<PressEvent>>();
            OnKeyDown.listen([=](PressEvent event){
                if (event.key == key)
                    stream->emit(event);
            });
            return stream;
        }

        shared_ptr<EventStream<PressEvent>> OnSpecificKeyPressing(IIOHAL_IO_ID_TYPE key){
            auto stream = make_shared<EventStream<PressEvent>>();
            OnKeyPressing.listen([=](PressEvent event){
                if (event.key == key)
                    stream->emit(event);
            });
            return stream;
        }

        virtual KeyState GetKeyState(IIOHAL_IO_ID_TYPE key){
            if (lastKeysStates.count(key))
                return lastKeysStates[key];
            else
                return KeyState{.key = key, .rawState = false, .debauncedState = false, .currentStateTime = 0, .startTime = 0};
        }
    protected:
        bool started = false;
        

        std::map<IIOHAL_IO_ID_TYPE, KeyState> lastKeysStates;
        //this method should be called periodicaly by the keyboard implementation, it is used to check the state of the keys and call the appropriate callbacks
        virtual void default_work_step(){
            auto keyIOs = GetKeyIOS();
            if (!started){
                started = true;
                for (auto io: keyIOs)
                    lastKeysStates[io] = KeyState{
                        .key = io,
                        .rawState = false,
                        .debauncedState = false,
                        .currentStateTime = 0,
                        .startTime = 0
                    };
            }

            for (auto key: keyIOs) {
                bool rawState = IsKeyPressed(key);

                auto &keyState = lastKeysStates[key];
                if (rawState != keyState.rawState){
                    keyState.rawState = rawState;
                    keyState.currentStateTime = 0;
                    if (rawState)
                        keyState.startTime = millis();
                } else {
                    keyState.currentStateTime = millis() - keyState.startTime;

                    if (keyState.currentStateTime < 10)
                        return;

                    auto oldDebauncedState = keyState.debauncedState;
                    keyState.debauncedState = rawState;

                    if (rawState){
                        OnKeyPressing.emit(
                            PressEvent{
                                .key = key,
                                .totalTime = keyState.currentStateTime
                            }

                        );
                    }
                    
                    if (oldDebauncedState != keyState.debauncedState){
                        if (oldDebauncedState){
                            OnKeyUp.emit(
                                PressEvent{
                                    .key = key,
                                    .totalTime = 0
                                }
                            );

                            OnKeyPress.emit(
                                PressEvent{
                                    .key = key,
                                    .totalTime = keyState.currentStateTime
                                }
                            );
                        }
                        else {
                            OnKeyDown.emit(
                                PressEvent{
                                    .key = key,
                                    .totalTime = 0
                                }
                            );
                        }
                    }
                }
            }
        }
    //}
};

#endif
