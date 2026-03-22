#ifndef __MAIN_TELNETUI__H__
#define __MAIN_TELNETUI__H__

#include <errors/errors.h>

#include <scheduler.h>

#include <ikeyboard.h>
#include <keyboard/impl/GenericIIOHalKeyboard.h>

#include <iiohal.h>
#include <iohal/impl/esp8266IOHal.h>

#include <ilogger.h>

#include <istorage.h>
#include <storage/impl/preferenceslibrary/preferenceslibrarystorage.h>

#include <inetwork.h>

#include <network/inetwork.h>
#include <network/impl/wfservice.h>

#include <telnet/telnetserver.h>
#include <stringutils.h>

class MainTelnetUI {
private:
    Scheduler scheduler;

    IIOHal &hal;
    ILogger &logService;
    IStorage &storage;
    INetwork &conService;
    IKeyboard &keyboard;
    TelnetServer &telnetInterface;

    std::map<String, function<void(TelnetServer::CliInfo &, function<void()>, vector<String>)>> commandsMap;

public:
    MainTelnetUI(
        IIOHal &hal,
        ILogger &logService,
        IStorage &storage,
        INetwork &conService,
        IKeyboard &keyboard,
        TelnetServer &telnetInterface
    ): 
        hal(hal), 
        logService(logService), 
        storage(storage), 
        conService(conService), 
        keyboard(keyboard), 
        telnetInterface(telnetInterface)
    {

        initCommandsMap();

        telnetInterface.onClientConnected.listen([=](TelnetServer::CliInfo &cli){
            cli.onDataReceived.subscribe([&](tuple<String, function<void()>> data){
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

                if (commandsMap.count(cmd))
                    commandsMap[cmd](cli, onReadyFunc, args);
                else
                {
                    cli.sendData("Unknown command: " + cmd);
                    onReadyFunc();
                }
            });

            displayHelp(cli, [](){ });
        });
    }

    void initCommandsMap()
    {
        commandsMap["help"] = [=](TelnetServer::CliInfo &cli, function<void()> onReady, vector<String> args){
            displayHelp(cli, onReady);
        };

        commandsMap["wifi"] = [=](TelnetServer::CliInfo &cli, function<void()> onReady, vector<String> args){
            wifiCommands(cli, onReady, args);
        };

        commandsMap["logs"] = [=](TelnetServer::CliInfo &cli, function<void()> onReady, vector<String> args){
            logsCommands(cli, onReady, args);
        };
    }

    
    void displayHelp(TelnetServer::CliInfo &cli, function<void()> onReady)
    {
        cli.sendData("Available commands:");
        cli.sendData("  help                   - Show this help message");
        cli.sendData("  wifi <subcommand>      - run wifi related commands. ");
        cli.sendData("    subcommands:");
        cli.sendData("      list                 - List available wifi networks");
        cli.sendData("      connect <ssid> <password> ");
        cli.sendData("                           - Connect to a wifi network. If connection");
        cli.sendData("                             succeeds, the network will be saved.");
        cli.sendData("      startAp              - Enters in access point mode. If connected");
        cli.sendData("                             to a network, it will be disconnected.");
        cli.sendData("      info                 - Show current wifi connection info");
        cli.sendData("      saved <subcommand>   - manages saved networks");
        cli.sendData("        subcommands:");
        cli.sendData("          list               - List saved networks");
        cli.sendData("          delete <id or ssid>");
        cli.sendData("                             - Delete a saved network. Get the id using 'wifi"); 
        cli.sendData("                               saved list'");
        cli.sendData("          deleteall          - Delete all saved networks.");
        cli.sendData("          add <ssid> <password> ");
        cli.sendData("                             - Add a saved network without connecting to it.");
        cli.sendData("                               If the ssid already exists, the password will be");
        cli.sendData("                               updated.");
        cli.sendData("          change <id or ssid> <new ssid> <new password>");
        cli.sendData("                             - Add a saved network without connecting to it.");
        cli.sendData("          get <id or ssid>   - Get the ssid and password of a saved networ");
        cli.sendData("  logs <subcommand>      - run wifi related commands. ");
        cli.sendData("    subcommands:");
        cli.sendData("      listen               - Start listening to logs.");
        cli.sendData("      stop                 - Stop listening to logs.");
        onReady();
    }

