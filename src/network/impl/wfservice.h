/**
 * Storage structure used by WIFI service
 * 
 *  wifi
 *      current "current ssid"
 *      knownssids
 *          list "ssid 1; ssid2; ssid3; current ssid; ssid 4"
 *          ssid 1
 *              password "the ssid psssword"
 *          ssid 2
 *              password "the ssid psssword"
*/

#ifndef __WFSERVICE_H__
#define __WFSERVICE_H__

#if defined(ESP8266)
    #include <ESP8266WiFi.h> 
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#include <Esp.h>
#include <tuple>
#include <eventstream.h>
#include <scheduler.h>
#include <scheduler/misc/promise.h>
#include "../inetwork.h"
#include "ilogger.h"
#include <errors/errors.h>
#include <stringutils.h>
#include <istorage.h>

using namespace std;
using namespace ProcessChain;

struct NetworkInfo{
    String ssid;
    int32_t rssid_dBm;
};

struct SavedNetworkInfo{
    NetworkInfo networkInfo;
    String password;
    int index;
};

class WFService: public INetwork{
public:
    enum WFServiceState{ //use custom state set instead of IConServiceBasicStates (but keep its values - DISCONNECTED=0, CONNECTING=1, CONNECTED=2)
        //IConServiceBasicStates states
        //DISCONNECTED = 0, //0 to 10 is disconnected states
        //CONNECTING = 10, //10 to 20 is connecting states
        //CONNECTED = 20, //20 to 30 is connected states

        //custom states
        BEGIN=2, 
        STARTING_AP=11,
        AP_STARTED=21
    };
    struct WifiConInfo{
        String ip;
        String gatewayIp;
        String broadcastIp;
        String macAddress;
        NetworkInfo networkInfo;
    };

    int connectToASsidRetryCount = 0;

    void preparePins();
private:
    shared_ptr<TimedTask> monitoringConnectionTask = nullptr;
    uint count = 0;
    uint milissecondsCount = 0;
    String connectedSsid = "";

    void showAvailableWifiNetworks();
    void displayAddressInfo(WifiConInfo info);

    void wifiMachineState();
    void monitorWifi();
    Scheduler &scheduler;
    //evtbus MessageBusTp *messageBus;
    NamedLog nLog;
    IStorage &storage;
    String hostName;

    /**
     * @brief Create a new Ssid to be used as the AP name. The password will be allwais '12345678'
     * 
     * @return tuple<String, String> A tuple with the format {ssid_name, '12345678'}
     */
    tuple<String, String> getAPSsidAndPassword();
    //void handleEventBusOperations();

    String wlStateToString(wl_status_t status);

    /*not pure*/void connectProcess();
    Promise<NetworkInfo>::smp_t getLastUsedNetwork();
    
    
    Promise<Error>::smp_t connectToNetwork(NetworkInfo network, bool registerNetworkIfSucess = true);
    Promise<ResultWithStatus<String>>::smp_t getNetworkdPassword(NetworkInfo network);
    
    Promise<TpNothing>::smp_t registerKnownNetwork(String ssid, String password);
    Promise<String>::smp_t getServiceInfo();
    void stopMonitorCurrentConnection();
    void startMonitoringCurrentConnection();
public:
    WFService(
        Scheduler &scheduler,
        ILogger& logService,
        IStorage &storage,
        String hostName
    ): 
        scheduler(scheduler), 
        storage(storage),
        hostName(hostName)
    {
        this->nLog = logService.getNLog("WFService");
    }

    
    //automatically manage the wifi (try connect, start acces point, ...)
    Promise<Error>::type autoConnectOrCreateAp();
    //create a new client. Destination is used to specify the server to be connected
    // Connect to a tcp server: tcp://ip:port
    // Connect to a udp server: udp://ip:port
    //if destination is empty, you will need to call the client's connect() method and init the client by yourself
    tuple<shared_ptr<Client>, Error> createClient(String destination = "") override;


    //create a server. Origin is used to specify the port to be listened:
    // for a tcp server tcp://port
    // for a udp server udp://port
    //if origin is empty, you will need to call the server->begin() method and init the server by yourself. In this case, an error will also be returned
    tuple<shared_ptr<WiFiServer>, Error> createServer(String origin = "") override;
    
    WifiConInfo getWifiCliAddrInfo();
    WifiConInfo getWifiApAddrInfo();
    String stateToString(IConServiceState state);

    //function for 'manual' wifi management
    Promise<vector<NetworkInfo>>::smp_t getAvailableNetworks();
    Promise<vector<SavedNetworkInfo>>::smp_t getRegisteredNetworks(bool onlyCurrentAvailableNetworks = true);
    Promise<Error>::smp_t deleteRegisteredNetwork(int index);
    Promise<Error>::smp_t deleteRegisteredNetwork(String indexOrSsid);
    Promise<Error>::smp_t deleteAllRegisteredNetworks();
    Promise<Error>::smp_t addOrUpdateRegisteredNetwork(String ssid, String password);
    Promise<Error>::smp_t changeRegisteredNetwork(String indexOrSsid, String newSsid, String newPassword);
    Promise<ResultWithStatus<int>>::smp_t getRegisteredNetworkIndex(String indexOrSsid);
    Promise<ResultWithStatus<SavedNetworkInfo>>::smp_t getRegisteredNetwork(String indexOrSsid);
    Promise<TpNothing>::smp_t startAccessPoint();
    Promise<Error>::smp_t connectToNetwork(String ssid, String password, bool registerNetworkIfSucess = true);

    
};

#endif