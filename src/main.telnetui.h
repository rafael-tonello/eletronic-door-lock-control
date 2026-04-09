#ifndef __MAIN_TELNETUI__H__
#define __MAIN_TELNETUI__H__

#include "main.h"
#include "main.lockcontrol.h"

class MainTelnetUI {
private:
    Scheduler scheduler;

    IIOHal &hal;
    ILogger &logService;
    IStorage &storage;
    INetwork &conService;
    IKeyboard &keyboard;
    VSTP::VstpClient *vssCli;
    TelnetServer &telnetInterface;
    MainLockControl &lockControl;
    Configs &configs;

    String helpTitleLine;

    NamedLog nLog;

    std::map<String, function<void(std::shared_ptr<TelnetServer::CliInfo>, function<void()>, vector<String>)>> commandsMap;

public:
    MainTelnetUI(
        String helpTitleLine,
        IIOHal &hal,
        ILogger &logService,
        IStorage &storage,
        INetwork &conService,
        IKeyboard &keyboard,
        VSTP::VstpClient *vssCli,
        TelnetServer &telnetInterface,
        MainLockControl &lockControl,
        Configs &configs
    ):
        hal(hal),
        logService(logService),
        storage(storage),
        conService(conService),
        keyboard(keyboard),
        vssCli(vssCli),
        telnetInterface(telnetInterface),
        lockControl(lockControl),
        configs(configs),
        helpTitleLine(helpTitleLine)
    {
        nLog = logService.getNLog("MainTelnetUI");

        initCommandsMap();

        telnetInterface.onClientConnected.listen([=](auto cli){
            
            auto subscribeId = cli->onDataReceived.subscribe([=](tuple<String, function<void()>> data){
                auto onReadyFunc = get<1>(data);
                String line = get<0>(data);

                int firstSpace = line.indexOf(" ");
                String cmd = line;
                String argsRaw = "";
                if (firstSpace > -1)
                {
                    cmd = line.substring(0, firstSpace);
                    argsRaw = line.substring(firstSpace + 1);
                }

                auto args = StringUtils::split(argsRaw, " ");

                if (commandsMap.count(cmd)){
                    commandsMap[cmd](cli, onReadyFunc, args);
                } else {
                    cli->sendData("Unknown command: " + cmd);
                    onReadyFunc();
                }
            });
            cli->tags["subscribeId"] = String(subscribeId);

            displayHelp(cli, [](){ });
        });

        telnetInterface.onClientDisconnected.listen([=](auto cli){
            if (cli->tags.count("subscribeId"))
            {
                auto subscribeId = cli->tags["subscribeId"].toInt();
                cli->onDataReceived.unsubscribe(subscribeId);
            }
            

            //unsubscribe from logs if the client was subscribed

            if (cli->tags.count("logs_observing_id") && cli->tags["logs_observing_id"] != "-1"){
                logsCommands(cli, [](){ }, {"stop"});
            }
        });
    }

