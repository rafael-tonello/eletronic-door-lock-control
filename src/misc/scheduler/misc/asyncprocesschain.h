#ifndef __ASYNCPROCESSCHAIN__H__ 
#define __ASYNCPROCESSCHAIN__H__

#include <functional>
#include <memory>
#include <WString.h>
#include <HardwareSerial.h>
 
using namespace std;

namespace ProcessChain{

    // template <typename T>
    // class IAsyncProcessChain{
    // public:
    //     template <typename S>
    //     virtual shared_ptr<IAsyncProcessChain<S>> then(function<S(T)> ff) = 0;

    //     template <typename S> 
    //     virtual shared_ptr<IAsyncProcessChain<S>> then(function<S(T, shared_ptr<IAsyncProcessChain<S>> nextChainNodeOrPromiseToResume)> ff) = 0;
    //     virtual void post(T data) = 0;
    // };

    using TpNone = void*;
    using RetNone = void*;
    extern TpNone None;

    using TpNothing = void*;
    using RetNothing = void*;
    extern TpNothing Nothing;

    template <typename T>
    class AsyncProcessChain{
    private:
        function<void(T)> f1 = [](T a){ };
        bool f_is_original = true;
        bool alreadyPosted = false;
        T posted;
        String name;
    public:

        AsyncProcessChain(String promName = ""): name(promName){
            
        }

        AsyncProcessChain(const AsyncProcessChain& cp){
            this->f1 = cp.f1;
            this->f_is_original = cp.f_is_original;
            this->alreadyPosted = cp.alreadyPosted;
            this->posted = cp.posted;
            this->name = cp.name;
        }
        ~AsyncProcessChain(){

        }

        //simplest way to set 'then'. No chained return
        void then(function<void(T)> ff)
        {
            if (!f_is_original)
                Serial.println("[warning] [AsysncProcessChain] 'Then' method called twice! " + this->name);

            f1=ff;

            this->f_is_original = false;

            if (this->alreadyPosted)
            {
                f1(posted);
            }
        }

        //ignore the data and just call the function
        void then(function<void()> ff){
            this->then([=](T data){
                ff();
            });
        }

        //sets the 'then' function and return a chained promise
        template <typename S>
        shared_ptr<AsyncProcessChain<S>> then(function<S(T)> ff)
        {
            shared_ptr<AsyncProcessChain<S>> resp = shared_ptr<AsyncProcessChain<S>>(new AsyncProcessChain<S>(name + ".child"));

            this->then([=](T data){
                auto ret = ff(data);
                resp->post(ret);
            });

            return resp;
        }


        T getData(){
            return posted;
        }
        
        
        //resolve the promise with the data
        void post(T data, String debugInfo = "")
        {
            if (alreadyPosted)
            {
                Serial.println("[error][AsyncProcessChain] posting data in already posted asyncprocesschain (promise)");
                return;
            };
            posted = data;
            this->alreadyPosted = true;
            if (this->f_is_original)
                return;

            //this->callFunction(data);
            this->f1(data);
        }

        //resolve the promise with the data
        void resolve(T data){
            this->post(data);
        }
    };
}


 
#endif 
