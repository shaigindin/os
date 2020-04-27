//
// Created by shai gindin on 13/04/2020.
//

#ifndef OS_EX2_THREAD_H
#define OS_EX2_THREAD_H


#include <setjmp.h>
#include "uthreads.h"

class Thread
{
private:
    sigset_t set;
    int _tid,_priority;
    char* _stackMemoryAlocPointer;
    bool _inReadyQueu;
    void (*_f)();
    int _state;
    bool _kill_me_flag;
    int _quantomCounter;

public:
// public fields
    sigjmp_buf env{};
// public constructor & destructor
    Thread(int tid, int priority, void (*f)());
    ~Thread();
//public methods
    bool isInQueue();
    void incCounter();
// Getters
    int getPriority();
    int getQuantomCounter();
    int getState();
    const bool& getKillerFlag() const;
// Setters
    void setState(int state);
    void setPriority(int priority);
    void setInQueue(bool q);
    void setKillerFlag();
};



#endif //OS_EX2_THREAD_H
