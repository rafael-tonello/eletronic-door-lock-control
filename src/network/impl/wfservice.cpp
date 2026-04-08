#include "wfservice.h"

Promise<Error>::type WFService::autoConnectOrCreateAp(){
    auto ret = Promise<Error>::get_smp("WFService::AutoConnect");

    this->conStateChangeEvent.stream(IConServiceCommonStates::DISCONNECTED);
    scheduler.delayedTask(1000, [=](){
        connectProcess();
        ret->post(Errors::NoError);
        
    }, "wifi::init::showAvailableWifiNetworks");

    //handleEventBusOperations();
    return ret;
}

void WFService::connectProcess(){
    stopMonitorCurrentConnection();

    //setHostname
    WiFi.setHostname(this->hostName.c_str());

    Serial.println("--------------====================");

    this->conStateChangeEvent.stream(IConServiceCommonStates::CONNECTING);
    getLastUsedNetwork()->then([=](NetworkInfo lastNetwork){
        Serial.println("----------------------------------------------------------------");
        Serial.println("Trying to connect to the last network: "+lastNetwork.ssid);


        connectToNetwork(lastNetwork)->then([=](Error error){
            if (error != Errors::NoError)
            {
                error = Errors::DerivateError(error, "Error connecting to the last network");
                nLog.error(error);

                nLog.info("Trying to connect to another network");
                getRegisteredNetworks()->then([=](auto networks){
                    vector<SavedNetworkInfo> *networksP = new vector<SavedNetworkInfo>();
                    *networksP = networks;
                    size_t *index = new size_t(0);

                    function<void()> *tryNextNetwork = new function<void()>();
                    *tryNextNetwork = [=](){
                        Serial.println("Index = "+String(*index) + " size = "+String(networksP->size()));
                        if ((*index) < networksP->size())
                        {
                            connectToNetwork((*networksP)[*index].networkInfo)->then([=](Error error2){
                                if (error2 == Errors::NoError)
                                {
                                    startMonitoringCurrentConnection();
                                    scheduler.run([=](){
                                        networksP->clear();
                                        delete networksP;
                                        delete index;
                                        delete tryNextNetwork;
                                    }, "wifi::connectProcess::1");
                                }
                                else
                                {
                                    (*index)++;

                                    scheduler.run([=](){
                                        (*tryNextNetwork)();
                                    }, "wifi::connectProcess::2");
                                }
                            });
                        }
                        else
                        {
                            nLog.info("No network to connect. Starting AP");
                            //no connetion. Start AP
                            this->conStateChangeEvent.stream(WFServiceState::STARTING_AP);
                            this->startAccessPoint()->then([=](TpNothing _void){
                                this->conStateChangeEvent.stream(WFServiceState::AP_STARTED);
//
                                scheduler.run([=](){
                                    networksP->clear();
                                    delete networksP;
                                    delete index;
                                    delete tryNextNetwork;
                                }, "wifi::connectProcess::3");
                            });
                        }
                    };
                    scheduler.run([=](){
                        (*tryNextNetwork)();
                    }, "wifi::connectProcess::4");
                });
            }
            else{
                startMonitoringCurrentConnection();
                nLog.debug("Connected to the last network with sucess");
            }
        });
    });
}

Promise<NetworkInfo>::smp_t WFService::getLastUsedNetwork(){
    auto ret = Promise<NetworkInfo>::get_smp();
    nLog.debug("Getting last used network");
    storage.pGet("wifi.current", "")->then([=](auto result){
        nLog.debug("Last used network: "+result);
        ret->post(NetworkInfo{
            .ssid = result
        });
    });

    return ret;
}

