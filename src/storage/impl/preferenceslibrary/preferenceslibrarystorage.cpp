#include "preferenceslibrarystorage.h"

PreferencesLibraryStorage::PreferencesLibraryStorage(Scheduler *scheduler)
{
    this->scheduler = scheduler;
    storage.begin("storage");
    
}

PreferencesLibraryStorage::~PreferencesLibraryStorage()
{
    storage.end();   
}


String PreferencesLibraryStorage::get(String key, String defaulValue)
{
    key = shortenKey(key);
    String value = storage.getString(key.c_str(), defaulValue);
    
    return value;
}

void PreferencesLibraryStorage::set(String key, String value)
{
    key = shortenKey(key);

    (void)storage.putString(key.c_str(), value);
}

//return a string with at max 15 characters (Preferences lib accept keys with max 15 chars)
String PreferencesLibraryStorage::shortenKey(String key){
    //this algorithm working converting two chars in one. The working occurs from the first to the last char. If the end of the key is reached, 
    //the process is repeasted from the start of the key. The process ends when the key has less than 15 chars. 
    //
    //this algorithm working by changes the less significant part of the key and trying to key most significant part of the key unchanged.
    //if the key is too big, the algorithm will change the entirekey.

    unsigned int workingIndex = 0;
    key.toLowerCase();
    while (true){
        if (key.length() > 15)
        {
            if (workingIndex >= key.length()-1)
                workingIndex = 0;

            int tmp = (int)key[workingIndex];
            tmp += (int)key[workingIndex+1];

            if (tmp > 255)
                tmp = 255 - tmp;

            key = key.substring(0, workingIndex) + (char)tmp + key.substring(workingIndex+2);

            workingIndex++;
        }
        else
            break;
    }

    //convert the key to a string with only valid chars
    String validChars="abcdefghijklmnopqrstuvwxyz0123456789";

    String newKey="";
    for (size_t c = 0; c < key.length(); c++)
    {
        if (validChars.indexOf(key[c]) > -1)
            newKey += key[c];
        else
            newKey += validChars[key[c] % validChars.length()];
    }

    return newKey;
}

//return a string with at max 15 characters (Preferences lib accept keys with max 15 chars)
//String PreferencesLibraryStorage::shortenKey(String key){
//    //this algorithm working reducing the key size in half until it be less than 15 chars
//    //key.toLowerCase();
//
//    if (key.length() > 15)
//    {
//        String newKey="";
//        int tmp = 0;
//        for (int c = 0; c < key.length(); c++)
//        {
//            tmp += (int)key[c];
//
//            if (tmp > 255)
//                tmp = 255 - tmp;
//
//            if ((c+1) % 2 == 0)
//            {
//                newKey += (char)tmp;
//                tmp = 0;
//            }
//
//
//        }
//        if (tmp != 0)
//            newKey += (char)tmp;
//
//        return  shortenKey(newKey);
//    }
//    else
//    {
//        //convert the key to a string with only valid chars
//        String validChars="abcdefghijklmnopqrstuvwxyz0123456789";
//
//        String newKey="";
//        for (size_t c = 0; c < key.length(); c++)
//        {
//            if (validChars.indexOf(key[c]) > -1)
//                newKey += key[c];
//            else
//                newKey += validChars[key[c] % validChars.length()];
//        }
//
//        return newKey;
//    }
//}

bool PreferencesLibraryStorage::has(String key)
{
    key = shortenKey(key);
    return storage.isKey(key.c_str());
}

void PreferencesLibraryStorage::remove(String key)
{
    key = shortenKey(key);
    storage.remove(key.c_str());
}

void PreferencesLibraryStorage::eraseAll()
{
    storage.clear();
}


Promise<String>::smp_t PreferencesLibraryStorage::pGet(String key, String defaultValue){
    auto ret = Promise<String>::get_smp();
    this->scheduler->run([=](){
        auto data = this->get(key, defaultValue);
        ret->resolve(data);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);

    return ret;
}

Promise<TpNone>::smp_t PreferencesLibraryStorage::pSet(String key, String value){
    auto ret = Promise<TpNone>::get_smp();
    this->scheduler->run([=](){
        this->set(key, value);
        ret->resolve(Nothing);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    
    return ret;
}

Promise<bool>::smp_t PreferencesLibraryStorage::pHas(String key){
    auto ret = Promise<bool>::get_smp();
    this->scheduler->run([=](){
        auto hasValue = this->has(key);
        ret->resolve(hasValue);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    return ret;
}

Promise<TpNone>::smp_t PreferencesLibraryStorage::pRemove(String key){
    auto ret = Promise<TpNone>::get_smp();
    this->scheduler->run([=](){
        this->remove(key);
        ret->resolve(Nothing);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    return ret;
}