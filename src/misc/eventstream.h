#ifndef __OBSERVABLELIST_H__
#define __OBSERVABLELIST_H__
#include <map>
#include <functional>
#include <HardwareSerial.h>

using namespace std;

template <typename T>
class EventStream{
private:
    int idCount = 0;
    std::map<int, function<void(T)>> list;
public:

    int listen(function<void(T)> observer)
    {
        int id = idCount++;
        list[id] =observer;

        return id;
    }
    int subscribe (function<void(T)> observer)
    {
        return listen(observer);
    }

    void stream(T item)
    {
        for(auto &c: list)
            c.second(item);
    }
    void emit(T item){ stream(item); }

    void stopListen(int id)
    {
        if (list.count(id))
            list.erase(id);
    }


};

template <typename T>
class EventStream_Stateful{
private:
    int idCount = 0;
    std::map<int, function<void(T)>> list;

    T lastData;
public:

    EventStream_Stateful(){}
    EventStream_Stateful(T initialState)
    {
        lastData = initialState;
    }

    void listen(function<void(T)> observer, bool callWithLastStreamedData = true)
    {
        int id = idCount++;
        list[id] =observer;
        observer(lastData);
    }

    void stream(T item)
    {
        lastData = item;
        for(auto &c: list)
            c.second(item);
    }
    void emit(T item){ stream(item); }

    T getLastStreamedData()
    {
        return lastData;
    }

    void stopListen(int id)
    {
        if (list.count(id))
            list.erase(id);
    }
};

#endif
