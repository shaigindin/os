//
// Created by shai gindin on 13/04/2020.
//

#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <sys/time.h>
#include <signal.h>
#include "Thread.h"
#include "QueueWrapper.h"

#define READY 1
#define RUNNING 2
#define BLOCK 3
#define MICROSECONDS_TO_SECONDS 1000000
#define FROM_FUTURE 5
#define SUCCESS 0
#define FAILURE -1
#define IN_READY_QUEUE true
#define NOT_IN_READY_QUEUE false
#define AVAILABLE 0
#define OCCUPIED 1
#define MAIN_THREAD_ID 0
#define DEAD -2

std::map<int, Thread *> threadList;
std::set<int> blocked;
std::set<int> all;
int killMe;
bool hasToKill;
int lastThreadNumber;

int availableID[MAX_THREAD_NUM]{0};
int *_quantum_usecs;
struct sigaction sa;
struct itimerval timer;
sigset_t set;
int totalQuantomCounter{};

// we did it because if you hold queue a static var it might cause unexpected problems
QueueWrapper* readyQueue;

/**
 * this function remove thread number from the ReadyQueue
 * @param tid
 */
void removeThreadFromReadyQueue(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    QueueWrapper tempQueue;
    while (!(*readyQueue).empty()) {
        if ((*readyQueue).front() != tid) {
            tempQueue.push((*readyQueue).front());
        }
        (*readyQueue).pop();
    }
    (*readyQueue) = tempQueue;
}

/**
 * remove thread from list blcoked / All
 * @param tid
 * @param list1
 */
void removeFromList(int tid, std::set<int> *list1) {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    std::set<int>::iterator it1{};
    for (auto x = list1->begin(); x != list1->end(); ++x) {
        if (*x == tid) {
            it1 = x;
        }
    }
    list1->erase(it1);
}

/**
 * kill the process and free it memory
 * @param tid
 * @return
 */
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
    return SUCCESS;
}
/**
 * print error to stderr
 * @return
 */
int noTid() {
    std::cerr << "thread library error: there is no thread with given id" << std::endl;
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return FAILURE;
}

/**
 * generates thread id by rules
 * @return
 */
int generateID() {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (availableID[i] == AVAILABLE) {
            availableID[i] = OCCUPIED;
            return i;
        }
    }
    return FAILURE;
}

/**
 * this helper function set the timer to signaling each amount of time
 * @param priority
 * @return
 */
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

/**
 * this function terminates all the thread except the running thread
 * @param remianTid
 */
void terminateAll(int remianTid)
{
    sigprocmask(SIG_BLOCK, &set, nullptr);
    for (auto const& x : threadList)
    {
        if (x.first != remianTid)
        {
            delete x.second;
        }
    }
}

/**
 * this is the time handler. each time there is a SIGVTIME signal. handler will take care of it.
 * @param sig
 */
void timer_handler(int sig) {
    //we knows it autmaticly blocks SIGVTIME but we read in STACKOVERFLOW that there are edge cases
    sigprocmask(SIG_BLOCK, &set, nullptr);

    //because when compiling with flag Werror it unused parameter and sa.handler must get void     (*sa_handler)(int);
    sig++;

    //if handler called with SIGVTALRM signal when threads quantom time was finished
    Thread *currentThread = threadList[(*readyQueue).front()];

    //saves the registers or jumping from the future
    int ret_val = sigsetjmp(currentThread->env, 1);
    if (ret_val == FROM_FUTURE) {
        if (hasToKill)
        {
            killTid(killMe);
            sigprocmask(SIG_BLOCK, &set, nullptr);
            hasToKill = false;
        }
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return;
    }

    //takes the current thread which now ended and turn it back to ready mode
    // ** if handler called from block func it means the current thread should be blocked and therefore skip this **
    if (currentThread->getState() == RUNNING) {
        currentThread->setState(READY);

        // push the finished thread to the be the last in the queue
        (*readyQueue).push((*readyQueue).front());
        (*readyQueue).pop();
    }
    else if (currentThread->getState() == BLOCK || currentThread->getState() == DEAD) {
        currentThread->setInQueue(false);
        (*readyQueue).pop();
    }

    currentThread = threadList[(*readyQueue).front()];
    currentThread->setState(RUNNING);
    currentThread->incCounter();
    totalQuantomCounter++;
    setTimerWithPriority(currentThread->getPriority());
    siglongjmp(currentThread->env, FROM_FUTURE);
}

/**
 * helper function to check if user array values are postivies
 * @param quantum_usecs  - the array pointer
 * @param size - the size of the array
 * @return failure / success
 */
