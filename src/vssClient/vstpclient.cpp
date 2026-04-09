#include  "vstpclient.h" 

Error VSTP::VstpErrors::disconected = "Disconnected from server";
Error VSTP::VstpErrors::errorReadingHeaders= "Error reading headers from server";

VSTP::VstpClient::VstpClient(
    Scheduler &scheduler, 
    INetwork &network, 
    ILogger &logService,
    String varsPrefix
): 
    scheduler(scheduler), 
    network(network),
    prefix(varsPrefix)
{
    this->nLog = logService.getNLog("VSTP::VstpClient");
    this->nLog.info("VstpClient service started");
    this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
    
    networkStateListenId = network.conStateChangeEvent.listen([=](IConServiceState conStatus){
        if (conStatus == IConServiceCommonStates::CONNECTED)
        {
            nLog.debug("Will run the connect process");
            //starts the connection monitor. The connection montor will automatically try to reconnect if the connection is lost
        }
    });

    talkToServerTask = this->scheduler.periodicTask(5, [=](shr_tmdtsk task){
        this->talkToServer();
    }, "", false, DEFAULT_PRIORITIES::HIGH_);
} 

VSTP::VstpClient::~VstpClient() 
{ 
    if (networkStateListenId > -1)
        network.conStateChangeEvent.stopListen(networkStateListenId);

    if (talkToServerTask != nullptr)
    {
        talkToServerTask->abort();
        talkToServerTask = nullptr;
    }

    stopConnectionMonitor();
}

Error VSTP::VstpClient::start(String server, int port)
{   
    if (server != "" && port != 0) {
        this->setServerPortAndPrefix(server, port, prefix);
        startConnectionMonitor();
        return Errors::NoError;
    } else {
        auto error = Errors::createError("The VSTP/VSS server address and port are not configured.");
        nLog.error(error);
        return error;
    }
}

void VSTP::VstpClient::stop()
{
    disconnect();
}

void VSTP::VstpClient::talkToServer()
{
    char buffer[128];
    while (true)
    {
        if (!cli.connected())
            break;

        auto availableToBeRead = cli.available();
        if (availableToBeRead)
        {
            if (availableToBeRead > 128)
                availableToBeRead = 128;

            size_t readed = cli.readBytes(buffer, availableToBeRead);
                
            for (size_t c = 0; c < readed; c++)
            {
                if (buffer[c] == '\n')
                {
                    processReceivedLine(incomingBuffer);
                    incomingBuffer = "";
                }
                else
                    incomingBuffer += buffer[c];
            }
        }
        else
            break;
    }
}

void VSTP::VstpClient::processReceivedLine(String line)
{
    scheduler.run([=](){
        this->processVssCommand(line);
    });
}