/*not pure*/
Promise<Error>::smp_t WFService::connectToNetwork(String ssid, String password, bool registerNetworkIfSucess)
{
    auto ret = Promise<Error>::get_smp();

    nLog.info("Connecting to network '"+ssid+"' with password '"+password+"'... ");

    auto returnWithError = [=](Error error){
        nLog.error("... error: "+error);
        ret->post(error);
    };

    if (ssid != "")
    {
        scheduler.periodicTask(500 MiliSeconds, [=](shr_tmdtsk tsk){
            if (tsk->taskAge <= 15000)
            {
                auto wfStatus = WiFi.status();
                //nLog.debug("... wifi status: "+wlStateToString(wfStatus));
                switch (wfStatus){
                    case WL_CONNECTED:
                        nLog.info("... wifi connected with sucess");
                        this->connectedSsid = ssid;
                        this->displayAddressInfo(getWifiCliAddrInfo());
                        this->conStateChangeEvent.stream(IConServiceCommonStates::CONNECTED);

                        this->scheduler.run([=](){
                            if (registerNetworkIfSucess)
                            {
                                nLog.info("Registering network '"+ssid+"' as a known network");
                                this->registerKnownNetwork(ssid, password)->then([=](TpNothing noneResult){
                                    
                                    nLog.info("Network '"+ssid+"' registered with sucess");
                                    ret->post(Errors::NoError);

                                    return None;
                                });
                            } else {
                                //evtbus this->messageBus->post("wifi.connected", this->connectedSsid);
                                ret->post(Errors::NoError);
                            }

                        }, "wifi::connectToNetwork::postConnectedEvent", DEFAULT_PRIORITIES::LOW_);

                        tsk->abort();
                    break;
                    case WL_CONNECT_FAILED:
                        tsk->abort();
                        nLog.error("... wifi connection failed");
                        returnWithError(Errors::createError("Connecting attempt failed"));
                        tsk->abort();
                    break;
                    case WL_NO_SSID_AVAIL:
                        tsk->abort();
                        nLog.error("... wifi connection failed. SSID '"+ssid+"' is not available");
                        returnWithError(Errors::createError("SSID '"+ssid+"' is not available"));
                        tsk->abort();
                    //case WL_WRONG_PASSWORD:
                    //    tsk->abort();
                    //    nLog.error("... wifi connection failed. Wrong password");
                    //    returnWithError(Errors::createError("Wrong password"));
                    //    tsk->abort();
                    break;
                    default:
                        // Handle other unhandled WiFi states (WL_NO_SHIELD, WL_IDLE_STATUS, etc.)
                        nLog.debug("... wifi status: "+wlStateToString(wfStatus));
                    break;
                }
            }
            else {
                nLog.error("... wifi connection failed. Timeout reached");
                returnWithError(Errors::TimeoutError);
                tsk->abort();
            }
        //});
        }, "wifi::connectToNetwork::periodicTask");
        
        scheduler.run([=](){
            WiFi.mode(WIFI_STA);
        }, "wifi::connectToNetwork::setMode");

        scheduler.delayedTask(100, [=](){
            WiFi.begin(ssid.c_str(), password.c_str());
        }, "wifi::connectToNetwork::begin");
    }
    else
        returnWithError(Errors::createError("Ssid is empty"));

    return ret;
}

Promise<Error>::smp_t WFService::connectToNetwork(NetworkInfo network, bool registerNetworkIfSucess)
{
    nLog.info("Getting network '"+network.ssid+"' information from storage");
    auto ret = Promise<Error>::get_smp();
    this->getNetworkdPassword(network)->then([=](ResultWithStatus<String> result){
        if (result.status != Errors::NoError)
        {
            ret->post(Errors::DerivateError(result.status, "Error recovering network password"));
            return;
        }

        this->connectToNetwork(network.ssid, result.result, registerNetworkIfSucess)->then([=](Error connectResult){
            ret->post(connectResult);
        });
    });

    return ret;
}

Promise<ResultWithStatus<String>>::smp_t WFService::getNetworkdPassword(NetworkInfo network)
{
    auto ret = Promise<ResultWithStatus<String>>::get_smp();
    auto tmp = SR("wifi.knownssids.?.password", {network.ssid});
    storage.pGet(tmp, "")->then([=](auto result){
        auto finalResult = ResultWithStatus<String>(result, Errors::NoError);
        if (result == "")
            finalResult.status = Errors::NotFound;

        ret->post(finalResult);
    });

    return ret;
}

