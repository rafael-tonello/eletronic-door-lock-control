#ifndef __ORDEREDRUNNER__H__ 
#define __ORDEREDRUNNER__H__ 

#include <map>
#include <functional>
#include "promise.h"

/**
 * OrderedRunner allow the call of async function in order, waiting for long runing ones.
 * In the example bellow, don't matter the order when the asyncProcess() calls returns.. The internal functions (controlled by 'or.run') will
 * run int the order specified by the first argument of 'or.run'
 * 
 * 
 * 
 * 
 * 
 * 
 * OrderedRunner or;
 * 
 * int main()
 * {
 *      asyncProcess()->then<TpNothing>([=](TpNothing _void){
 *          or.run(1, [=](){
 *              // ... do something ...
 *          });
 *      });
 * 
 *      asyncProcess()->then<TpNothing>([=](TpNothing _void){
 *          or.run(2, [=](){
 *              // ... do something ...
 *          });
 *      });
 * 
 *      asyncProcess()->then<TpNothing>([=](TpNothing _void){
 *          or.run(3, [=](){
 *              // ... do something ...
 *          });
 *      });
 * 
 *      asyncProcess()->then<TpNothing>([=](TpNothing _void){
 *          or.run(4, [=](){
 *              // ... do something ...
 *          });
 *      });
 * 
 *      asyncProcess()->then<TpNothing>([=](TpNothing _void){
 *          or.run(5, [=](){
 *              // ... do something ...
 *          });
 *      });
 * 
 *      or.wait([](){
 *          //all functions runned
 *      });
 * 
 * }
*/

namespace ProcessChain {
    using namespace std; 
    class OrderedRunner{
    private:
        int curr = 1;
        bool done = false;
        IIOHAL_IO_ID_TYPE<int, function<void()>> pending;
        function<void()> onDone = [](){};
        function<void(function<void()>)> schedule = [](function<void()> f){f();};
    public:
        OrderedRunner(){}
        OrderedRunner(function<void(function<void()>)> scheduleFunction): schedule(scheduleFunction){}

        void run(int order, function<void()> f)
        {
            done = false;
            if (order == curr)
            {
                schedule([f](){
                    f();
                });

                curr++;

                if (pending.count(order +1))
                {
                    auto tmpF = pending[order+1];
                    pending.erase(order+1);
                    run(order+1, tmpF);
                }

                if (pending.size() == 0)
                {
                    done = true;
                    schedule([=](){
                        onDone();
                    });
                }
            }
            else
                pending[order] = f;

        }

        void wait(function<void()> onDone)
        {
            this->onDone = onDone;
            if (done)
                this->onDone();
        }

        Promise<TpNothing>::smp_t waitWithProm(){
            auto ret = Promise<TpNothing>::get_smp();

            this->wait([ret](){
                ret->post(Nothing);
            });

            return ret;
        }
    };
}
#endif 