Promise<Error>::smp_t VSTP::VstpClient::connectToServer(String server, int port)
{
    auto ret = Promise<Error>::get_smp();
    if (connectingInProgress)
    {
        auto error = Errors::createError("The connect process is already running");
        nLog.error(error);
        ret->post(error);
        return ret;
    }

    connectingInProgress = true;

    auto lastConState = this->network.conStateChangeEvent.getLastData();
    if (lastConState != IConServiceCommonStates::CONNECTED && 
        lastConState != WFService::WFServiceState::AP_STARTED
    )
    {
        auto error = Errors::createError(
            "cannot connec to vss server, because networks is not connect as client to a ssid or is not ready (curr wifi state is '"
            +this->network.stateToString(lastConState)
            +"')"
        );

        nLog.error(error);

        connectingInProgress = false;
        ret->post(error);
        return ret;
    }

    finalizePreviousConnection()->then([=](Error result){
        if(result != Errors::NoError)
        {
            auto error = Errors::DerivateError(result, "error finalizing previous connection to begin a new one: "+result);
            nLog.error(error);
            
            connectingInProgress = false;
            ret->post(error);
            return;
        }

        this->onConnectionStateChange.stream(VstpClientState::CONNECTING);

        if (server == "")
        {
            auto error = Errors::createError("The VSTP/VSS server address is not configured.");
            nLog.error(error);
            //do not try a new connection
            
            connectingInProgress = false;
            ret->post(error);
            this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
            return;
        }

        //connect to the server
        this->connectToTheTCPServer(server, port)->then([=](Error result2){
            if(result2 != Errors::NoError)
            {
                auto error = Errors::DerivateError(result2, "error connecting to the server: "+result2);
                nLog.error(error);
                connectingInProgress = false;
                ret->post(error);
                this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
                return;
            }

            //exchange headers
            this->readHeaders()->then([=](Error result3){
                if(result3 != Errors::NoError)
                {
                    auto error = Errors::DerivateError(result3, "error reading headers: "+result3);
                    nLog.error(error);
                    
                    connectingInProgress = false;
                    ret->post(error);   
                    this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
                    return;
                }

                //exchange ids
                this->exchangeIds()->then([=](Error result4){
                    if(result4 != Errors::NoError)
                    {
                        auto error = Errors::DerivateError(result4, "error exchanging IDs: "+result4);
                        nLog.error(error);
                        
                        connectingInProgress = false;
                        ret->post(error);
                        this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
                        return;
                    }

                    nLog.info("Connected to the VSS Server!");
                    connectingInProgress = false;
                    this->onConnectionStateChange.stream(VstpClientState::CONNECTED);


                    //scroll all observing and check for those that are not sent to the server
                    for (auto &obs: this->observings)
                    {
                        if (!obs.second.sentToServer)
                        {
                            nLog.debug("Sending observation pack to the server for var '"+obs.second.varName+"'");
                            sendToServer(VstpCmdAndPayload{.cmd = ACTIONS_SUBSCRIBE_VAR, .payload = obs.second.varName + "("+String(obs.second.id)+")"});
                            this->observings[obs.first].sentToServer = true;
                        }
                    }

                    ret->post(Errors::NoError);

                    
                    startConnectionMonitor();
                });
            });
        });
    });

    return ret;
}

void VSTP::VstpClient::disconnect()
{
    stopConnectionMonitor();
    finalizePreviousConnection();
}

void VSTP::VstpClient::startConnectionMonitor()
{
    //check if connection monitor is already running
    if (this->monitorTask != nullptr){
        return;
    }

    this->monitorTask = this->scheduler.periodicTask(5000, [=](shr_tmdtsk taskControl){
        if (!cli.connected())
        {
            taskControl->pause();

            bool firstTime = taskControl->taskAge < taskControl->period*2;
            if (!firstTime)
                nLog.warning("Connection was lost. Reconnecting ...");

            this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
            scheduler.run([=](){
                this->connectToServer(this->serverAddr, this->serverPort)->then([=](Error error){
                    if (error != Errors::NoError) {
                        error = Errors::DerivateError(error, "error reconnecting (retrying in 5 seconds): "+error);
                        nLog.error(error);

                    } else {
                        nLog.info("Connection reestablished");
                    }
                    taskControl->resume();
                });
            });
        }
    }, "", DEFAULT_PRIORITIES::LOW_);
}

void VSTP::VstpClient::stopConnectionMonitor()
{
    if (this->monitorTask != nullptr){
        this->monitorTask->abort();
        this->monitorTask = nullptr;
    }
}

Promise<Error>::smp_t VSTP::VstpClient::finalizePreviousConnection()
{
    auto ret = Promise<Error>::get_smp();
    scheduler.run([=](){
        this->cli.stop();
        this->onConnectionStateChange.stream(VstpClientState::DISCONNECTED);
        ret->post(Errors::NoError);
    });
    return ret;
}

Promise<Error>::smp_t VSTP::VstpClient::connectToTheTCPServer(String server, int port)
{
    auto ret = Promise<Error>::get_smp();
    scheduler.run([=](){
        nLog.info("Connection to '"+server+"' on port '"+port+"'... ", false);
        this->cli.connect(server, port);
        if (this->cli.connected())
        {
            nLog.info("connected");
            ret->post(Errors::NoError);
        }
        else
        {
            nLog.info(" error");
            ret->post(Errors::createError("Cannot connect to the server '"+server+"' on port '"+port+"'"));
        }
    });

    return ret;
}

