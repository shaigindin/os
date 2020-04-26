//
// Created by shai gindin on 13/04/2020.
//

#include <vector>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sys/time.h>
#include <signal.h>
#include "Thread.h"

#define MICROSECONDS_TO_SECONDS 1000000
#define FROM_FUTURE 5
#define SUCCESS 0
#define MICRO_TO_SEC 1000000
#define FAILURE -1
#define IN_READY_QUEUE true
#define NOT_IN_READY_QUEUE false
#define AVAILABLE 0
#define OCCUPIED 1
#define MAIN_THREAD_ID 0
#define FINNISHED -1
#define DEAD -2

std::map<int, Thread *> threadList;
std::queue<int> readyQueue;
std::set<int> blocked;
std::set<int> all;
int killMe;
bool hasToKill;

int availableID[MAX_THREAD_NUM]{0};
int *_quantum_usecs;
int _size;
struct sigaction sa;
struct itimerval timer;
sigset_t set;
int totalQuantomCounter{};

void removeThreadFromReadyQueue(int tid) {
    std::queue<int> tempQueue;
    while (!readyQueue.empty()) {
        if (readyQueue.front() != tid) {
            tempQueue.push(readyQueue.front());
        }
        readyQueue.pop();
    }
    readyQueue = tempQueue;
}


void removeFromList(int tid, std::set<int> *list1) {
    std::set<int>::iterator it1{};
    for (auto x = list1->begin(); x != list1->end(); ++x) {
        if (*x == tid) {
            it1 = x;
        }
    }
    list1->erase(it1);
}


int killTid(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    // if thread in block list
    if (blocked.count(tid)) {
        removeFromList(tid, &blocked);
    }
    removeFromList(tid, &all);
    // if the thread in the ready queue
    if (threadList[tid]->isInQueue()) {
        removeThreadFromReadyQueue(tid);
    }
    availableID[tid] = AVAILABLE;
    delete threadList[tid];
    threadList.erase(tid);
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

int noTid() {
    std::cerr << "thread library error: there is no thread with given id" << std::endl;
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return FAILURE;
}


int generateID() {
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (availableID[i] == AVAILABLE) {
            availableID[i] = OCCUPIED;
            return i;
        }
    }
    return FAILURE;
}

int setTimerWithPriority(int priority) {

    timer.it_value.tv_sec = priority / MICROSECONDS_TO_SECONDS;
    timer.it_value.tv_usec = priority % MICROSECONDS_TO_SECONDS;
    timer.it_interval.tv_sec = priority / MICROSECONDS_TO_SECONDS;
    timer.it_interval.tv_usec = priority % MICROSECONDS_TO_SECONDS;
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
        std::cerr << "system error: system call setitimer failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    return SUCCESS;
}

void timer_handler(int sig) {
    //because when compiling with flag Werror it unused parameter and sa.handler must get void     (*sa_handler)(int);
    sig++;
    // if handler called from uthread terminate after killing all other threads except running one
    if (threadList[readyQueue.front()]->getState() == FINNISHED) {
        delete threadList[readyQueue.front()];
        exit(SUCCESS);
    }

    //if handler called with SIGVTALRM signal when threads quantom time was finished
    Thread *currentThread = threadList[readyQueue.front()];

    //saves the registers or jumping from the future
    int ret_val = sigsetjmp(currentThread->env, 1);
    if (ret_val == FROM_FUTURE) {
        if (hasToKill)
        {
            killTid(killMe);
            hasToKill = false;
        }
        return;
    }

    //takes the current thread which now ended and turn it back to ready mode
    // ** if handler called from block func it means the current thread should be blocked and therefore skip this **
    if (currentThread->getState() == RUNNING) {
        currentThread->setState(READY);
        // push the finished thread to the be the last in the queue
        readyQueue.push(readyQueue.front());
        readyQueue.pop();
    }
    while (threadList[readyQueue.front()]->getState() == BLOCK || threadList[readyQueue.front()]->getState() == DEAD) {
        if (threadList[readyQueue.front()]->getState() == BLOCK) {
            //takes the thread out from queue and into the block list
            currentThread = threadList[readyQueue.front()];
            currentThread->setInQueue(NOT_IN_READY_QUEUE);
            blocked.insert(readyQueue.front());
            readyQueue.pop();
        } else {
            if (!threadList[readyQueue.front()]->getKillerFlag()) {
                threadList[readyQueue.front()]->setKillerFlag();
                readyQueue.push(readyQueue.front());
                readyQueue.pop();
            } else {
                killTid(readyQueue.front());
            }
        }

    }
    currentThread = threadList[readyQueue.front()];
    currentThread->setState(RUNNING);
    currentThread->incCounter();
    totalQuantomCounter++;
    setTimerWithPriority(currentThread->getPriority());
    siglongjmp(currentThread->env, FROM_FUTURE);
}


