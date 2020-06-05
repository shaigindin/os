//
// Created by shai on 16/05/2020.
//

#include <atomic>
#include <iostream>
#include "MapReduceFramework.h"
#include "Barrier.h"
#include <pthread.h>
//#include <algorithm>

static const char *const SYSTEM_ERROR_MUTEX_FAILED = "system error: mutex failed.";
static const char *const SYSTEM_ERROR_UTHREAD_FAILED = "system error: pthread failed.";
static const int SUCCESS = 0;

/**
 * a struct used as a context for each created thread, to obtain various data.
 */
struct mapThreadContext {
    std::atomic<unsigned int>* atomicCounter3{};
    bool* mapping{};
    bool* shuffeling{};
    bool* finished{};
    IntermediateMap* mapFinal{};
    std::vector<K2*>* uniqe{};
    std::atomic<unsigned int>* atomicCounter{};
    std::atomic<unsigned int>* atomicCounter2{};
    std::atomic<unsigned int>* atomicCounterForFinishReadFromINputVector{};
    std::atomic<unsigned int>* intermediatePairsProduced{};
    std::atomic<unsigned int>* intermediatePairsInMap{};
    const MapReduceClient* client{};
    const InputVec* inputVector{};
    OutputVec* outputVector{};
    unsigned int totalThreadNum{};
    unsigned int threadId{};
    std::vector<IntermediatePair>* threadVector{};
    Barrier* barrier{};
    pthread_mutex_t* mutexArray{};
    pthread_mutex_t* read{};
    pthread_mutex_t* write{};
    std::map<int, std::vector<IntermediatePair>*> mapVectors{};
    IntermediateMap map{};
};

struct Bundle {
    bool* mapping{};
    bool* shuffeling{};
    bool* finished{};
    IntermediateMap* mapFinal{};
    std::vector<K2*>* uniqe{};
    pthread_t* threads{};
    mapThreadContext* contexts{};
    std::atomic<unsigned int>* atomicCounter3{};
    std::atomic<unsigned int>* atomicCounter{};
    std::atomic<unsigned int>* atomicCounter2{};
    std::atomic<unsigned int>* atomicCounterForFinishReadFromINputVector{};
    std::atomic<unsigned int>* intermediatePairsProduced{};
    std::atomic<unsigned int>* intermediatePairsInMap{};
    std::map<int, std::vector<IntermediatePair>*> mapVectors{};
    Barrier* barrier{};
    pthread_mutex_t* mutexArray{};
    int threadNumber{};
    IntermediateMap map{};
    pthread_mutex_t* write{};
    pthread_mutex_t* read{};
    bool wasInWaitFunc{};
} Bundle{};

void lockMutexWrapper(pthread_mutex_t* lock)
{
    if (pthread_mutex_lock(lock) != SUCCESS){
        std::cerr << SYSTEM_ERROR_MUTEX_FAILED << std::endl;
        exit(EXIT_FAILURE);
    }
}

void unlockMutexWrapper(pthread_mutex_t* lock)
{
    if (pthread_mutex_unlock(lock) != SUCCESS){
        std::cerr << SYSTEM_ERROR_MUTEX_FAILED << std::endl;
        exit(EXIT_FAILURE);
    }
}


void mapPhase(mapThreadContext *context)
{
    unsigned int currentIndex = (*(context->atomicCounter))++;
    while (currentIndex < context->inputVector->size()) {
        context->client->map((*(context->inputVector))[currentIndex].first,
                             (*(context->inputVector))[currentIndex].second,
                             context);
        currentIndex = (*(context->atomicCounter))++;
    }
}

void reducePhase(mapThreadContext *context)
{
    unsigned int currentIndex = (*(context->atomicCounter2))++;;
    while (currentIndex < context->uniqe->size()) {
        lockMutexWrapper(context->read);

        K2* key = context->uniqe->at(currentIndex);
        std::vector<V2 *> val = (*context->mapFinal)[key];

        unlockMutexWrapper(context->read);

        context->client->reduce(key, val , context);
        currentIndex = (*(context->atomicCounter2))++;
    }
}

