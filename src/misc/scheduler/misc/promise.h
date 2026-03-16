#ifndef __PROMISE__H__ 
#define __PROMISE__H__ 
#include <WString.h>
#include <vector>
 
#include "asyncprocesschain.h"

namespace ProcessChain{
    template <typename T>
    class Promise: public AsyncProcessChain<T> { 
        //returna a shared_ptr with a Promise
    public:
        Promise(String name): AsyncProcessChain<T>(name){}

        static shared_ptr<Promise<T>> getSMP(String name = "")
        {
            auto tmp = shared_ptr<Promise<T>>(new Promise<T>(name));
            return tmp;
        }

        
        
        //helpers
            typedef shared_ptr<Promise<T>> smp_t;
            typedef shared_ptr<Promise<T>> shared_memory_t;

            static shared_ptr<Promise<T>> get_shared_memory_promise(String name = ""){return getSMP(name);}

            //internally, this function calls the getSMP function to create a shared memory poniter. Do exacly the same of create_smp
            static shared_ptr<Promise<T>> get_smp(String name = ""){ return getSMP(name); }
            //internally, this function calls the getSMP function to create a shared memory poniter. Do exacly the same of get_smp
            static shared_ptr<Promise<T>> create_smp(String name = ""){ return getSMP(name); }
    };

    using RO_continue = function<void()>;
    using RO_cancel = function<void()>;
    class PromiseUtils{
    public:
        /**
         * @brief this function avois the Haidouken code when the use of a sequence of promises is need.
         * this:
         *      promise1()->then([](){
         *          promise2()->then([](){
         *              promise3()->then([](){
         * 
         *              });
         *          });    
         *      })
         * becomes this:
         *      RunFsInOrder({
         *          [](RO_continue cnt, RO_cancel abort){ promise1()->then([cnt](){ cnt(); }) }
         *          [](RO_continue cnt, RO_cancel abort){ promise2()->then([cnt](){ cnt(); }) }
         *          [](RO_continue cnt, RO_cancel abort){ promise3()->then([cnt](){ cnt(); }) }
         *      });
         * 
         * @param fs is a vector with a list of functions to be executed in sequenece
         * @return Promise<bool>::smp_t returns a shared_memory to a bool promise that receives a 'true' if all function are executed, or 'false' if the the sequence are canceled
         */
        
        static shared_ptr<Promise<bool>> runFsInOrder(vector<function<void(RO_continue, RO_cancel)>> fs){
            Promise<bool>::smp_t result = Promise<bool>::get_smp();

            if (fs.size() == 0)
            {
                result->post(true);
                return result;
            }

            function<void(size_t)> runNext;
            runNext = [fs, result, runNext](size_t pos){
                fs[pos]([fs, pos, result, runNext](){
                    if (pos < fs.size() -1)
                        runNext(pos+1);
                    else
                        result->post(true);
                }, [result](){
                    result->post(false);
                });
            };
            runNext(0);

            return result;
        } 
        // usage examples
        // runFsInOrder({ [](function<void>() continue, function<void()> abort) });
        // 
        // runFsInOrder({
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })},
            // [](function<void()> done, function<void()> abort){ this->findServer()->then<TpNothing>([done](bool sucess){done(); return Nothing; })}
        // });


        //awaits a lists of Promise<TpNothing>::smp_t
        static Promise<TpNothing>::smp_t WaitPromises(vector<Promise<TpNothing>::smp_t> proms)
        {
            auto ret = Promise<TpNothing>::getSMP();
            if (proms.size() > 0)
            {
                int *doneCount = new int(0);
                int *max = new int(proms.size());
                for (auto &c: proms)
                {
                    c->then([=](){
                        *doneCount = *doneCount+1;
                        if (*doneCount >= *max)
                        {
                            delete doneCount;
                            delete max;
                            ret->post(Nothing);
                        }
                    });
                }
            }
            else
                ret->post(Nothing);

            return ret;
        }
    };




    //helper
    template <typename T>
    using PromiseSM = shared_ptr<Promise<T>>;

    //helper
    template <typename T>
    using Shared_ptr_promise = shared_ptr<Promise<T>>;

    //helper
    #define SMP PromiseSM




    
}
 
#endif 