Promise<TpNothing>::smp_t WFService::startAccessPoint()
{
    auto ret = Promise<TpNothing>::get_smp();

        scheduler.run([=](){
            nLog.info("Preparing radio for AP mode");
            WiFi.enableSTA(false);
            WiFi.mode(WIFI_AP);
            WiFi.persistent(false);
        }, "wifi::startAccessPoint::prepareRadioForAPMode");
    
        scheduler.run([=](){
            auto [apssid, appassword] = getAPSsidAndPassword();
            nLog.info("Starting up the Access Point '"+apssid+"' with password '"+appassword+"'");
            WiFi.softAP(apssid.c_str(), appassword.c_str(), 8, 0, 10);
        }, "wifi::startAccessPoint::startAP");

        scheduler.run([=](){
            this->displayAddressInfo(getWifiApAddrInfo());
            ret->post(Nothing);
        }, "wifi::startAccessPoint::displayAddressInfo");
    return ret;
}

Promise<vector<NetworkInfo>>::smp_t WFService::getAvailableNetworks()
{
    auto ret = Promise<vector<NetworkInfo>>::get_smp();
    
    this->scheduler.run([=](){

         auto numberOfNetworks = WiFi.scanNetworks(false, true);

         this->scheduler.run([=](){
             vector<NetworkInfo> result;
             for (int c  = 0; c < numberOfNetworks; c++)
             {
                 result.push_back(NetworkInfo{
                     .ssid = WiFi.SSID(c),
                     .rssid_dBm = WiFi.RSSI(c)
                 });
             }

             ret->post(result);
         }, "wifi::getAvailableNetworks::processScanResult");
    }, "wifi::getAvailableNetworks::run");

    return ret;
}

void WFService::stopMonitorCurrentConnection()
{
    if (this->monitoringConnectionTask != nullptr)
        this->monitoringConnectionTask->abort();
}

void WFService::startMonitoringCurrentConnection()
{
    this->monitoringConnectionTask = scheduler.periodicTask(1000 MiliSeconds, [=](shr_tmdtsk tsk){
        if (WiFi.status() != WL_CONNECTED)
        {
            this->conStateChangeEvent.stream(IConServiceCommonStates::DISCONNECTED);
            nLog.info("Connection lost. Trying to reconnect");
            connectProcess();
            tsk->abort();
        }
    });
}

WFService::WifiConInfo WFService::getWifiCliAddrInfo()
{
    WifiConInfo ret;
    ret.ip = String(WiFi.localIP().toString().c_str());
    ret.gatewayIp = String(WiFi.gatewayIP().toString().c_str());
    ret.broadcastIp = "";
    
    ret.macAddress = WiFi.macAddress();
    
    ret.networkInfo = NetworkInfo{
        .ssid = WiFi.SSID(),
        .rssid_dBm = WiFi.RSSI()
    };
    
    if (ret.gatewayIp.indexOf('.') > -1){
        ret.broadcastIp = ret.gatewayIp.substring(0, ret.gatewayIp.lastIndexOf('.')) + ".255";
    }

    return ret;
}

WFService::WifiConInfo WFService::getWifiApAddrInfo()
{
    WifiConInfo ret;
    ret.ip = String(WiFi.softAPIP().toString().c_str());
    ret.gatewayIp = String(WiFi.softAPIP().toString().c_str());
    ret.broadcastIp = "";

    ret.macAddress = WiFi.softAPmacAddress();

    ret.networkInfo = NetworkInfo{
        .ssid = WiFi.softAPSSID(),
        .rssid_dBm = 0
    };

    if (ret.gatewayIp.indexOf('.') > -1)
        ret.broadcastIp = ret.gatewayIp.substring(0, ret.gatewayIp.lastIndexOf('.')) + ".255";

    return ret;

}

void WFService::displayAddressInfo(WifiConInfo info)
{
    String infoStr = "Addresses infomation:\n";
    infoStr += "  Device ip: " + String(info.ip.c_str()) + "\n";
    infoStr += "  Gateway ip: " + String(info.gatewayIp.c_str()) + "\n";
    infoStr += "  Broadcast ip: " + String(info.broadcastIp.c_str()) + "\n";

    nLog.info(infoStr);

}