Promise<Error>::smp_t VSTP::VstpClient::readHeaders()
{
    auto ret = Promise<Error>::get_smp();

    serverHeaders.clear();
    this->getCommandsUntil([=](VstpCmdAndPayload checkIfIsTimeToStop){
        return checkIfIsTimeToStop.cmd == ACTIONS_SERVER_END_HEADERS;
    })->then<TpNone>([=](ResultWithStatus<vector<VstpCmdAndPayload>> result){
        if (result.status == Errors::NoError)
        {
            for (auto &c: result.result)
            {
                if (c.cmd != ACTIONS_SERVER_BEGIN_HEADERS)
                {
                    this->serverHeaders.push_back(c.cmd + ":" +c.payload);
                }
            }

            ret->post(Errors::NoError);
        }
        else
            ret->post(Errors::DerivateError(result.status, VstpErrors::errorReadingHeaders));


        return None;
    });

    return ret;
}

Promise<Error>::smp_t VSTP::VstpClient::exchangeIds()
{
    auto ret = Promise<Error>::get_smp();
    scheduler.run([=](){
        auto suggestedId = this->getSugestedId();

        nLog.debug("Suggested ID by the server: '"+suggestedId+"'");

        if (currentId == "")
        {
            if (suggestedId == "")
            {
                auto error = Errors::createError("The ID suggested by the server could not be obtained and no previous one was used. System canno continue without an ID.");
                nLog.error(error);
                ret->post(error);
                return;
            }

            nLog.debug("Using the ID suggested by the server ("+suggestedId+")");
            this->currentId = suggestedId;
        }
        else
            nLog.debug("A previous used ID will be used ("+this->currentId+")");

        this->sendToServer(VstpCmdAndPayload{
            .cmd = ACTIONS_CHANGE_OR_CONFIRM_CLI_ID,
            .payload = this->currentId
        });

        ret->post(Errors::NoError);
    });
    return ret;
}

void VSTP::VstpClient::processVssCommand(String receivedLine)
{
    auto parts = StringUtils::split(receivedLine, ":");
    if (parts.size() > 0)
    {
    
        processVssCommand(
            parts[0],
            (parts.size() > 1) ? parts[1] : ""
        );
    }
}

void VSTP::VstpClient::processVssCommand(String command, String data)
{
    auto cmdAndPayload = VstpCmdAndPayload{.cmd = command, .payload = data};
    //nLog.debug("Received VSTP command '"+command+"' with data '"+data+"'");
    //dispatch the data for internal observers
    this->onCommandReceived.stream(cmdAndPayload);

    //processing commands
    if (command == ACTIONS_VAR_CHANGED)
        this->handle_var_change_cmd(cmdAndPayload);

}

void VSTP::VstpClient::handle_var_change_cmd(VSTP::VstpCmdAndPayload cmdAndPayload)
{
    if (auto pos = cmdAndPayload.payload.indexOf('='); pos > -1)
    {
        String VarNameWithMetadata = cmdAndPayload.payload.substring(0, pos);
        String Value = cmdAndPayload.payload.substring(pos+1);

        auto [varName, internalId] = separateNameAndMetadata(VarNameWithMetadata);

        auto internalIdI = std::strtoul(internalId.c_str(), nullptr, 10);

        if (internalIdI == 0 || internalIdI == ULONG_MAX)
        {
            nLog.error("Error parsing metadata (try to getting the internal observation id, that is a int ) received from server in a varchange notification. Received value was '"+cmdAndPayload.payload+"'");
            return;
        }
        
        //nLog.debug("Received a var notification: '"+cmdAndPayload.payload+"'");
        if (this->observings.count(internalIdI))
        {
            //nLog.debug("Observer (id '"+String(internalIdI)+"') found");

            if (prefix != "" && varName.indexOf(prefix) == 0)
                varName = varName.substring(prefix.length());

            this->observings[internalIdI].f(VssVar(varName, Value));
        }
        else
        {
            nLog.error("Observer (id '"+String(internalIdI)+"') was not foundFound");
        }
    }
    else
    {
        nLog.error("Received invalida var=value pair from server in a var notification. Received value was '"+cmdAndPayload.payload+"'");
    }
}   

