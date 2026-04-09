#ifndef __CONFIGS__H__ 
#define __CONFIGS__H__ 

#include <Arduino.h>
#include <storage/istorage.h>
#include <map>
#include <functional>

//this is bassicaly a cache for istorage, using an internal map for fast data access
class Configs { 
private:
    std::map<String, String> cache;
    IStorage &storage;

public:
    void set(String name, String value);
    String get(String name, String defaultValue);
    //void listenConfig(String name, function<void(String)> f, bool fireFImediatelly=true);
    Configs(IStorage &storage); 
}; 
 
#endif 