    void initCommandsMap()
    {
        commandsMap["help"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            displayHelp(cli, onReady);
        };

        commandsMap["wifi"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            wifiCommands(cli, onReady, args);
        };

        commandsMap["logs"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            logsCommands(cli, onReady, args);
        };

        commandsMap["door"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            doorCommands(cli, onReady, args);
        };

        commandsMap["locker"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            doorCommands(cli, onReady, args);
        };

        commandsMap["vss"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            vssCommands(cli, onReady, args);
        };

        commandsMap["vstp"] = [=](std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args){
            vssCommands(cli, onReady, args);
        };
    }

    
    void displayHelp(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady)
    {
        cli->sendData(helpTitleLine);
        cli->sendData("Available commands:");
        cli->sendData("  help                   - Show this help message");
        cli->sendData("  door|locker <subcommand> - Door lock control commands.");
        cli->sendData("    subcommands:");
        cli->sendData("      lock [timeout_ms]    - Lock the door using the motor controller");
        cli->sendData("      unlock [timeout_ms]  - Unlock the door using the motor controller");
        cli->sendData("      timeout [new_ms]     - Show or change the saved door timeout");
        cli->sendData("  wifi <subcommand>      - run wifi related commands. ");
        cli->sendData("    subcommands:");
        cli->sendData("      list                 - List available wifi networks");
        cli->sendData("      connect <ssid> <password> ");
        cli->sendData("                           - Connect to a wifi network. If connection");
        cli->sendData("                             succeeds, the network will be saved.");
        cli->sendData("      startAp              - Enters in access point mode. If connected");
        cli->sendData("                             to a network, it will be disconnected.");
        cli->sendData("      info                 - Show current wifi connection info");
        cli->sendData("      saved <subcommand>   - manages saved networks");
        cli->sendData("        subcommands:");
        cli->sendData("          list               - List saved networks");
        cli->sendData("          delete <id or ssid>");
        cli->sendData("                             - Delete a saved network. Get the id using 'wifi"); 
        cli->sendData("                               saved list'");
        cli->sendData("          deleteall          - Delete all saved networks.");
        cli->sendData("          add <ssid> <password> ");
        cli->sendData("                             - Add a saved network without connecting to it.");
        cli->sendData("                               If the ssid already exists, the password will be");
        cli->sendData("                               updated.");
        cli->sendData("          change <id or ssid> <new ssid> <new password>");
        cli->sendData("                             - Add a saved network without connecting to it.");
        cli->sendData("          get <id or ssid>   - Get the ssid and password of a saved networ");
        cli->sendData("  logs <subcommand>      - run wifi related commands. ");
        cli->sendData("    subcommands:");
        cli->sendData("      listen               - Start listening to logs.");
        cli->sendData("      stop                 - Stop listening to logs.");
        cli->sendData("  vss|vstp <subcommand> - VSS/VSTP server connection commands.");
        cli->sendData("    subcommands:");
        cli->sendData("      info                 - Show current VSS/VSTP config and state.");
        cli->sendData("      connect <server> <port> [prefix]");
        cli->sendData("                           - Save config and start/restart connection monitor.");
        cli->sendData("      reconnect            - Connect using saved server/port config.");
        cli->sendData("      disconnect           - Stop monitor and close current connection.");
        onReady();
    }

    void doorCommands(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args)
    {
        if (args.size() == 0 || args[0] == "")
        {
            cli->sendData("Missing door subcommand. Use: door lock [timeout_ms] | door unlock [timeout_ms] | door timeout [new_timeout_ms]");
            onReady();
            return;
        }

        auto sub = args[0];

        if (sub == "lock")
        {
            auto lockArgs = vector<String>();
            if (args.size() > 1)
                lockArgs.assign(args.begin() + 1, args.end());

            lockCommands(cli, onReady, lockArgs, true);
            return;
        }
        else if (sub == "unlock")
        {
            auto lockArgs = vector<String>();
            if (args.size() > 1)
                lockArgs.assign(args.begin() + 1, args.end());

            lockCommands(cli, onReady, lockArgs, false);
            return;
        }
        else if (sub == "timeout")
        {
            if (args.size() == 1)
            {
                cli->sendData("Current door timeout: " + String(configs.get(AppMain::CONFIG_LOCK_TIMEOUT, String(AppMain::DEFAULT_LOCK_TIMEOUT_MS))) + "ms");
                onReady();
                return;
            }

            auto parsedTimeout = args[1].toInt();
            if (parsedTimeout <= 0)
            {
                cli->sendData("Invalid timeout. Usage: door timeout [new_timeout_ms]");
                onReady();
                return;
            }

            configs.set(AppMain::CONFIG_LOCK_TIMEOUT, String(parsedTimeout));
            cli->sendData("Door timeout updated to " + String(parsedTimeout) + "ms.");
            onReady();
            return;
        }

        cli->sendData("Unknown door subcommand: " + sub);
        cli->sendData("Available: lock, unlock, timeout");
        onReady();
    }

    void lockCommands(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args, bool isLockAction)
    {
        uint timeoutMs = configs.get(AppMain::CONFIG_LOCK_TIMEOUT, String(AppMain::DEFAULT_LOCK_TIMEOUT_MS)).toInt();
        if (args.size() > 0 && args[0] != "")
        {
            auto parsedTimeout = args[0].toInt();
            if (parsedTimeout <= 0)
            {
                cli->sendData("Invalid timeout. Usage: door " + String(isLockAction ? "lock" : "unlock") + " [timeout_ms]");
                onReady();
                return;
            }

            timeoutMs = (uint)parsedTimeout;
        }

        nLog.info("telnet requested to " + String(isLockAction ? "lock" : "unlock") + " with timeout " + String(timeoutMs) + "ms");
        cli->sendData(String(isLockAction ? "Lock" : "Unlock") + " request started. Waiting for completion...");

        auto action = isLockAction ? lockControl.Lock(timeoutMs) : lockControl.Unlock(timeoutMs);
        action->then([=](MainLockControl::LockUnlockResult result){
            if (result.err == Errors::NoError)
            {
                cli->sendData(String(isLockAction ? "Door locked" : "Door unlocked") + " successfully in " + String(result.timeTaken) + "ms.");
            }
            else
            {
                cli->sendData("Failed to " + String(isLockAction ? "lock" : "unlock") + ": " + result.err + " (" + String(result.timeTaken) + "ms)");
            }

            onReady();
        });
    }