/**
 * @brief Create a new Ssid to be used as the AP name. The password will be allwais '12345678'
 * 
 * @return tuple<String, String> A tuple with the format {ssid_name, '12345678'}
 */
tuple<String, String> WFService::getAPSsidAndPassword()
{
    /**
     * TERLA: A RANDOM GENERATED NAME
     * NC: NOT CONFIGURED
     * [ID]: ESP UNIQUE DEVICE ID
     */

    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8) {
        //check is in a esp32
        #if defined(ESP32)
            chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
        #else
            chipId = ESP.getChipId();
        #endif
    }

    return {"TERLA_NC_"+String(chipId), "12345678"};
}

tuple<shared_ptr<Client>, Error> WFService::createClient(String destination)
{
    if (destination == "")
        return {shared_ptr<Client>(new WiFiClient()), Errors::NoError};
    //get text until '://'
    auto protocol = destination.substring(0, destination.indexOf("://"));
    destination = destination.substring(destination.indexOf("://")+3);

    if (destination != "tcp")
        return {nullptr, Errors::createError("invalid protocol. Currently only 'tcp' is supported")};

    //get the host (until ':')
    auto host = destination.substring(0, destination.indexOf(":"));
    //get the port (after ':')
    auto port = destination.substring(destination.indexOf(":")+1).toInt();

    auto theClient = shared_ptr<Client>(new WiFiClient());

    theClient->connect(host.c_str(), port);

    return {theClient, Errors::NoError};


}


//create a server. Origin is used to specify the port to be listened:
// for a tcp server tcp://port
// for a udp server udp://port
//if origin is empty, you will need to call the server->begin() method and init the server by yourself. In this case, an error will also be returned
tuple<shared_ptr<WiFiServer>, Error> WFService::createServer(String origin)
{
    if (origin == "")
        return make_tuple<shared_ptr<WiFiServer>, Error>(nullptr, Errors::createError("invalid origin. Origin cannot be empty"));
        
    //get text until '://'
    auto protocol = origin.substring(0, origin.indexOf("://"));
    origin = origin.substring(origin.indexOf("://")+3);

    if (protocol != "tcp")
        return make_tuple<shared_ptr<WiFiServer>, Error>(nullptr, Errors::createError("invalid protocol. Currently only 'tcp' is supported"));

    auto port = origin.toInt();

    shared_ptr<WiFiServer> result = make_shared<WiFiServer>(port);
    result->begin();

    return make_tuple(result, Errors::NoError);
}

String WFService::stateToString(IConServiceState state)
{
    switch (state)
    {
        case WFServiceState::BEGIN:
            return "BEGIN";
        break;
        case IConServiceCommonStates::DISCONNECTED:
            return "DISCONNECTED";

        break;
        case IConServiceCommonStates::CONNECTING:
            return "CONNECTING";

        break;
        case IConServiceCommonStates::CONNECTED:
            return "CONNECTED";

        break;
        case WFServiceState::STARTING_AP:
            return "STARTING_AP";
        break;
        case WFServiceState::AP_STARTED:
            return "AP_STARTED";
        break;
    }

    return "UNKNOWN";
}

String WFService::wlStateToString(wl_status_t status)
{
    switch (status)
    {
        case WL_NO_SHIELD:
            return "WL_NO_SHIELD";
        break;
        case WL_IDLE_STATUS:
            return "WL_IDLE_STATUS";
        break;
        case WL_NO_SSID_AVAIL:
            return "WL_NO_SSID_AVAIL";
        break;
        case WL_SCAN_COMPLETED:
            return "WL_SCAN_COMPLETED";
        break;
        case WL_CONNECTED:
            return "WL_CONNECTED";
        break;
        case WL_CONNECT_FAILED:
            return "WL_CONNECT_FAILED";
        break;
        case WL_CONNECTION_LOST:
            return "WL_CONNECTION_LOST";
        break;
        case WL_WRONG_PASSWORD:
            return "WL_WRONG_PASSWORD";
        break;
        case WL_DISCONNECTED:
            return "WL_DISCONNECTED";
        break;
    }

    return "UNKNOWN";
}

