#include <signal.h>
#include <iostream>
#include "Thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

Thread::Thread(int tid , int priority , void (*f)()) : _kill_me_flag(false)
{
    _tid = tid;
    _priority = priority;
    _f = f;
    _stackMemoryAlocPointer = new char[STACK_SIZE];
    _state = READY;
    address_t sp, pc;
    sp = (address_t)_stackMemoryAlocPointer + STACK_SIZE - sizeof(address_t);
    pc = (address_t)_f;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
    _inReadyQueu = true;
    _quantomCounter = 0;
}

Thread::~Thread()
{

    delete[] _stackMemoryAlocPointer;
}

int Thread::getState() {
    return _state;
}

int Thread::getPriority() {
    return _priority;
}

void Thread::setState(int state) {
    _state = state;

}

bool Thread::isInQueue() {
    return _inReadyQueu;
}

void Thread::setInQueue(bool b) {
    _inReadyQueu = b;
}

void Thread::setPriority(int priority) {
    _priority = priority;
}

void Thread::incCounter() {
    _quantomCounter++;
}

int Thread::getQuantomCounter() {
    return _quantomCounter;
}

const bool& Thread::getKillerFlag() const
{
    return this->_kill_me_flag;
}

void Thread::setKillerFlag()
{
    this->_kill_me_flag = true;
}

