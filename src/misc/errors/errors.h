#ifndef __ERRORS__H__ 
#define __ERRORS__H__ 
#include <WString.h>


using namespace std;

using Error = String;
class Errors { 
public:
    static Error NoError;
    static Error TimeoutError;
    static Error UnknownError;
    static Error NotFound;
    static Error AlreadyWorking;
    
    static Error createError(String message){ return message; }
    static Error DerivateError(Error baseError, String message){ 
        baseError.replace("  >", "    >");
        message = message + "\n  >" + baseError;
        return message; 
    }
};

template <typename T>
class ResultWithStatus{
public:
    T result;
    Error status = Errors::NoError;

    ResultWithStatus(){}
    ResultWithStatus(T result, Error error): result(result), status(error){}
};
 
#endif 
