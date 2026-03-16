#include  "cancelationtoken.h" 
 
CancelationToken::CancelationToken() 
{ 
    this->changeState(false);
} 
 
CancelationToken::~CancelationToken() 
{ 
     
}

void CancelationToken::reset()
{
    this->changeState(false);
}

void CancelationToken::cancel()
{
    this->changeState(true);
}

void CancelationToken::changeState(bool newState)
{
    this->state = newState;
    this->onChangeEvent.stream(newState);
}

bool CancelationToken::wasTheCancellationRequested()
{
    return this->state; 
}