Promise<TpNothing>::smp_t WFService::registerKnownNetwork(String ssid, String password)
{
    auto ret = Promise<TpNothing>::get_smp();
    //check if the network is already registered
    this->storage.pGet(SR("wifi.knownssids.?.index", {ssid}), "")->then([=](auto index){
        if (index != "")
        {
            this->storage.pSet(SR("wifi.knownssids.?.password", {ssid}), password)->then([=](){
                ret->post(Nothing);
            });
            return;
        }
        
        auto count = this->storage.get("wifi.knownssids.index.count", "0");
        this->storage.set("wifi.knownssids.index.count", String(count.toInt()+1));
        
        auto prom1 = this->storage.pSet(SR("wifi.knownssids.index.?", {count}), ssid);
        auto prom2 = this->storage.pSet(SR("wifi.knownssids.?.password", {ssid}), password);
        auto prom3 = this->storage.pSet(SR("wifi.knownssids.?.index", {ssid}), count);

        PromiseUtils::WaitPromises({prom1, prom2, prom3})->then([=](){
            ret->post(Nothing);
        });
    });

    return ret;
}

Promise<vector<SavedNetworkInfo>>::smp_t WFService::getRegisteredNetworks(bool onlyCurrentAvailableNetworks)
{
    auto ret = Promise<vector<SavedNetworkInfo>>::get_smp();
    
    this->getAvailableNetworks()->then([=](vector<NetworkInfo> availableNetowrks){
        
        vector<SavedNetworkInfo> result;

        auto count = this->storage.get("wifi.knownssids.index.count", "0");
        for (int c = 0; c < count.toInt(); c++)
        {
            auto ssid = this->storage.get(SR("wifi.knownssids.index.?", {String(c)}), "");
            auto password = this->storage.get(SR("wifi.knownssids.?.password", {ssid}), "");

            //find network in the available networks
            if (onlyCurrentAvailableNetworks){
                bool found = false;
                NetworkInfo foundNetInfo;
                for (auto &currANet: availableNetowrks)
                {
                    if (currANet.ssid == ssid)
                    {
                        found = true;
                        foundNetInfo = currANet;
                        break;
                    }
                }

                if (!found && !onlyCurrentAvailableNetworks)
                    foundNetInfo = { .ssid = ssid, .rssid_dBm = -100 };

                result.push_back(SavedNetworkInfo
                {
                    .networkInfo = foundNetInfo,
                    .password = password,
                    .index = c
                });
                
            }
        }

        ret->post(result);
    });

    return ret;
}

Promise<Error>::smp_t WFService::deleteRegisteredNetwork(int index)
{
    auto ret = Promise<Error>::get_smp();

    auto count = this->storage.get("wifi.knownssids.index.count", "0");

    if (index >= count.toInt() || index < 0)
    {
        ret->post(Errors::createError("Index out of range"));
        return ret;
    }

    auto ssid = this->storage.get(SR("wifi.knownssids.index.?", {String(index)}), "");
    if (ssid == "")
    {
        ret->post(Errors::NotFound);
        return ret;
    }

    for (int c = index; c < count.toInt()-1; c++)
    {
        auto nextSsid = this->storage.get(SR("wifi.knownssids.index.?", {String(c+1)}), "");
        //update idex list
        this->storage.set(SR("wifi.knownssids.index.?", {String(c)}), nextSsid);

        //update the ssid record
        this->storage.set(SR("wifi.knownssids.?.index", {nextSsid}), String(c));
    }
    
    //remove the last ssid record
    this->storage.remove(SR("wifi.knownssids.index.?", {String(count.toInt()-1)}));
    this->storage.remove(SR("wifi.knownssids.?.index", {ssid}));
    this->storage.remove(SR("wifi.knownssids.?.password", {ssid}));
    this->storage.set("wifi.knownssids.index.count", String(count.toInt()-1));
    ret->post(Errors::NoError);
    return ret;
}

