
#ifndef __ICONSERVICE_H__
#define __ICONSERVICE_H__


#include <ESP8266WiFi.h> 
#include <Esp.h>
#include <tuple>
#include <eventstream.h>
#include <memory.h>

using namespace std;

typedef int IConServiceState;

enum IConServiceCommonStates {
    DISCONNECTED = 0, 
    CONNECTING=1,
    CONNECTED=2
};

class IConService{
public:

public:
    EventStream_Stateful<IConServiceState> conStateChangeEvent;

    virtual String stateToString(IConServiceState state) = 0;
    virtual shared_ptr<Client> createClient() = 0;
};

#endif