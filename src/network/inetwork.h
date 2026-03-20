
#ifndef __ICONSERVICE_H__
#define __ICONSERVICE_H__


#include <ESP8266WiFi.h> 
#include <Esp.h>
#include <tuple>
#include <eventstream.h>
#include <memory.h>
#include <errors/errors.h>

using namespace std;

typedef int IConServiceState;

enum IConServiceCommonStates {
    DISCONNECTED = 0, //0 to 9 is disconnected
    CONNECTING=10, //10 to 19 is connecting states
    CONNECTED=20 //20 to 29 is connected states,

};

class INetwork{
public:

public:
    EventStream_Stateful<IConServiceState> conStateChangeEvent;

    virtual String stateToString(IConServiceState state) = 0;

    //create a new client. Destination is used to specify the server to be connected
    // Connect to a tcp server: tcp://ip:port
    // Connect to a udp server: udp://ip:port
    //if destination is empty, you will need to call the client's connect() method and init the client by yourself
    virtual tuple<shared_ptr<Client>, Error> createClient(String destination = "") = 0;


    //create a server. Origin is used to specify the port to be listened:
    // for a tcp server tcp://port
    // for a udp server udp://port
    //if origin is empty, you will need to call the server->begin() method and init the server by yourself. In this case, an error will also be returned
    virtual tuple<shared_ptr<WiFiServer>, Error> createServer(String origin = "") = 0;
};

#endif