Promise<ResultWithStatus<int>>::smp_t WFService::getRegisteredNetworkIndex(String indexOrSsid)
{
    auto ret = Promise<ResultWithStatus<int>>::get_smp();

    bool isNumber = true;
    for (size_t i = 0; i < indexOrSsid.length(); i++)
    {
        if (!isDigit(indexOrSsid[i]))
        {
            isNumber = false;
            break;
        }
    }

    if (indexOrSsid == "")
        isNumber = false;

    if (isNumber)
    {
        ret->post(ResultWithStatus<int>(indexOrSsid.toInt(), Errors::NoError));
        return ret;
    }

    auto idx = storage.get(SR("wifi.knownssids.?.index", {indexOrSsid}), "");
    if (idx == "")
    {
        ret->post(ResultWithStatus<int>(-1, Errors::NotFound));
        return ret;
    }

    ret->post(ResultWithStatus<int>(idx.toInt(), Errors::NoError));
    return ret;
}

Promise<Error>::smp_t WFService::deleteRegisteredNetwork(String indexOrSsid)
{
    auto ret = Promise<Error>::get_smp();
    getRegisteredNetworkIndex(indexOrSsid)->then([=](ResultWithStatus<int> idxResult){
        if (idxResult.status != Errors::NoError)
        {
            ret->post(Errors::NotFound);
            return;
        }

        deleteRegisteredNetwork(idxResult.result)->then([=](Error err){
            ret->post(err);
        });
    });

    return ret;
}

Promise<Error>::smp_t WFService::deleteAllRegisteredNetworks()
{
    auto ret = Promise<Error>::get_smp();

    auto count = storage.get("wifi.knownssids.index.count", "0").toInt();
    for (int i = 0; i < count; i++)
    {
        auto ssid = storage.get(SR("wifi.knownssids.index.?", {String(i)}), "");
        storage.remove(SR("wifi.knownssids.index.?", {String(i)}));
        if (ssid != "")
        {
            storage.remove(SR("wifi.knownssids.?.index", {ssid}));
            storage.remove(SR("wifi.knownssids.?.password", {ssid}));
        }
    }

    storage.set("wifi.knownssids.index.count", "0");
    storage.set("wifi.current", "");
    ret->post(Errors::NoError);
    return ret;
}

Promise<Error>::smp_t WFService::addOrUpdateRegisteredNetwork(String ssid, String password)
{
    auto ret = Promise<Error>::get_smp();

    if (ssid == "")
    {
        ret->post(Errors::createError("SSID cannot be empty"));
        return ret;
    }

    auto existingIndex = storage.get(SR("wifi.knownssids.?.index", {ssid}), "");
    if (existingIndex != "")
    {
        storage.set(SR("wifi.knownssids.?.password", {ssid}), password);
        ret->post(Errors::NoError);
        return ret;
    }

    auto countStr = storage.get("wifi.knownssids.index.count", "0");
    auto newIndex = countStr.toInt();

    storage.set(SR("wifi.knownssids.index.?", {String(newIndex)}), ssid);
    storage.set(SR("wifi.knownssids.?.password", {ssid}), password);
    storage.set(SR("wifi.knownssids.?.index", {ssid}), String(newIndex));
    storage.set("wifi.knownssids.index.count", String(newIndex + 1));

    ret->post(Errors::NoError);
    return ret;
}

Promise<ResultWithStatus<SavedNetworkInfo>>::smp_t WFService::getRegisteredNetwork(String indexOrSsid)
{
    auto ret = Promise<ResultWithStatus<SavedNetworkInfo>>::get_smp();

    getRegisteredNetworkIndex(indexOrSsid)->then([=](ResultWithStatus<int> idxResult){
        if (idxResult.status != Errors::NoError)
        {
            ret->post(ResultWithStatus<SavedNetworkInfo>(SavedNetworkInfo{}, Errors::NotFound));
            return;
        }

        auto idx = idxResult.result;
        auto count = storage.get("wifi.knownssids.index.count", "0").toInt();
        if (idx < 0 || idx >= count)
        {
            ret->post(ResultWithStatus<SavedNetworkInfo>(SavedNetworkInfo{}, Errors::NotFound));
            return;
        }

        auto ssid = storage.get(SR("wifi.knownssids.index.?", {String(idx)}), "");
        if (ssid == "")
        {
            ret->post(ResultWithStatus<SavedNetworkInfo>(SavedNetworkInfo{}, Errors::NotFound));
            return;
        }

        auto password = storage.get(SR("wifi.knownssids.?.password", {ssid}), "");
        SavedNetworkInfo info;
        info.index = idx;
        info.password = password;
        info.networkInfo = NetworkInfo{.ssid = ssid, .rssid_dBm = -100};
        ret->post(ResultWithStatus<SavedNetworkInfo>(info, Errors::NoError));
    });

    return ret;
}