void emptyAllThreadsVectors(mapThreadContext *context)
{
    for (unsigned  int i=0 ; i < (context->totalThreadNum - 1); i++)
    {
        lockMutexWrapper(&context->mutexArray[i]);

        std::vector<IntermediatePair>& threadIVector = *context->mapVectors[i];
        for (auto p: threadIVector)
        {
            context->map[p.first].push_back(p.second);
            (*(context->intermediatePairsInMap))++;
        }
        context->mapVectors[i]->clear();

        unlockMutexWrapper(&context->mutexArray[i]);
    }
}

void initiateFinaleMap(mapThreadContext *context)
{
    for ( const auto &myPair : context->map ) {
        context->uniqe->push_back(myPair.first);
        (*context->mapFinal)[myPair.first]=myPair.second;
    }
}
void sortPhase(mapThreadContext *context)
{
    while (*context->atomicCounterForFinishReadFromINputVector < (context->totalThreadNum - 1))
    {
        // shuffle thread itreats all the threads vectors and empty them into one map (sort them)
        emptyAllThreadsVectors(context);
    }

    //shuffle thread will do it one more time to make sure all vectors empty
    //because now all the map threads are done
    emptyAllThreadsVectors(context);

    //fill the shared map thats all the thread could access in the reduce stage
    initiateFinaleMap(context);

    //finish shuffle
    *context->shuffeling = true;

}

void* threadFunction(void* arg) {
    auto *context = (mapThreadContext *) arg;
    if (context->threadId < context->totalThreadNum - 1)
    {
        mapPhase(context);
    }
    else
    {
        sortPhase(context);
    }

    //counter which tells shuffle when all threads done mapping
    unsigned finishMap = ( *(context->atomicCounterForFinishReadFromINputVector))++;

    //change to shuffle stage
    if (finishMap == context->totalThreadNum - 2)
    {
        *context->mapping = true;
    }
    //barrier that waits will shuffle thread finish
    context->barrier->barrier();

    //all threads do reduce
    reducePhase(context);

    unsigned int finishedAll = (*(context->atomicCounter3))++;
    if (finishedAll == context->totalThreadNum - 1)
    {
        *context->finished = true;
    }
    return nullptr;
}

void initialize(struct Bundle* b, int multiThreadLevel)
{
    b->mapFinal = new IntermediateMap();
    b->threadNumber = multiThreadLevel;
    b->threads= new pthread_t[multiThreadLevel];
    b->contexts = new mapThreadContext[multiThreadLevel];
    b->atomicCounter= new std::atomic<unsigned int>(0);
    b->atomicCounter2 = new std::atomic<unsigned int>(0);
    b->atomicCounter3 = new std::atomic<unsigned int>(0);
    b->barrier = new Barrier(multiThreadLevel);
    b->mutexArray = new pthread_mutex_t[multiThreadLevel];
    b->atomicCounterForFinishReadFromINputVector = new std::atomic<unsigned int>(0);
    b->read = new pthread_mutex_t;
    b->write = new pthread_mutex_t;
    b->uniqe = new  std::vector<K2*>();
    b->intermediatePairsProduced = new std::atomic<unsigned int>(0);
    b->intermediatePairsInMap = new std::atomic<unsigned int>(0);
    b->mapping = new bool();
    b->shuffeling = new bool();
    b->finished = new bool();
    *b->mapping = false;
    *b->shuffeling = false;
    *b->finished = false;

    //initiate mutex locks
    for (int i=0; i < multiThreadLevel; i++)
    {
        pthread_mutex_init(&b->mutexArray[i], nullptr);
    }
    pthread_mutex_init(b->write, nullptr);
    pthread_mutex_init(b->read, nullptr);
}

