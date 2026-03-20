#include  "telnetserver.h" 

namespace {
    const uint8_t TELNET_IAC = 255;
    const uint8_t TELNET_WILL = 251;
    const uint8_t TELNET_WONT = 252;
    const uint8_t TELNET_OPT_ECHO = 1;
    const uint8_t TELNET_OPT_SUPPRESS_GO_AHEAD = 3;
}
 
TelnetServer::TelnetServer(
    String systemName,
    String telnetDefaultPassword,
    Scheduler &scheduler, 
    IStorage &storage,
    INetwork &conService, 
    ILogger &logService
): 
    InitialBanner(systemName),
    telnetDefaultPassword(telnetDefaultPassword),
    scheduler(scheduler), 
    storage(storage),
    conService(conService)
{ 
    nLog = logService.getNLog("TelnetServer");
    working = false;
    
    conService.conStateChangeEvent.listen([&](IConServiceState conStatus){
        //connected = 20, ap_started = 21
        if (conStatus >= IConServiceCommonStates::CONNECTED)
        {
            nLog.info("Wifi service is ready. Starting telnet interface server...");
            stopOldServer();
            auto [tmpServer, error] = this->conService.createServer("tcp://23");
            if (error != Errors::NoError)
            {
                nLog.critical("Error creating server for telnet interface: "+error);
                return;
            }

            server = tmpServer;
            
            this->nLog.info("Telnet interface server started");
        }
    });

    scheduler.periodicTask(50, [=](shr_tmdtsk task){
        work();
    }, "telnet::work", false, TASKS_PRIORITY);

} 
 
TelnetServer::~TelnetServer() 
{ 
    stopOldServer();
} 

void TelnetServer::stopOldServer()
{
    if (server != nullptr)
    {
        server->stop();
        server = nullptr;
        this->nLog.info("Telnet interface server stopped");
    }
} 

void TelnetServer::work()
{
    if (server == nullptr)
        return;

    if (working)
        return;


    working = true;

    picUpkNextClient();

    auto prom = talkToClients();
    prom->then<TpNothing>([=](TpNothing _void){
        this->working = false;
        return Nothing;
    });
}

void TelnetServer::picUpkNextClient()
{
    WiFiClient client = server->accept();
    if (client) {
        nLog.info("TCP client ("+client.remoteIP().toString()+":"+String(client.remotePort())+") connected.");
        auto id = cliIdCount++;
        
        clients[id] = CliInfo();
        clients[id].remoteIP = client.remoteIP();
        clients[id].remotePort = client.remotePort();
        clients[id].theClient = client;
        clients[id].buffer = "";
        clients[id].authenticationState = CliInfo::AuthState::AS_UNAUTHENTICATED;
        clients[id].id = id;
        clients[id].currentlyIsProcessingALine = false;
        clients[id].sendData = [=](String data){
            sendToClient(clients[id], data);
        };


        //discard trash data
        char buffer[10];
        auto availableToBeRead = client.available();
        while (availableToBeRead)
        {
            if (availableToBeRead)
            {
                if (availableToBeRead > 10)
                    availableToBeRead = 10;

                client.readBytes(buffer, availableToBeRead);
            }
            availableToBeRead = client.available();
        }


        cliIds.push_back(id);
        //request password

        nLog.info("The connected client was added to internal vector. Its id is: "+String(id)+". Asking it for the passowrd...");
        setClientEcho(clients[id], false);
        sendToClient(clients[id], "Password: ", false);
    }
}

Promise<TpNothing>::smp_t TelnetServer::talkToClients()
{
    vector<Promise<TpNothing>::smp_t> waitChain;

    for (size_t c = 0; c < cliIds.size(); c++)
    {
        auto cliId = cliIds[c];
        if (clients.count(cliId))
            waitChain.push_back(this->TalkToAClient(clients[cliId]));
        else
            nLog.error("'cliIds' vector and 'clients' map with dvergent data (a 'cliId' is not in 'clients' map)");
    }

    auto tmp = PromiseUtils::WaitPromises(waitChain);
    return tmp;
}