String VSTP::VstpClient::getSugestedId()
{
    auto found = getHeaders(ACTIONS_SUGGEST_NEW_CLI_ID + ":");
    if (found.size() > 0)
        return found[0].substring(found[0].indexOf(":\"")+1);
    return "";
}

vector<String> VSTP::VstpClient::getHeaders(function<bool(String)> isValid)
{
    vector<String> ret;
    for (auto &c: this->serverHeaders)
    {
        if (isValid(c))
            ret.push_back(c);
    }

    return ret;
}

void VSTP::VstpClient::sendToServer(String data, bool sendLineBreakToo)
{
    if (sendLineBreakToo)
        data += "\n";
    sendBytesToServer(data.c_str(), (size_t)data.length());
}

void VSTP::VstpClient::sendBytesToServer(const char* buffer, size_t size)
{
    this->cli.write(buffer, size);
}

Promise<Error>::smp_t VSTP::VstpClient::setVar(String varName, String value)
{
    if (prefix != "")
        varName = prefix + varName;

    auto ret = Promise<Error>::get_smp();
    scheduler.run([=](){
        if (onConnectionStateChange.getCurrentState() == VstpClientState::CONNECTED)
        {
            this->sendToServer(ACTIONS_SET_VAR, varName + "=" + value);
            ret->post(Errors::NoError);
        }
        else
            ret->post(VstpErrors::disconected);
    });

    return ret;
}

Promise<ResultWithStatus<vector<VSTP::VssVar>>>::smp_t VSTP::VstpClient::getVar(String varName)
{
    if (prefix != "")
        varName = prefix + varName;

    auto ret = Promise<ResultWithStatus<vector<VssVar>>>::get_smp();
    auto responseOk = std::make_shared<bool>(false);

    scheduler.run([=](){
        this->sendToServer(ACTIONS_GET_VAR, varName);

        this->getCommandsUntil([=](VstpCmdAndPayload checkStop){
            if (checkStop.cmd == ACTIONS_RESPONSE_END){
                if (checkStop.payload.indexOf(ACTIONS_GET_VAR) > -1 && checkStop.payload.indexOf(varName) > -1)
                {
                    *responseOk = true;
                    return true;
                }
            }
            return false;
        })->then<TpNone>([=](ResultWithStatus<vector<VstpCmdAndPayload>> retData){
            if (retData.status == Errors::NoError)
            {
                vector<VssVar> vars;
                for (auto &curr: retData.result)
                {
                    if (curr.cmd == ACTIONS_GET_VAR_RESPONSE)
                    {
                        auto sepPos = curr.payload.indexOf('=');
                        if (sepPos == -1)
                            sepPos = curr.payload.indexOf(':');
                        if (sepPos == -1)
                            sepPos = curr.payload.indexOf(';');
                        if (sepPos == -1)
                            sepPos = curr.payload.indexOf(' ');
                        
                        if (sepPos > -1){
                            auto currVarName = curr.payload.substring(0, sepPos);
                            auto currVarValue = curr.payload.substring(sepPos + 1);
                            vars.push_back(VssVar(currVarName, currVarValue));
                        }
                        else {
                            vars.push_back(VssVar(varName, curr.payload));
                        }
                    }
                }

                ret->post(ResultWithStatus<vector<VssVar>>(vars, Errors::NoError));
            }
            else{
                ret->post(ResultWithStatus<vector<VssVar>>({}, Errors::DerivateError(retData.status, "error getting variables from server: "+retData.status)));
            }
            return None;
        });
    });
    return ret;
}

