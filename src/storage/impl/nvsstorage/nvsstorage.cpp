#include "nvsstorage.h"
#if defined(ESP32)

NvsStorage::NvsStorage(Scheduler *scheduler)
{
    this->scheduler = scheduler;

    // Inicializa a NVS apenas uma vez, se ainda não foi feita
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Apaga a partição NVS e reinicializa
        nvs_flash_erase();
        nvs_flash_init();
    }
    
}

NvsStorage::~NvsStorage()
{
    
}

String NvsStorage::get(String key, String defaulValue)
{
    key = shortenKey(key);

    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        // Falha ao abrir NVS, retorna valor default
        return defaulValue;
    }

    // Primeiro lê o tamanho da string armazenada
    size_t required_size = 0;
    err = nvs_get_str(handle, key.c_str(), nullptr, &required_size);
    if (err != ESP_OK || required_size == 0) {
        nvs_close(handle);
        return defaulValue;
    }

    // Aloca buffer para ler o valor
    char *buf = new char[required_size];
    err = nvs_get_str(handle, key.c_str(), buf, &required_size);
    String value = defaulValue;
    if (err == ESP_OK) {
        value = String(buf);
    }

    delete[] buf;
    nvs_close(handle);
    return value;
}

void NvsStorage::set(String key, String value)
{
    key = shortenKey(key);

    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        // Falha ao abrir NVS, talvez logar ou tratar erro
        return;
    }

    err = nvs_set_str(handle, key.c_str(), value.c_str());
    if (err == ESP_OK) {
        nvs_commit(handle);  // Confirma as alterações
    }
    nvs_close(handle);
}


bool NvsStorage::has(String key)
{
    key = shortenKey(key);
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    size_t required_size = 0;
    err = nvs_get_str(handle, key.c_str(), NULL, &required_size);
    nvs_close(handle);
    return (err == ESP_OK);
}

void NvsStorage::remove(String key)
{
    key = shortenKey(key);
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_key(handle, key.c_str());
        nvs_commit(handle);
        nvs_close(handle);
    }
}

//return a string with at max 15 characters (Preferences lib accept keys with max 15 chars)
String NvsStorage::shortenKey(String key){
    //this algorithm working converting two chars in one. The working occurs from the first to the last char. If the end of the key is reached, 
    //the process is repeasted from the start of the key. The process ends when the key has less than 15 chars. 
    //
    //this algorithm working by changes the less significant part of the key and trying to key most significant part of the key unchanged.
    //if the key is too big, the algorithm will change the entirekey.

    int workingIndex = 0;
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

void NvsStorage::eraseAll()
{
    // Erase entire NVS namespace "storage"
    nvs_flash_erase_partition("storage");
    // ou, se "storage" não for partição, pode ser apagado todo o NVS (mais agressivo):
    // nvs_flash_erase();
    // Depois re-inicia para abrir de novo, se precisar:
    nvs_flash_init();
}

Promise<String>::smp_t NvsStorage::pGet(String key, String defaultValue){
    auto ret = Promise<String>::get_smp();
    this->scheduler->run([=](){
        auto data = this->get(key, defaultValue);
        ret->resolve(data);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);

    return ret;
}

Promise<TpNone>::smp_t NvsStorage::pSet(String key, String value){
    auto ret = Promise<TpNone>::get_smp();
    this->scheduler->run([=](){
        this->set(key, value);
        ret->resolve(Nothing);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    
    return ret;
}

Promise<bool>::smp_t NvsStorage::pHas(String key){
    auto ret = Promise<bool>::get_smp();
    this->scheduler->run([=](){
        auto hasValue = this->has(key);
        ret->resolve(hasValue);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    return ret;
}

Promise<TpNone>::smp_t NvsStorage::pRemove(String key){
    auto ret = Promise<TpNone>::get_smp();
    this->scheduler->run([=](){
        this->remove(key);
        ret->resolve(Nothing);
    }, "", DEFAULT_PRIORITIES::VERY_LOW);
    return ret;
}

#endif