int uthread_init(int *quantum_usecs, int size) {
    _quantum_usecs = quantum_usecs;
    _size = size;
    if (size <= 0){
        std::cerr << "thread library error: invalid size of array\n";
        return FAILURE;
    }
    for (int i = 0; i < size; ++i) {
        if (quantum_usecs[i] <= 0) {
            std::cerr << "thread library error: uthread_init called with non-positive integer array\n";
            return FAILURE;
        }
    }
    //making the timer_handler to intiate when there is signal from the timer
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
        std::cerr << "system error: system call sigaction failed." << std::endl;
        exit(EXIT_FAILURE);
    }


    //creating the main thread
    int mainThereadID = generateID();

    // creates the main thread and mark it as running
    // **dont forget to free from memory main thread**
    Thread *mainThread = new Thread(mainThereadID, _quantum_usecs[0], nullptr);
    mainThread->setState(RUNNING);
    totalQuantomCounter++;
    mainThread->incCounter();

    // insert main thread to queue
    readyQueue.push(mainThereadID);
    threadList[mainThereadID] = mainThread;
    all.insert(MAIN_THREAD_ID);

    //set the timer to the quantom of main thread and return
    setTimerWithPriority(mainThread->getPriority());
    return SUCCESS;
}


int uthread_spawn(void (*f)(), int priority) {

    sigprocmask(SIG_BLOCK, &set, nullptr);
    //creating the new thread
    int newThereadID = generateID();
    if (newThereadID == FAILURE) {
        std::cerr << "thread library error: you reached the max number of threads" << std::endl;
        fflush(stdout);
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return FAILURE;
    }

    // **dont forget to free from memory main thread**
    Thread *newThread = new Thread(newThereadID, _quantum_usecs[priority], f);

    // set the new thread to ready mode and insert to ready qeueu
    newThread->setState(READY);
    all.insert(newThereadID);
    readyQueue.push(newThereadID);
    threadList[newThereadID] = newThread;


    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return newThereadID;
}

int uthread_block(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState() == DEAD) {
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return noTid();
    }

    // if tid is main thread
    if (tid == 0) {
        std::cerr << "thread library error: you can't block main thread" << std::endl;
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return FAILURE;
    }

    // if thread asks to block itself
    if (threadList[tid]->getState() == RUNNING) {
        threadList[tid]->setState(BLOCK);
        //saves the registers or jumping from the future
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        timer_handler(50);
        return SUCCESS;
    } else if (!blocked.count(tid)) {
        threadList[tid]->setState(BLOCK);
        blocked.insert(tid);
    }

    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

int uthread_resume(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState() == DEAD) {
        return noTid();
    }

    // if the thread in the block list
    if (blocked.count(tid)) {
        // if the thread in the block list and already moved out from ready queue
        if (threadList[tid]->isInQueue() == NOT_IN_READY_QUEUE) {
            removeFromList(tid, &blocked);
            threadList[tid]->setInQueue(IN_READY_QUEUE);
        }
            // if the thread still in ready queue
        else {
            removeThreadFromReadyQueue(tid);
        }
        threadList[tid]->setState(READY);
        readyQueue.push(tid);
    }
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

int uthread_terminate(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState() == DEAD) {
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return noTid();
    }

    // if tid is main thread
    if (tid == 0) {
        for (int x : all) {
            if (x != readyQueue.front()) {
                killTid(x);
            }
        }
        threadList[readyQueue.front()]->setState(FINNISHED);
        timer_handler(FINNISHED);
    } else if (threadList[tid]->getState() == RUNNING) {
        // mark this thread finished and put it last
        threadList[tid]->setState(DEAD);
        killMe = tid;
        hasToKill=true;
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        timer_handler(6);
    } else {
        //the running thread x terminates thread y
        killTid(tid);
    }

    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}


int uthread_change_priority(int tid, int priority) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState() == DEAD) {
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return noTid();
    }
    threadList[tid]->setPriority(_quantum_usecs[priority]);
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}


int uthread_get_tid() {
    return readyQueue.front();
}

int uthread_get_quantums(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState()== DEAD) {
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return noTid();
    }
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return threadList[tid]->getQuantomCounter();
}


int uthread_get_total_quantums() {
    return totalQuantomCounter;

}
