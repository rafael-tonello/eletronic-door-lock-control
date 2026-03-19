#ifndef __ISTORAGE__H__
#define __ISTORAGE__H__
#include <Arduino.h>
#include <scheduler/misc/promise.h>

using namespace ProcessChain;

class IStorage{
public:
    virtual String get(String key, String defaulValue = "") = 0;
    virtual void set(String key, String value) = 0;
    virtual bool has(String key) = 0;
    virtual void remove(String key) = 0;
    virtual void eraseAll() = 0;
    

    virtual Promise<String>::smp_t pGet(String key, String defaultValue = "") = 0;
    virtual Promise<TpNone>::smp_t pSet(String key, String value) = 0;
    virtual Promise<bool>::smp_t pHas(String key) = 0;
    virtual Promise<TpNone>::smp_t pRemove(String key) = 0;

};

#endif
