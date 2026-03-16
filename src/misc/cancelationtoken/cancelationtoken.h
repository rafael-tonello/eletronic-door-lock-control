#ifndef __CANCELATIONTOKEN__H__ 
#define __CANCELATIONTOKEN__H__ 
#include <atomic>
#include "../eventstream.h" 
class CancelationToken { 
private:
    std::atomic<bool> state;

    void changeState(bool newState);
public: 
    CancelationToken(); 
    ~CancelationToken(); 

    /**
     * @brief resets the cancelation state to 'not canceled'. After this, the method 'wasTheCancellationRequested' will returns 'false'
     * 
     */
    void reset();

    /**
     * @brief Sets the cancelation state as 'canceled'. After this, the method 'wasTheCancellationRequested' will returns 'true'
     * 
     */
    void cancel();

    /**
     * @brief Returns the current cancelation state.
     * 
     * @return true 
     * @return false 
     */
    bool wasTheCancellationRequested();

    /**
     * @brief an event that can be used to observe the changes in the cancelation state. Is an alternative to pulling of 'wasTheCancellationRequested' method
     * 
     */
    EventStream<bool> onChangeEvent;
}; 
 
#endif 