int checkUserInput(const int *quantum_usecs, int size)
{
    if (size <= 0){
        std::cerr << "thread library error: invalid size of array\n";
        return FAILURE;
    }
    //dealing with non positive quantoms
    for (int i = 0; i < size; ++i) {
        if (quantum_usecs[i] <= 0) {
            std::cerr << "thread library error: uthread_init called with non-positive integer array\n";
            return FAILURE;
        }
    }
    return SUCCESS;
}


/**
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * an array of the length of a quantum in micro-seconds for each priority.
 * It is an error to call this function with an array containing non-positive integer.
 * size - is the size of the array.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int *quantum_usecs, int size) {

    //check user input
    if (checkUserInput(quantum_usecs, size) == FAILURE)
    {
        return FAILURE;
    }
    _quantum_usecs = quantum_usecs;


    //after researching, it was recommended to allocate memory in the heap for Queue containers and wrap
    // them with class
    readyQueue = new QueueWrapper();

    //initiate the set of alarams with SIGVTALARM
    sigemptyset(&set);
    sigaddset(&set, SIGVTALRM);

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
    auto mainThread = new Thread(mainThereadID, _quantum_usecs[0], nullptr);
    mainThread->setState(RUNNING);
    totalQuantomCounter++;
    mainThread->incCounter();

    // insert main thread to queue
    (*readyQueue).push(mainThereadID);
    threadList[mainThereadID] = mainThread;
    all.insert(MAIN_THREAD_ID);

    //set the timer to the quantom of main thread and return
    setTimerWithPriority(mainThread->getPriority());
    return SUCCESS;
}

/**
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * priority - The priority of the new thread.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(), int priority) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    //creating the new thread
    int newThereadID = generateID();

   // if there are already MAXIMUM threads active
    if (newThereadID == FAILURE) {
        std::cerr << "thread library error: you reached the max number of threads" << std::endl;
        return FAILURE;
    }

    //creating new thread in Heap
    Thread *newThread = new Thread(newThereadID, _quantum_usecs[priority], f);

    // set the new thread to ready mode and insert to ready qeueu
    newThread->setState(READY);
    all.insert(newThereadID);
    (*readyQueue).push(newThereadID);
    threadList[newThereadID] = newThread;


    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return newThereadID;
}

/**
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) ) {
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
        threadList[tid]->setInQueue(NOT_IN_READY_QUEUE);
        blocked.insert(tid);
        //saves the registers or jumping from the future
        timer_handler(50);
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return SUCCESS;

    } else if (!blocked.count(tid)) {
        threadList[tid]->setState(BLOCK);
        threadList[tid]->setInQueue(NOT_IN_READY_QUEUE);
        removeThreadFromReadyQueue(tid);
        sigprocmask(SIG_BLOCK, &set, nullptr);
        blocked.insert(tid);
    }

    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

/**
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    // if tid is not exist
    if (!threadList.count(tid)) {
        return noTid();
    }

    // if the thread in the block list
    if (blocked.count(tid)) {
        removeFromList(tid, &blocked);
        threadList[tid]->setInQueue(IN_READY_QUEUE);
        threadList[tid]->setState(READY);
        (*readyQueue).push(tid);
    }

    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

/**
 * terminates the entire program
 */
void terminateProgram()
{
    Thread* lastThread = threadList[lastThreadNumber];
    terminateAll(lastThreadNumber);
    sigprocmask(SIG_BLOCK, &set, nullptr);
    delete readyQueue;
    delete lastThread;
    exit(SUCCESS);
}


/**
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid) || threadList[tid]->getState() == DEAD) {
        return noTid();
    }

    // if tid is main thread
    if (tid == 0) {
        lastThreadNumber = (*readyQueue).front();
        terminateProgram();
    }

    if (threadList[tid]->getState() == RUNNING) {
        // mark this thread finished and put it last
        threadList[tid]->setState(DEAD);
        killMe = tid;
        hasToKill=true;
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        timer_handler(6);
    }
    else {
        //the running thread x terminates thread y
        killTid(tid);
        sigprocmask(SIG_BLOCK, &set, nullptr);
    }

    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

/**
 * Description: This function changes the priority of the thread with ID tid.
 * If this is the current running thread, the effect should take place only the
 * next time the thread gets scheduled.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_change_priority(int tid, int priority) {
    sigprocmask(SIG_BLOCK, &set, nullptr);

    // if tid is not exist
    if (!threadList.count(tid)) {
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        return noTid();
    }
    threadList[tid]->setPriority(_quantum_usecs[priority]);
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return SUCCESS;
}

/**
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    sigprocmask(SIG_BLOCK, &set, nullptr);
    int i = (*readyQueue).front();
    sigprocmask(SIG_UNBLOCK, &set, nullptr);
    return i;
}

/**
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
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

/**
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() {
    return totalQuantomCounter;
}