    String vssStateToString(VSTP::VstpClientState state)
    {
        if (state == VSTP::VstpClientState::DISCONNECTED)
            return "DISCONNECTED";
        if (state == VSTP::VstpClientState::WAITING_NEXT_IP)
            return "WAITING_NEXT_IP";
        if (state == VSTP::VstpClientState::CONNECTING)
            return "CONNECTING";
        if (state == VSTP::VstpClientState::CONNECTED)
            return "CONNECTED";
        return "UNKNOWN";
    }

    void vssCommands(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args)
    {
        if (vssCli == nullptr)
        {
            cli->sendData("VSS/VSTP client is not initialized.");
            onReady();
            return;
        }

        if (args.size() == 0 || args[0] == "")
        {
            cli->sendData("Missing vss subcommand. Use: vss info | vss connect <server> <port> [prefix] | vss reconnect | vss disconnect");
            onReady();
            return;
        }

        auto sub = args[0];
        if (sub == "info" || sub == "status")
        {
            auto server = storage.get("vstp.server", "");
            auto port = storage.get("vstp.port", "5032");
            auto prefix = storage.get("vstp.prefix", "n1.sys.genio.");
            auto state = vssStateToString(vssCli->onConnectionStateChange.getLastData());

            cli->sendData("VSS/VSTP info:");
            cli->sendData("  server: " + (server == "" ? "<empty>" : server));
            cli->sendData("  port: " + port);
            cli->sendData("  prefix: " + prefix);
            cli->sendData("  state: " + state);

            onReady();
        }
        else if (sub == "connect")
        {
            if (args.size() < 3)
            {
                cli->sendData("Usage: vss connect <server> <port> [prefix]");
                onReady();
                return;
            }

            auto server = args[1];
            auto portRaw = args[2];
            auto port = portRaw.toInt();

            if (server == "" || port <= 0)
            {
                cli->sendData("Invalid arguments. Usage: vss connect <server> <port> [prefix]");
                onReady();
                return;
            }

            storage.set("vstp.server", server);
            storage.set("vstp.port", String(port));
            if (args.size() >= 4)
                storage.set("vstp.prefix", args[3]);

            auto err = vssCli->start(server, port);
            if (err == Errors::NoError)
                cli->sendData("VSS/VSTP connect requested for " + server + ":" + String(port));
            else
                cli->sendData("Error starting VSS/VSTP connection: " + err);

            onReady();
        }
        else if (sub == "reconnect")
        {
            auto server = storage.get("vstp.server", "");
            auto portRaw = storage.get("vstp.port", "5032");
            auto port = portRaw.toInt();

            auto err = vssCli->start(server, port);
            if (err == Errors::NoError)
                cli->sendData("VSS/VSTP reconnect requested for " + server + ":" + String(port));
            else
                cli->sendData("Error reconnecting to VSS/VSTP server: " + err);

            onReady();
        }
        else if (sub == "disconnect")
        {
            vssCli->stop();
            cli->sendData("VSS/VSTP connection stopped.");
            onReady();
        }
        else {

            cli->sendData("Unknown vss subcommand: " + sub);
            cli->sendData("Available: info, connect, reconnect, disconnect");
            onReady();
        }
    }