Promise<TpNothing>::smp_t TelnetServer::TalkToAClient(CliInfo &cli)
{
    auto ret = Promise<TpNothing>::getSMP();
    scheduler.run([&, ret]()
    {
        char buffer[128];
        if (cli.theClient.connected())
        {
            auto availableToBeRead = cli.theClient.available();
            if (availableToBeRead)
            {
                if (availableToBeRead > 128)
                    availableToBeRead = 128;

                size_t readed = cli.theClient.readBytes(buffer, availableToBeRead);
                
                for (size_t c = 0; c < readed; c++)
                {
                    
                    if ((uint8_t)buffer[c] == TELNET_IAC)
                    {
                        c += 2;
                        continue;
                    }

                    if (buffer[c] == '\r')
                    {
                        cli.ignoreNextLf = true;
                        processLineReceivedFromClient(cli, cli.buffer);
                        cli.buffer = "";
                    }
                    else if (buffer[c] == '\n')
                    {
                        if (cli.ignoreNextLf)
                        {
                            cli.ignoreNextLf = false;
                            continue;
                        }

                        processLineReceivedFromClient(cli, cli.buffer);
                        cli.buffer = "";
                    }
                    else {
                        if (buffer[c] == 0)
                            continue;

                        cli.buffer += buffer[c];
                    }

                }
            }
        }
        else
            finalizeClient(cli);

        ret->post(Nothing);
    }, "telnet::talkCli::run", TASKS_PRIORITY);

    return ret;
}

void TelnetServer::processLineReceivedFromClient(CliInfo &cli, String receivedLine)
{
    if (cli.currentlyIsProcessingALine)
        return;

    if (receivedLine == "")
    {
        this->sendToClient(cli, TERMINAL_INPUT_INDICATOR, false);
        return;
    }

    cli.currentlyIsProcessingALine = true;

    scheduler.run([&, receivedLine](){
        nLog.info("Received a line from a client(" + String(receivedLine.length()) + " bytes): [" + receivedLine + "]");
        Promise<Error>::smp_t subCmdExecResult = Promise<Error>::get_smp();

        String line = receivedLine;
        // remove optional carriage return when a client sends CR-only.
        if (line.length() > 0 && line[line.length()-1] == '\r')
            line = line.substring(0, line.length()-1);

        
        if (cli.authenticationState != CliInfo::AS_AUTHENTICATED)
        {
            //this condition should never be true, because the 'double' execution of this function is avoided by the 'cli.currentlyIsProcessingALine' flag
            if (cli.authenticationState == CliInfo::AS_AUTHENTICATING)
            {
                this->nLog.warning("Client (id '"+String(cli.id)+"') is already authenticating and sent another line. It could not happens because the code should avoid it");

                this->sendToClient(cli, "You are already authenticating. Wait a moment...", false);
                subCmdExecResult->post(Errors::NoError);
                return;
            }

            cli.authenticationState = CliInfo::AS_AUTHENTICATING;
            

            cli.passwordAttempts++;
            nLog.info("Client (id '"+String(cli.id)+"') is trying to authenticate with password '"+line+"'");
            this->validadtePassword(line)->then([&, subCmdExecResult](bool authenticatedWithSucess){
                if (authenticatedWithSucess)
                {
                    nLog.info("Connected client (id '"+String(cli.id)+"') send the correct password and is, now, authenticated");
                    cli.authenticationState = CliInfo::AuthState::AS_AUTHENTICATED;
                    setClientEcho(cli, true);
                    this->sendToClient(cli, "");
                    sendInitialInfoToClient(cli)->then<TpNothing>([&](TpNothing _void)
                    {
                        
                        //cli.currentlyIsProcessingALine = false;
                        //this->sendToClient(cli, TERMINAL_INPUT_INDICATOR, false);
                        onClientConnected.emit(cli);
                        subCmdExecResult->post(Errors::NoError);
                        return Nothing;
                    });

                }
                else
                {
                    nLog.warning("sent password size: "+String(line.length()));
                    nLog.warning("Conencted client (id '"+String(cli.id)+"') send wrong password ("+line+")");
                    if (cli.passwordAttempts < MAX_PASSWORD_ATTEMPTS)
                    {
                        cli.authenticationState = CliInfo::AS_UNAUTHENTICATED;
                        cli.currentlyIsProcessingALine = false;
                        sendToClient(cli, "Wrong password!\nTry again: ", false);
                        subCmdExecResult->post(Errors::NoError);
                    }
                    else
                    {
                        sendToClient(cli, "You have been disconnected due to reaching the maximum number of password attempts.");
                        this->finalizeClient(cli);
                        nLog.warning("A client (id '"+String(cli.id)+"') was disconnected because reached max password attempts.");
                        subCmdExecResult->post(Errors::NoError);
                    }
                }
            });
        }
        else if (String("exitquit").indexOf(line) == 0)
        {
            finalizeClient(cli);
            subCmdExecResult->post(Errors::NoError);
        }
        else
        {
            //invalid subset
            //this->sendToClient(cli, "Relaying command to all services");

            auto message = line.substring(0, line.indexOf(" "));
            auto data = line.substring(line.indexOf(" ")+1);
            //this->mbus->syncRequest("request "+message, data, message, "", 5000, this->scheduler)->then([=](ResultWithStatus<MBMessage<String>> result){
            auto doneFunc = [&, subCmdExecResult](){
                cli.currentlyIsProcessingALine = false;
                subCmdExecResult->post(Errors::NoError);
            };

            onClientDataReceived.emit(make_tuple(ref(cli), line, doneFunc));
            cli.onDataReceived.emit(make_tuple(line, doneFunc));
        }

        subCmdExecResult->then<TpNothing>([&](Error err){

            if (err != Errors::NoError)
                this->sendToClient(cli, Errors::DerivateError(err, "Error executing command: "));

            cli.currentlyIsProcessingALine = false;
            if (cli.theClient.connected() && cli.authenticationState == CliInfo::AS_AUTHENTICATED)
                this->sendToClient(cli, TERMINAL_INPUT_INDICATOR, false);
            return Nothing;
        });

        
    }, "telnet::procLine::run", TASKS_PRIORITY);
}