void makeThread(struct Bundle* b, int i, const MapReduceClient& client,
               const InputVec& inputVec, OutputVec& outputVec,
               int multiThreadLevel){
    b->mapVectors[i] = new std::vector<IntermediatePair>();
    b->contexts[i].atomicCounter3 = b->atomicCounter3;
    b->contexts[i].mapping =b->mapping;
    b->contexts[i].shuffeling = b->shuffeling;
    b->contexts[i].finished = b->finished;
    b->contexts[i].mapFinal = b->mapFinal;
    b->contexts[i].uniqe = b->uniqe;
    b->contexts[i].atomicCounter =b->atomicCounter;
    b->contexts[i].atomicCounter2 = b->atomicCounter2;
    b->contexts[i].atomicCounterForFinishReadFromINputVector = b->atomicCounterForFinishReadFromINputVector;
    b->contexts[i].intermediatePairsProduced = b->intermediatePairsProduced;
    b->contexts[i].intermediatePairsInMap = b->intermediatePairsInMap;
    b->contexts[i].client = &client;
    b->contexts[i].inputVector = &inputVec;
    b->contexts[i].outputVector = &outputVec;
    b->contexts[i].totalThreadNum =  multiThreadLevel;
    b->contexts[i].threadId = i;
    b->contexts[i].threadVector = b->mapVectors[i];
    b->contexts[i].barrier =b->barrier;
    b->contexts[i].mutexArray = b->mutexArray;
    b->contexts[i].read = b->read;
    b->contexts[i].write = b->write;
}

void createThreads(struct Bundle* b, const MapReduceClient& client,
                   const InputVec& inputVec, OutputVec& outputVec,
                   int multiThreadLevel)
{
    //creating a context for each created thread.
    for (int i = 0; i < multiThreadLevel - 1; ++i) {
        makeThread(b,i,client,inputVec,outputVec, multiThreadLevel);
    }
    //creates the special thread shuffle context
    makeThread(b, multiThreadLevel -1, client,inputVec,outputVec, multiThreadLevel);
    b->contexts[multiThreadLevel -1].mapVectors =  b->mapVectors;

    //creating multiThreadLevel threads.
    for (int j = 0; j < multiThreadLevel; ++j) {
        if (pthread_create(b->threads + j, nullptr, threadFunction, b->contexts + j) != SUCCESS)
        {
            std::cerr << SYSTEM_ERROR_UTHREAD_FAILED << std::endl;
            exit(EXIT_FAILURE);
        }
    }

}

JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec, OutputVec& outputVec,
                            int multiThreadLevel)
{
    //creates bundle
    auto b = new struct Bundle();

    //initialize bundle b
    initialize(b, multiThreadLevel);

    //creates contexts and threads
    createThreads(b, client, inputVec, outputVec , multiThreadLevel);

    return b;
}


void emit2 (K2* key, V2* value, void* arg){
    mapThreadContext* context = (mapThreadContext*) arg;
    IntermediatePair pair(key, value);

    lockMutexWrapper(&(context->mutexArray[context->threadId]));

    (*(context->intermediatePairsProduced))++;
    context->threadVector->push_back(pair);

    unlockMutexWrapper(&(context->mutexArray[context->threadId]));
}