    void wifiCommands(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args)
    {
        //auto wifi = static_cast<WFService*>(&conService);
        auto wifi = (WFService*)&conService;

        if (args.size() == 0 || args[0] == "")
        {
            cli->sendData("Missing wifi subcommand. Use: wifi list | wifi connect <ssid> <password> | wifi startAp | wifi info | wifi saved ...");
            onReady();
            return;
        }

        auto sub = args[0];

        if (sub == "list")
        {
            cli->sendData("Scanning for WiFi networks...");
            delay(100);
            wifi->getAvailableNetworks()->then([=](vector<NetworkInfo> nets){
                if (nets.size() == 0)
                    cli->sendData("No WiFi networks found.");
                else
                {
                    sort(nets.begin(), nets.end(), [](NetworkInfo &a, NetworkInfo &b){
                        return a.rssid_dBm > b.rssid_dBm;
                    });

                    cli->sendData("Available WiFi networks:");
                    for (auto &net : nets){
                        String signalQuality = drawWifiSignalBarChart(net.rssid_dBm);


                        cli->sendData("  " + signalQuality + " " + net.ssid + " (" + String(net.rssid_dBm) + " dBm)");
                    }
                }
                onReady();
            });
            return;
        }
        else if (sub == "connect")
        {
            if (args.size() < 3)
            {
                cli->sendData("Usage: wifi connect <ssid> <password>");
                onReady();
                return;
            }

            auto ssid = args[1];
            auto password = args[2];
            wifi->connectToNetwork(ssid, password)->then([=](Error err){
                if (err == Errors::NoError)
                    cli->sendData("Connected to '" + ssid + "'.");
                else
                    cli->sendData("Error connecting to '" + ssid + "': " + err);
                onReady();
            });
            return;
        }
        else if (sub == "startAp")
        {
            wifi->startAccessPoint()->then([=](TpNothing _void){
                auto apInfo = wifi->getWifiApAddrInfo();
                cli->sendData("Access point started.");
                cli->sendData("  ip: " + apInfo.ip);
                cli->sendData("  gateway: " + apInfo.gatewayIp);
                cli->sendData("  broadcast: " + apInfo.broadcastIp);
                onReady();
            });
            return;
        }
        else if (sub == "info" || sub == "status")
        {
            auto state = wifi->stateToString(wifi->conStateChangeEvent.getLastData());
            auto cliInfo = wifi->getWifiCliAddrInfo();
            auto apInfo = wifi->getWifiApAddrInfo();
            auto ApMode = wifi->conStateChangeEvent.getLastData() == WFService::WFServiceState::AP_STARTED;


            cli->sendData("WiFi info:");
            cli->sendData("  type: " + String(ApMode ? "Access Point" : "Station"));
            cli->sendData("  state: " + state);
            cli->sendData("  STA ip: " + cliInfo.ip);
            cli->sendData("  STA gateway: " + cliInfo.gatewayIp);
            cli->sendData("  AP ip: " + apInfo.ip);
            cli->sendData("  Current network (if STA): " + (cliInfo.networkInfo.ssid != "" ? (drawWifiSignalBarChart(cliInfo.networkInfo.rssid_dBm) + " " +cliInfo.networkInfo.ssid + " (" + String(cliInfo.networkInfo.rssid_dBm) + " dBm)"): "AP Mode"));
             

            onReady();
            return;
        }
        else if (sub == "saved")
        {
            if (args.size() < 2)
            {
                cli->sendData("Usage: wifi saved list | wifi saved delete <id or ssid> | wifi saved deleteall | wifi saved add <ssid> <password> | wifi saved change <id or ssid> <new ssid> <new password> | wifi saved get <id or ssid>");
                onReady();
                return;
            }

            auto savedSub = args[1];
            if (savedSub == "list")
            {
                wifi->getRegisteredNetworks(false)->then([=](vector<SavedNetworkInfo> nets){
                    if (nets.size() == 0)
                    {
                        cli->sendData("No saved networks.");
                        onReady();
                        return;
                    }

                    cli->sendData("Saved networks:");
                    for (auto &net : nets)
                        cli->sendData("  [" + String(net.index) + "] " + net.networkInfo.ssid + " (" + net.password + ")");

                    onReady();
                });
                return;
            }
            else if (savedSub == "delete")
            {
                if (args.size() < 3)
                {
                    cli->sendData("Usage: wifi saved delete <id or ssid>");
                    onReady();
                    return;
                }

                auto target = args[2];
                wifi->deleteRegisteredNetwork(target)->then([=](Error err){
                    if (err == Errors::NoError)
                        cli->sendData("Saved network deleted: " + target);
                    else
                        cli->sendData("Error deleting saved network: " + err);
                    onReady();
                });
                return;
            }
            else if (savedSub == "deleteall")
            {
                wifi->deleteAllRegisteredNetworks()->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli->sendData("All saved networks were removed.");
                    else
                        cli->sendData("Error deleting all saved networks: " + err);
                    onReady();                });
                return;
            }
            else if (savedSub == "add")
            {
                if (args.size() < 4)
                {
                    cli->sendData("Usage: wifi saved add <ssid> <password>");
                    onReady();
                    return;
                }

                auto ssid = args[2];
                auto password = args[3];
                wifi->addOrUpdateRegisteredNetwork(ssid, password)->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli->sendData("Saved network stored: " + ssid);
                    else
                        cli->sendData("Error storing saved network: " + err);
                    onReady();
                });
                return;
            }
            else if (savedSub == "get")
            {
                if (args.size() < 3)
                {
                    cli->sendData("Usage: wifi saved get <id or ssid>");
                    onReady();
                    return;
                }

                auto target = args[2];
                wifi->getRegisteredNetwork(target)->then([=, &cli](ResultWithStatus<SavedNetworkInfo> result){
                    if (result.status != Errors::NoError)
                    {
                        cli->sendData("Saved network not found: " + target);
                        onReady();
                        return;
                    }

                    cli->sendData("Saved network:");
                    cli->sendData("  id: " + String(result.result.index));
                    cli->sendData("  ssid: " + result.result.networkInfo.ssid);
                    cli->sendData("  password: " + result.result.password);
                    onReady();
                });
                return;
            }
            else if (savedSub == "change")
            {
                if (args.size() < 5)
                {
                    cli->sendData("Usage: wifi saved change <id or ssid> <new ssid> <new password>");
                    onReady();
                    return;
                }

                auto target = args[2];
                auto newSsid = args[3];
                auto newPassword = args[4];
                wifi->changeRegisteredNetwork(target, newSsid, newPassword)->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli->sendData("Saved network changed: " + target + " -> " + newSsid);
                    else
                        cli->sendData("Error changing saved network: " + err);
                    onReady();
                });
                return;
            }

            cli->sendData("Unsupported subcommand: wifi saved " + savedSub);
            cli->sendData("Available: list, delete, deleteall, add, change, get");
            onReady();
            return;
        }

        cli->sendData("Unknown wifi subcommand: " + sub);
        onReady();
    }

    void logsCommands(std::shared_ptr<TelnetServer::CliInfo>cli, function<void()> onReady, vector<String> args)
    {

        if (args.size() == 0 || args[0] == "")
        {
            cli->sendData("Missing logs subcommand. Use: logs listen | logs stop");
            onReady();
            return;
        }

        auto sub = args[0];

        if (sub == "listen" || sub == "start")
        {
            if (cli->tags.count("logs_observing_id") && cli->tags["logs_observing_id"] != "-1")
            {
                cli->sendData("Already listening to logs.");
                onReady();
                return;
            }

            auto observingId = this->logService.OnLog.listen([=](ILoggerObservingItem item){
                auto msg = logService.MountLineHeader(item.level, item.name, true);
                msg += logService.identMsg(item.msg, msg.length());
                msg += FF::Reset;

                cli->sendData(msg);
            });
            cli->tags["logs_observing_id"] = String(observingId);
        }
        else if (sub == "stop")
        {
            if (!cli->tags.count("logs_observing_id"))
            {
                cli->sendData("Not currently listening to logs.");
                onReady();
                return;
            }

            auto observingId = cli->tags["logs_observing_id"].toInt();
            this->logService.OnLog.stopListening(observingId);
            cli->tags.erase("logs_observing_id");
        }
        else
        {
            cli->sendData("Unknown logs subcommand: " + sub);
            cli->sendData("Available: listen, stop");
            onReady();
            return;
        }

        onReady();
    }
        

    String drawWifiSignalBarChart(int rssid_dBm)
    {
        String signalQuality = "[          ]";
                        
        //convert signal to percentagem (-50 or higher = 100%, -100 or lower = 0%)
        auto tmpSignal = rssid_dBm;
        if (tmpSignal > -50) tmpSignal = -50;
        else if (tmpSignal < -100) tmpSignal = -100;
        auto percent = (tmpSignal + 100) * 100 / 50;

        //converto to 10 scale 
        auto perTen = percent / 10;
        for (int i = 0; i < perTen; i++)
            signalQuality.setCharAt(i + 1, '#');

        return signalQuality;
    }
};

#endif