void TelnetServer::sendToClient(CliInfo &cli, String message, bool breakLine)
{
    if (!cli.theClient.connected())
    {
        nLog.warning("Trying to send data to disconnected client");
        return;
    }

    if (breakLine)
        cli.theClient.println(message);
    else
        cli.theClient.print(message);
}

Promise<TpNothing>::smp_t TelnetServer::sendInitialInfoToClient(CliInfo &cli)
{
    auto ret = Promise<TpNothing>::get_smp();

    this->sendToClient(cli, InitialBanner);
            
    ret->post(Nothing);

    return ret;
    
}

void TelnetServer::finalizeClient(CliInfo &cli)
{
    cli.messagebus_cmd_observinIds.clear();

    cli.theClient.stop();
    String finalLogMessage = "Client ("+cli.remoteIP.toString()+":"+String(cli.remotePort)+") disconnected.";
    
    for (int c = cliIds.size()-1; c >= 0; c--)
    {
        if(cliIds[c] == cli.id)
            cliIds.erase(cliIds.begin() + c);
    }


    onClientDisconnected.emit(cli);
    
    if (clients.count(cli.id))
        clients.erase(cli.id);


    nLog.info(finalLogMessage);

}

Promise<bool>::smp_t TelnetServer::validadtePassword(String password)
{
    auto ret = Promise<bool>::get_smp();
    scheduler.run([=](){
        storage.pGet("telnet.password", telnetDefaultPassword)->then([=](String savedPassword){
            ret->post(password == savedPassword);
        });
    }, "telnet::validatePassword", TASKS_PRIORITY);

    return ret;
}

void TelnetServer::setClientEcho(CliInfo &cli, bool enabled)
{
    if (!cli.theClient.connected())
        return;

    uint8_t cmd_echo[3] = {
        TELNET_IAC,
        enabled ? TELNET_WONT : TELNET_WILL,
        TELNET_OPT_ECHO
    };

    uint8_t cmd_sga[3] = {
        TELNET_IAC,
        enabled ? TELNET_WONT : TELNET_WILL,
        TELNET_OPT_SUPPRESS_GO_AHEAD
    };

    cli.theClient.write(cmd_echo, 3);
    cli.theClient.write(cmd_sga, 3);
}
