#include  "configs.h" 

Configs::Configs(IStorage &storage)
    : storage(storage)
{

}
 
void Configs::set(String name, String value)
{
    cache[name] = value;
    storage.set(name, value);
}

String Configs::get(String name, String defaultValue)
{
    if (cache.count(name))
        return cache[name];
    
    auto valueFromStorage = storage.get(name, defaultValue);
    cache[name] = valueFromStorage;
    return valueFromStorage;
}