    void wifiCommands(TelnetServer::CliInfo &cli, function<void()> onReady, vector<String> args)
    {
        //auto wifi = static_cast<WFService*>(&conService);
        auto wifi = (WFService*)&conService;

        if (args.size() == 0 || args[0] == "")
        {
            cli.sendData("Missing wifi subcommand. Use: wifi list | wifi connect <ssid> <password> | wifi startAp | wifi info | wifi saved ...");
            onReady();
            return;
        }

        auto sub = args[0];

        if (sub == "list" || sub == "stop")
        {
            cli.sendData("Scanning for WiFi networks...");
            delay(100);
            wifi->getAvailableNetworks()->then([=, &cli](vector<NetworkInfo> nets){
                if (nets.size() == 0)
                    cli.sendData("No WiFi networks found.");
                else
                {
                    sort(nets.begin(), nets.end(), [](NetworkInfo &a, NetworkInfo &b){
                        return a.rssid_dBm > b.rssid_dBm;
                    });

                    cli.sendData("Available WiFi networks:");
                    for (auto &net : nets){
                        String signalQuality = drawWifiSignalBarChart(net.rssid_dBm);


                        cli.sendData("  " + signalQuality + " " + net.ssid + " (" + String(net.rssid_dBm) + " dBm)");
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
                cli.sendData("Usage: wifi connect <ssid> <password>");
                onReady();
                return;
            }

            auto ssid = args[1];
            auto password = args[2];
            wifi->connectToNetwork(ssid, password)->then([=, &cli](Error err){
                if (err == Errors::NoError)
                    cli.sendData("Connected to '" + ssid + "'.");
                else
                    cli.sendData("Error connecting to '" + ssid + "': " + err);
                onReady();
            });
            return;
        }
        else if (sub == "startAp")
        {
            wifi->startAccessPoint()->then([=, &cli](TpNothing _void){
                auto apInfo = wifi->getWifiApAddrInfo();
                cli.sendData("Access point started.");
                cli.sendData("  ip: " + apInfo.ip);
                cli.sendData("  gateway: " + apInfo.gatewayIp);
                cli.sendData("  broadcast: " + apInfo.broadcastIp);
                onReady();
            });
            return;
        }
        else if (sub == "info")
        {
            auto state = wifi->stateToString(wifi->conStateChangeEvent.getLastStreamedData());
            auto cliInfo = wifi->getWifiCliAddrInfo();
            auto apInfo = wifi->getWifiApAddrInfo();
            auto ApMode = wifi->conStateChangeEvent.getLastStreamedData() == WFService::WFServiceState::AP_STARTED;


            cli.sendData("WiFi info:");
            cli.sendData("  type: " + String(ApMode ? "Access Point" : "Station"));
            cli.sendData("  state: " + state);
            cli.sendData("  STA ip: " + cliInfo.ip);
            cli.sendData("  STA gateway: " + cliInfo.gatewayIp);
            cli.sendData("  AP ip: " + apInfo.ip);
            cli.sendData("  Current network (if STA): " + (cliInfo.networkInfo.ssid != "" ? (drawWifiSignalBarChart(cliInfo.networkInfo.rssid_dBm) + " " +cliInfo.networkInfo.ssid + " (" + String(cliInfo.networkInfo.rssid_dBm) + " dBm)"): "AP Mode"));
             

            onReady();
            return;
        }
        else if (sub == "saved")
        {
            if (args.size() < 2)
            {
                cli.sendData("Usage: wifi saved list | wifi saved delete <id or ssid> | wifi saved deleteall | wifi saved add <ssid> <password> | wifi saved change <id or ssid> <new ssid> <new password> | wifi saved get <id or ssid>");
                onReady();
                return;
            }

            auto savedSub = args[1];
            if (savedSub == "list")
            {
                wifi->getRegisteredNetworks(false)->then([=, &cli](vector<SavedNetworkInfo> nets){
                    if (nets.size() == 0)
                    {
                        cli.sendData("No saved networks.");
                        onReady();
                        return;
                    }

                    cli.sendData("Saved networks:");
                    for (auto &net : nets)
                        cli.sendData("  [" + String(net.index) + "] " + net.networkInfo.ssid + " (" + net.password + ")");

                    onReady();
                });
                return;
            }
            else if (savedSub == "delete")
            {
                if (args.size() < 3)
                {
                    cli.sendData("Usage: wifi saved delete <id or ssid>");
                    onReady();
                    return;
                }

                auto target = args[2];
                wifi->deleteRegisteredNetwork(target)->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli.sendData("Saved network deleted: " + target);
                    else
                        cli.sendData("Error deleting saved network: " + err);
                    onReady();
                });
                return;
            }
            else if (savedSub == "deleteall")
            {
                wifi->deleteAllRegisteredNetworks()->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli.sendData("All saved networks were removed.");
                    else
                        cli.sendData("Error deleting all saved networks: " + err);
                    onReady();
                });
                return;
            }
            else if (savedSub == "add")
            {
                if (args.size() < 4)
                {
                    cli.sendData("Usage: wifi saved add <ssid> <password>");
                    onReady();
                    return;
                }

                auto ssid = args[2];
                auto password = args[3];
                wifi->addOrUpdateRegisteredNetwork(ssid, password)->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli.sendData("Saved network stored: " + ssid);
                    else
                        cli.sendData("Error storing saved network: " + err);
                    onReady();
                });
                return;
            }
            else if (savedSub == "get")
            {
                if (args.size() < 3)
                {
                    cli.sendData("Usage: wifi saved get <id or ssid>");
                    onReady();
                    return;
                }

                auto target = args[2];
                wifi->getRegisteredNetwork(target)->then([=, &cli](ResultWithStatus<SavedNetworkInfo> result){
                    if (result.status != Errors::NoError)
                    {
                        cli.sendData("Saved network not found: " + target);
                        onReady();
                        return;
                    }

                    cli.sendData("Saved network:");
                    cli.sendData("  id: " + String(result.result.index));
                    cli.sendData("  ssid: " + result.result.networkInfo.ssid);
                    cli.sendData("  password: " + result.result.password);
                    onReady();
                });
                return;
            }
            else if (savedSub == "change")
            {
                if (args.size() < 5)
                {
                    cli.sendData("Usage: wifi saved change <id or ssid> <new ssid> <new password>");
                    onReady();
                    return;
                }

                auto target = args[2];
                auto newSsid = args[3];
                auto newPassword = args[4];
                wifi->changeRegisteredNetwork(target, newSsid, newPassword)->then([=, &cli](Error err){
                    if (err == Errors::NoError)
                        cli.sendData("Saved network changed: " + target + " -> " + newSsid);
                    else
                        cli.sendData("Error changing saved network: " + err);
                    onReady();
                });
                return;
            }

            cli.sendData("Unsupported subcommand: wifi saved " + savedSub);
            cli.sendData("Available: list, delete, deleteall, add, change, get");
            onReady();
            return;
        }

        cli.sendData("Unknown wifi subcommand: " + sub);
        onReady();
    }

    void logsCommands(TelnetServer::CliInfo &cli, function<void()> onReady, vector<String> args)
    {

        if (args.size() == 0 || args[0] == "")
        {
            cli.sendData("Missing logs subcommand. Use: logs listen | logs stop");
            onReady();
            return;
        }

        auto sub = args[0];

        if (sub == "listen")
        {
            if (cli.tags.count("logs_observing_id") && cli.tags["logs_observing_id"] != "-1")
            {
                cli.sendData("Already listening to logs.");
                onReady();
                return;
            }

            auto observingId = this->logService.OnLog.listen([&](ILoggerObservingItem item){
                auto msg = logService.MountLineHeader(item.level, item.name) + logService.identMsg(item.msg, 0);
                msg += logService.identMsg(item.msg, msg.length());

                cli.sendData(msg);
            });
            cli.tags["logs_observing_id"] = String(observingId);
        }
        else if (sub == "stop")
        {
            if (!cli.tags.count("logs_observing_id"))
            {
                cli.sendData("Not currently listening to logs.");
                onReady();
                return;
            }

            auto observingId = cli.tags["logs_observing_id"].toInt();
            this->logService.OnLog.stopListening(observingId);
            cli.tags.erase("logs_observing_id");
        }
        else
        {
            cli.sendData("Unknown logs subcommand: " + sub);
            cli.sendData("Available: listen, stop");
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
