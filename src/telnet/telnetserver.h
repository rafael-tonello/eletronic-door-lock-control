#ifndef __TELNETINTERFACE__H__ 
#define __TELNETINTERFACE__H__ 

#include <vector>
#include <atomic>
#include <map>
#include <WString.h>

#include <scheduler.h>
#include <inetwork.h>
#include <scheduler/misc/promise.h>
#include <errors/errors.h>
#include <stringutils.h>
#include <istorage.h>
#include <ilogger.h>

using namespace std;
using namespace ProcessChain;


class TelnetServer { 
public:
    class CliInfo {
    public:
        //a generic key-value store that can be used for any purpose
        std::map<String, String> tags; 

        enum AuthState {
            AS_UNAUTHENTICATED,
            AS_AUTHENTICATING,
            AS_AUTHENTICATED
        };
        IPAddress remoteIP;
        int remotePort;
        WiFiClient theClient;
        String buffer;
        AuthState authenticationState = AS_UNAUTHENTICATED;
        int id;
        int passwordAttempts = 0;

        bool currentlyIsProcessingALine = false;
        int telnetCmdBytesToSkip = 0;
        bool ignoreNextLf = false;

        virtual void sendData(String data, bool breakLine = true) = 0;

        EventStream<tuple<String, function<void()>>> onDataReceived;
    };

    class CliInfoExtented: public CliInfo {
    public:
        function<void(String, bool)> ___sendDataf = nullptr;

        void sendData(String data, bool breakLine = true) override{
            if (___sendDataf != nullptr)
                ___sendDataf(data, breakLine);
        }
    };

private:
    const int TASKS_PRIORITY = DEFAULT_PRIORITIES::VERY_LOW; //DEFAULT_PRIORITIES::LOW_;
    const String InitialBanner = "";
    const int MAX_PASSWORD_ATTEMPTS = 3;
    const String TERMINAL_INPUT_INDICATOR = "# ";

    int cliIdCount = 0;

    String telnetDefaultPassword;

    shared_ptr<WiFiServer> server = nullptr;
    Scheduler &scheduler;
    IStorage &storage;
    INetwork &conService;


    NamedLog nLog;

    vector<int> cliIds;
    std::map<int, CliInfoExtented> clients;

    atomic<bool> working;


    void work();
    void picUpkNextClient();
    Promise<TpNothing>::smp_t talkToClients();
    Promise<TpNothing>::smp_t TalkToAClient(CliInfo &cli);
    void processLineReceivedFromClient(CliInfo &cli, String receivedLine);
    void sendToClient(CliInfo &cli, String message, bool breakLine = true);
    Promise<TpNothing>::smp_t sendInitialInfoToClient(CliInfo &cli);
    void finalizeClient(CliInfo &cli);
    Promise<Error>::smp_t log_cmds(CliInfo &cli, String cmd);
    Promise<Error>::smp_t messagebus_cmds(CliInfo &cli, String cmd);
    Promise<Error>::smp_t display_help(CliInfo& cli);
    
    Promise<bool>::smp_t validadtePassword(String password);
    void setClientEcho(CliInfo &cli, bool enabled);

    void stopOldServer();

    String getServiceInfo();
public: 

    TelnetServer(
        String initialBanner,
        String telnetDefaultPassword,
        Scheduler &scheduler, 
        IStorage &storage,
        INetwork &conService, 
        ILogger &logService
    );

    ~TelnetServer(); 

    //called after client be called and authenticated
    EventStream<CliInfo&> onClientConnected;
    EventStream<CliInfo&> onClientDisconnected;
    EventStream<tuple<CliInfo&, String, function<void()>>> onClientDataReceived;

    void SendData(Client& client, String data);
    void BroadcastData(String data);
    vector<Client&> getConnectedClients();


}; 
 
#endif 
