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
        vector<int> ids;
        ids.reserve(list.size());
        for (auto &c: list)
            ids.push_back(c.first);

        for (auto id: ids)
        {
            if (list.count(id))
                list[id](item);
        }
    }
    void emit(T item){ stream(item); }

    void stopListening(int id)
    {
        if (list.count(id))
            list.erase(id);
    }
    void unsubscribe(int id)
    {
        stopListening(id);
    }


};

template <typename T>
class EventStream_Stateful{
private:
    int idCount = 0;
    std::map<int, function<void(T)>> list;

public:

    T lastData;
    
    EventStream_Stateful(){}
    EventStream_Stateful(T initialState)
    {
        lastData = initialState;
    }

    int listen(function<void(T)> observer, bool callWithLastStreamedData = true)
    {
        int id = idCount++;
        list[id] =observer;
        if (callWithLastStreamedData)
            observer(lastData);
        return id;
    }

    void stream(T item)
    {
        lastData = item;
        vector<int> ids;
        ids.reserve(list.size());
        for (auto &c: list)
            ids.push_back(c.first);

        for (auto id: ids)
        {
            if (list.count(id))
                list[id](item);
        }
    }
    void emit(T item){ stream(item); }

    T getLastData()
    {
        return lastData;
    }
    
    T getCurrentState()
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