Promise<Error>::smp_t VSTP::VstpClient::deleteVar(String varName)
{
    if (prefix != "")
        varName = prefix + varName;

    auto ret = Promise<Error>::get_smp();

    scheduler.run([=](){
        if (onConnectionStateChange.getCurrentState() == VstpClientState::CONNECTED)
        {
            this->sendToServer(ACTIONS_DELETE_VAR, varName);
            ret->post(Errors::NoError);
        }
        else
            ret->post(VstpErrors::disconected);
    });

    return ret;
}

Promise<uint>::smp_t VSTP::VstpClient::listenVar(String varName, function<void(VSTP::VssVar var)> f)
{
    if (prefix != "")
        varName = prefix + varName;
        
    auto ret = Promise<uint>::get_smp();
    scheduler.run([=](){
        auto uniqueId = observerIdCount++;
        this->observings[uniqueId] = VarListenInfo{.id = uniqueId, .varName = varName, .f = f, .sentToServer = false};
        nLog.debug("Sending observation pack to the server");
        if (onConnectionStateChange.getCurrentState() == VstpClientState::CONNECTED){
            sendToServer(VstpCmdAndPayload{.cmd = ACTIONS_SUBSCRIBE_VAR, .payload = varName + "("+String(uniqueId)+")"});
            this->observings[uniqueId].sentToServer = true;
        }

        ret->post(uniqueId);
    });

    return ret;

}

Promise<TpNothing>::smp_t VSTP::VstpClient::stopListenVar(uint listenId)
{
    auto ret = Promise<TpNothing>::get_smp();
    scheduler.run([=](){
        if (this->observings.count(listenId))
        {
            auto listenInfo = this->observings[listenId];
            sendToServer(VstpCmdAndPayload{.cmd = ACTIONS_UNSUBSCRIBE_VAR, .payload = listenInfo.varName + "("+String(listenId)+")"});

            this->observings.erase(listenId);

            ret->post(None);
        }
        else
            ret->post(None);
    });

    return ret;
}

tuple<String, String> VSTP::VstpClient::separateNameAndMetadata(String originalVarName)
{
    if (auto pos = originalVarName.indexOf('('); pos > -1)
    {
        auto varname = originalVarName.substring(0, pos);
        auto metadata = originalVarName.substring(pos + 1);
        if (metadata.length() > 0 && metadata[metadata.length()-1] == ')')
            metadata = metadata.substring(0, metadata.length()-1);

        return { varname, metadata };
    }
    else
        return {originalVarName, ""};
    
}

Promise<ResultWithStatus<vector<VSTP::VstpCmdAndPayload>>>::smp_t VSTP::VstpClient::getCommandsUntil(function<bool(VstpCmdAndPayload)> isToStop, int timeout)
{
    auto ret = Promise<ResultWithStatus<vector<VstpCmdAndPayload>>>::get_smp();
    auto observerId = std::make_shared<int>(0);
    auto success = std::make_shared<bool>(false);
    auto tmpCache = std::make_shared<vector<VstpCmdAndPayload>>();


    *observerId = this->onCommandReceived.listen([=](VstpCmdAndPayload received){
        if (*success)
            return;

        if (isToStop(received))
        {
            *success = true;

            ret->post(ResultWithStatus<vector<VstpCmdAndPayload>>(*tmpCache, Errors::NoError));
            this->onCommandReceived.stopListening(*observerId);
        }
        else
            tmpCache->push_back(received);
    });

    scheduler.delayedTask(timeout, [=](){
        if (*success)
            return;

        *success = true;
        ret->post(ResultWithStatus<vector<VstpCmdAndPayload>>(*tmpCache, Errors::TimeoutError));
        this->onCommandReceived.stopListening(*observerId);
    });

    return ret;
}

void VSTP::VstpClient::setServerPortAndPrefix(String server, int port, String)
{
    this->serverAddr = server;
    this->serverPort = port;
    this->prefix = prefix;
}