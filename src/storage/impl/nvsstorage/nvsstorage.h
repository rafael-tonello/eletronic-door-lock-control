#ifndef __NVSSTORAGE__H__
#define __NVSSTORAGE__H__

//check for esp32
#if defined(ESP32)
    


#include <scheduler/scheduler.h>
#include "istorage.h"

#include <nvs_flash.h>
#include <nvs.h>
#include "esp_crc.h"

class NvsStorage: public IStorage {
private:
    Scheduler *scheduler;
public:
    NvsStorage(Scheduler *scheduler);
    ~NvsStorage();
    String get(String key, String defaulValue = "") override;
    void set(String key, String value) override;
    bool has(String key) override;
    void remove(String key) override;
    void eraseAll() override;

    Promise<String>::smp_t pGet(String key, String defaultValue = "") override;
    Promise<TpNone>::smp_t pSet(String key, String value) override;
    Promise<bool>::smp_t pHas(String key) override;
    Promise<TpNone>::smp_t pRemove(String key) override;
    static String shortenKey(String key);
};

#endif
#endif