Promise<Error>::smp_t WFService::changeRegisteredNetwork(String indexOrSsid, String newSsid, String newPassword)
{
    auto ret = Promise<Error>::get_smp();

    getRegisteredNetworkIndex(indexOrSsid)->then([=](ResultWithStatus<int> idxResult){
        if (idxResult.status != Errors::NoError)
        {
            ret->post(Errors::NotFound);
            return;
        }

        deleteRegisteredNetwork(idxResult.result)->then([=](Error delErr){
            if (delErr != Errors::NoError)
            {
                ret->post(delErr);
                return;
            }

            addOrUpdateRegisteredNetwork(newSsid, newPassword)->then([=](Error addErr){
                ret->post(addErr);
            });
        });
    });

    return ret;
}

Promise<String>::smp_t WFService::getServiceInfo(){
    auto ret = Promise<String>::get_smp();
    String message="Wifi Service Info:\n";
    message+= "  Current state: "+stateToString(conStateChangeEvent.getLastData())+"\n";
    message+= "  Radio state: "+wlStateToString(WiFi.status())+"\n";
    message+= "  Connected ssid: "+connectedSsid+"\n";

    ret->post(message);
    return ret;
}

//these variable are being used because the ssid and password variables (previusly tryied) was not being copied to the lambda functions
String __tmpssid="";
String __tmppassword="";
/* //evtbus void WFService::handleEventBusOperations()
{
    this->messageBus->listen("request wifi.*", [=](MBMessage<String> msg){
        
        if (msg.message == "request wifi.list"){

            this->messageBus->stream("wifi.list", "responses in wifi_list__late_response");

            nLog.debug("Requesting list");
            this->getAvailableNetworks()->then<TpNothing>([=] (vector<NetworkInfo> result)
            {
                String resultStr = "";
                for (size_t c = 0; c < result.size(); c++)
                {
                    resultStr += result[c].ssid + " ("+String(result[c].rssid_dBm)+")";

                    if (c < result.size()-1)
                        resultStr += '\n';
                }

                this->scheduler.run([=](){
                    this->messageBus->stream("wifi_list__late_response", resultStr);
                    this->messageBus->stream("wifi_list__late_response", "done");
                }, "wifi::handleBus::1", DEFAULT_PRIORITIES::LOW_);

                return Nothing;
            });
        }
        else if (msg.message == "request wifi.connect"){
            auto parts=StringUtils::split(msg.payload, " ");
            if (parts.size() == 2)
            {
                //By some rason, these variables are not being copied to the lambda functions
                //string mpssid = parts[0];
                //string mppassword = parts[1];

                __tmpssid = parts[0];
                __tmppassword = parts[1];
                this->connectToNetwork(__tmpssid, __tmppassword)->then([=](Error error){
                    if (error == Errors::NoError)
                    {
                        //save networkd to known netowrks
                        auto tmp = SR("wifi.knownssids.?.password", {__tmpssid});
                        this->storage.set(tmp, __tmppassword);
                            //save the current network to an index of network
                            this->registerKnownNetwork(__tmpssid, __tmppassword)->then([=](TpNothing _void){
                                this->storage.set("wifi.current", __tmpssid);
                            });

                    }
                    else {
                        this->scheduler.run([=](){
                            this->messageBus->stream("wifi.connect", Errors::DerivateError(error, "Error connecting to network"));
                        }, "wifi::handleBus::2", DEFAULT_PRIORITIES::LOW_);
                    }
                });
            }
        }
        else if (msg.message == "request wifi.info"){
            this->getServiceInfo()->then([=](String info){
                this->messageBus->stream("wifi.info", info);
            });
        }
        else if (msg.message == "request wifi.saved.list"){
            this->messageBus->stream("wifi.saved.list", "responses in wifi_saved_networks__late_response");
            this->getRegisteredNetworks()->then([=](auto networks){
                String result = "";
                int index = 0;
                for (auto &c: networks)
                {
                    result += String(c.index) + ") "+c.networkInfo.ssid+" (";
                    if (c.networkInfo.rssid_dBm > -100)
                        result += String(c.networkInfo.rssid_dBm);
                    else
                        result += "unreachable";

                    result += ")";

                    if (index < networks.size()-1)
                        result += '\n';

                    index++;
                }

                this->scheduler.run([=](){
                    this->messageBus->stream("wifi_saved_networks__late_response", result);
                    this->messageBus->stream("wifi_saved_networks__late_response", "done");
                }, "wifi::handlebus::3", DEFAULT_PRIORITIES::LOW_);
            });
        }
        else if (msg.message == "request wifi.saved.delete"){
            auto parts=StringUtils::split(msg.payload, " ");
            if (parts.size() != 1)
            {
                this->messageBus->stream("wifi.saved.delete", Errors::createError("Invalid number of arguments"));
                return;
            }

            //TODO: check if the id is a number
            if (StringUtils::IsNumber(parts[0], 0, 1000) == false)
            {
                this->messageBus->stream("wifi.saved.delete", Errors::createError("Invalid number"));
                return;
            }

            this->messageBus->stream("wifi.saved.delete", "responses in wifi_delete_saved__late_response");
            int id = parts[0].toInt();
            this->deleteRegisteredNetwork(id)->then([=](Error error){

                if (error != Errors::NoError){
                    this->scheduler.run([=](){
                        this->messageBus->stream("wifi_delete_saved__late_response", Errors::DerivateError(error, "Error deleting network"));
                        this->messageBus->stream("wifi_delete_saved__late_response", "done");
                    }, "wifi::handlebus::4", DEFAULT_PRIORITIES::LOW_);
                    return;
                }
                
                this->messageBus->stream("wifi_delete_saved__late_response", "Ok, deleted");
                this->messageBus->stream("wifi_delete_saved__late_response", "done");
            });
        } 
        else{
            auto spacePosition=msg.message.indexOf(' ');
            if (spacePosition > -1)
            {
                auto response = msg.message.substring(spacePosition+1);
                this->messageBus->post(response, "Unknown wifi command/request");
            }
            else
                this->messageBus->post("wifi request error", "Unknown wifi command/request");
            
        }
    });


    this->messageBus->listen("request services.helptext", [=](MBMessage<String> msg){

        String result = 
        String("Wifi commands ('NI' means the command is not implemented yet):\n") +
        String("   wifi.list - Return the available wifi networks\n") +
        String("   wifi.connect <ssid> <password> - Connect to an wifi network\n") +
        String("       <ssid>     - the ssid of the network to connect\n") +
        String("       <password> - the password of the network to connect\n" )+ 
        String("   (NI) wifi.startAp - Disconnect the current network (if connected) and enters in AP mode\n") +
        String("   wifi.info - Return the current state of the wifi service, including current ssid and ip address\n") +
        String("   wifi.saved.list - List saved networks in format '<id>) <ssid>' \n") +
        String("   wifi.saved.delete <id>- List saved networks\n") +
        String("       <id> - the network saved id (get ids using wifi.save.list)\n");
        String("   (NI) wifi.saved.clear - clear all saved networks\n") +
        String("       <id> - the network saved id (get ids using wifi.save.list)\n");


            this->messageBus->post("services.helptext", result);
    });

    this->messageBus->listen("request info.services states", [=](MBMessage<String> msg){
        getServiceInfo()->then([=](String info){
            this->messageBus->stream("info.service state.WifiService", info);
        });
    });



} 
    */