void waitForJob(JobHandle job) {
    auto b = (struct Bundle*) job;
    if (!b->wasInWaitFunc)
    {
        for (int k = 0; k < b->threadNumber; ++k) {
            if (pthread_join(b->threads[k], nullptr) != SUCCESS){
                std::cerr <<SYSTEM_ERROR_UTHREAD_FAILED << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        b->wasInWaitFunc = true;
    }
}

void emit3(K3 *key, V3 *value, void *arg) {
    mapThreadContext* context = (mapThreadContext*) arg;
    OutputPair pair(key, value);
    lockMutexWrapper(context->write);

    context->outputVector->push_back(pair);

    unlockMutexWrapper(context->write);
}

void emptyContext(struct Bundle* b)
{
    int multiThreadLevel = b->threadNumber;
    for (int i=0; i < multiThreadLevel; i++)
    {
        b->contexts[i].mapping = nullptr;
        b->contexts[i].shuffeling = nullptr;
        b->contexts[i].finished = nullptr;
        b->contexts[i].mapFinal = nullptr;
        b->contexts[i].uniqe = nullptr;
        b->contexts[i].atomicCounter = nullptr;
        b->contexts[i].atomicCounter2 = nullptr;
        b->contexts[i].atomicCounterForFinishReadFromINputVector = nullptr;
        b->contexts[i].intermediatePairsProduced = nullptr;
        b->contexts[i].intermediatePairsInMap = nullptr;
        b->contexts[i].client = nullptr;
        b->contexts[i].inputVector = nullptr;
        b->contexts[i].outputVector = nullptr;
        b->contexts[i].threadVector = nullptr;
        b->contexts[i].barrier = nullptr;
        b->contexts[i].mutexArray = nullptr;
        b->contexts[i].read = nullptr;
        b->contexts[i].write = nullptr;
    }

}
void destoryMutexWrapper(pthread_mutex_t* mutex)
{
    if (pthread_mutex_destroy(mutex) != 0) {
        fprintf(stderr, "[[Barrier]] error on pthread_mutex_destroy");
        exit(1);
    }
}

void destroyAllMutexes(struct Bundle* b)
{
    int multiThreadLevel = b->threadNumber;
    destoryMutexWrapper(b->read);
    destoryMutexWrapper(b->write);
    for (int i =0 ; i < multiThreadLevel; i++)
    {
        destoryMutexWrapper(b->mutexArray+i);
    }

}
void closeJobHandle(JobHandle job) {
    waitForJob(job);
    auto b = (struct Bundle*) job;
    int multiThreadLevel = b->threadNumber;

    //not accendentliy free same object twice
    emptyContext(b);


    for (int i = 0; i < multiThreadLevel; ++i) {
        delete b->mapVectors[i];
    }
    
    delete b->mapFinal;

    delete b->atomicCounter3;
    delete b->atomicCounter;
    delete b->atomicCounter2;
    delete b->intermediatePairsProduced;
    delete b->intermediatePairsInMap;
    delete b->atomicCounterForFinishReadFromINputVector;

    delete b->barrier;

    // destroy all mutexes before release
    destroyAllMutexes(b);

    delete b->read;
    delete b->write;

    delete b->uniqe;

    delete b->mapping;
    delete b->shuffeling;
    delete b->finished;

    delete[] b->threads;
    delete[] b->mutexArray;
    delete[] b->contexts;

    delete b;
}

void getJobState(JobHandle job, JobState *state) {
    auto b =  (struct Bundle*) job;

    if (*b->finished)
    {
        state->stage = REDUCE_STAGE;
        state->percentage = 100;
    }
    //in the reduce stage
    else if (*b->shuffeling)
    {
        unsigned int read = *(b->atomicCounter2);
        unsigned int total =  b->uniqe->size();
        state->stage = REDUCE_STAGE;
        state->percentage = (read < total) ? (float)read / ((float)total) * 100: 100;
    }
    //in the shuffle stage
    else if (*b->mapping)
    {
        unsigned int to    makeThread(b, multiThreadLevel -1, client,inputVec,outputVec, multiThreadLevel);
tal = *(b->intermediatePairsProduced);
        unsigned int read =  *(b->intermediatePairsInMap);
        state->stage = SHUFFLE_STAGE;
        state->percentage = (read < total) ? (float)read / ((float)total) * 100: 100;
    }
    //in the mapping stage
    else
    {
        unsigned int read = *(b->atomicCounter);
        unsigned int total =  b->contexts[0].inputVector->size();
        state->stage = MAP_STAGE;
        state->percentage = (read < total) ? ((float)read / ((float)total)) * 100: 100;
    }
}