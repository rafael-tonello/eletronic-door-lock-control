#ifndef __VSTPCLIENT__H__ 
#define __VSTPCLIENT__H__ 

/* vstp is a text based protocol that allow real time communication with a VSS server, a key-value database with a native notification system*/

#include <map>
#include <inetwork.h>
#include <ilogger.h>
#include <errors/errors.h>
#include <stringutils.h>
#include <scheduler/misc/promise.h>
#include <istorage.h>
#include <scheduler.h>
#include <network/impl/wfservice.h>

namespace VSTP {
    using namespace std;
    using namespace ProcessChain;

    class VssVar{
    public:
        String name;
        String value;

        VssVar(String name, String value): name(name), value(value){}
    };

    struct VstpCmdAndPayload{
        String cmd;
        String payload;
    };

    class VstpErrors{
    public:
        static Error disconected;
        static Error errorReadingHeaders;
    };


    struct VarListenInfo{
        uint id;
        String varName;
        function<void(VssVar var)> f;
    };

    enum VstpClientState {DISCONNECTED, WAITING_NEXT_IP, CONNECTING, CONNECTED};
    class VstpClient{
    private:
        const String ACTIONS_SEND_SERVER_INFO_AND_CONFS = "serverinfo";
        const String ACTIONS_PING = "ping";
        const String ACTIONS_PONG = "pong";
        const String ACTIONS_SUGGEST_NEW_CLI_ID = "sugestednewid";
        const String ACTIONS_CHANGE_OR_CONFIRM_CLI_ID = "setid";
        const String ACTIONS_TOTAL_VARIABLES_ALREADY_BEING_OBSERVED = "aoc";
        const String ACTIONS_RESPONSE_BEGIN = "beginresponse";
        const String ACTIONS_RESPONSE_END = "endresponse";
        const String ACTIONS_SET_VAR = "set";
        const String ACTIONS_DELETE_VAR = "delete";
        const String ACTIONS_DELETE_VAR_RESULT = "deleteresult";
        const String ACTIONS_GET_VAR = "get";
        const String ACTIONS_GET_VAR_RESPONSE = "varvalue";
        const String ACTIONS_SUBSCRIBE_VAR = "subscribe";
        const String ACTIONS_UNSUBSCRIBE_VAR = "unsubscribe";
        const String ACTIONS_VAR_CHANGED = "varchanged";
        const String ACTIONS_GET_CHILDS = "getchilds";
        const String ACTIONS_GET_CHILDS_RESPONSE = "childs";
        const String ACTIONS_LOCK_VAR = "lock";
        const String ACTIONS_UNLOCK_VAR = "unlock";
        const String ACTIONS_LOCK_VAR_RESULT = "lockresult";
        const String ACTIONS_UNLOCK_VAR_DONE = "unlockdone";
        const String ACTIONS_SERVER_BEGIN_HEADERS = "beginserverheaders";
        const String ACTIONS_SERVER_END_HEADERS = "endserverheaders";
        const String ACTIONS_HELP = "help";
        const String ACTIONS_SET_TELNET_SESSION = "telnet";
        const String ACTIONS_CHECK_VAR_LOCK_STATUS = "lockstatus";
        const String ACTIONS_CHECK_VAR_LOCK_STATUS_RESULT =  "lockstatusresult";
        const String ACTIONS_ERROR = "error";

        EventStream<VstpCmdAndPayload> onCommandReceived;

        shared_ptr<Promise<bool>> connect;

        Scheduler &scheduler;
        INetwork &network;
        NamedLog nLog;
        WiFiClient cli;
        String incomingBuffer = "";
        vector<String> serverHeaders;
        String currentId = "";

        String serverAddr;
        int serverPort;

        shr_tmdtsk monitorTask = nullptr;
        shr_tmdtsk talkToServerTask = nullptr;
        int networkStateListenId = -1;

        //flag used to prevent double connecting process
        bool connectingInProgress = false;

        uint observerIdCount = 1;
        std::map<uint, VarListenInfo> observings;

        //run the connection process using saved data. Load data and calls connectAndUpdateSaveddata
        Promise<Error>::smp_t connectWithSavedData();

        

        Promise<Error>::smp_t finalizePreviousConnection();
        Promise<Error>::smp_t connectToTheTCPServer(String server, int port);
        Promise<Error>::smp_t readHeaders();
        Promise<Error>::smp_t exchangeIds();
        void talkToServer();
        void processReceivedLine(String line);

        vector<String> getHeaders(function<bool(String)> isValid);
        vector<String> getHeaders(String contains){return getHeaders([contains](String curr){ return curr.indexOf(contains) > -1; });}
        String getSugestedId();

        void processVssCommand(String receivedLine);
        void processVssCommand(String command, String data);

        void sendBytesToServer(const char* buffer, size_t size);
        void sendToServer(String data, bool sendLineBreakToo = true);
        void sendToServer(String cmd, String payload){ sendToServer(cmd + ":" + payload);};
        void sendToServer(VstpCmdAndPayload cmdAndPayload) {sendToServer(cmdAndPayload.cmd, cmdAndPayload.payload);}
        Promise<ResultWithStatus<vector<VstpCmdAndPayload>>>::smp_t getCommandsUntil(function<bool(VstpCmdAndPayload)> isToStop, int timeout = 5000);

        void handle_var_change_cmd(VstpCmdAndPayload cmdAndPayload);
        tuple<String, String> separateNameAndMetadata(String originalVarName);
        String prefix;

        String readingLineBuffer;

        void disconnect();

        String getServiceInfo();
        

        void startConnectionMonitor();
        void stopConnectionMonitor();

        //run the connection process using the given server and port
        Promise<Error>::smp_t connectToServer(String server, int port);

        void setServerPortAndPrefix(String server, int port, String);


    public:
        VstpClient(
            Scheduler &scheduler,
            INetwork &network,
            ILogger &logService,
            String varsPrefix = ""
        );
        ~VstpClient();

        

        //sets the server and port, and if no errors happens will begin the connection process, e.g., will keep trying to connect
        Error start(String server, int port);

        //stops monitoring and closes the active VSTP/VSS connection
        void stop();
        

        EventStream_Stateful<VstpClientState> onConnectionStateChange;


        //set a variable in the server. All observing clientes will receive the update inmediatly. If the variable does not exist, it will be created
        Promise<Error>::smp_t setVar(String varName, String value);
        
        //get var can return one or many variables (get may be done using wildcard, like "var*")
        Promise<ResultWithStatus<vector<VssVar>>>::smp_t getVar(String varName);

        //remove a variable from the server
        Promise<Error>::smp_t deleteVar(String varName);
        //// Promise<Error>::smp_t lockVar(strign varName, uint timeout_ms = UINT_MAX);
        //// Promise<Error>::smp_t unlockVar(strign varName);


        //observe a variable. The function 'f' will be called every time the variable changes. The returned uint is an id that can be used to stop observing
        Promise<uint>::smp_t listenVar(String varName, function<void(VssVar var)> f);

        //stop observing a variable. The listenId is the id returned by listenVar
        Promise<TpNothing>::smp_t stopListenVar(uint listenId);

        
    }; 
}
 
#endif 
