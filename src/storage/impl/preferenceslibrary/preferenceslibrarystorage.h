#ifndef __PREFERENCESLIBRARYSTORAGE__H__
#define __PREFERENCESLIBRARYSTORAGE__H__
#include "istorage.h"
#include <scheduler/scheduler.h>
#include <Preferences.h>

class PreferencesLibraryStorage: public IStorage{
public:
    PreferencesLibraryStorage();
    ~PreferencesLibraryStorage();
    Preferences storage;
    Scheduler &scheduler;
public: /* IStorage interface */
    PreferencesLibraryStorage(Scheduler &scheduler);
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
