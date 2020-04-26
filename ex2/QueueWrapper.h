//
// Created by shai on 26/04/2020.
//

#ifndef EX2_QUEUEWRAPPER_H
#define EX2_QUEUEWRAPPER_H


#include <queue>

class QueueWrapper {
private:
    std::queue<int> readyQueue;
public:
    void push(int tid)
    {
        readyQueue.push(tid);
    }
    void pop()
    {
        readyQueue.pop();
    }
    int front()
    {
        return readyQueue.front();
    }

    bool empty()
    {
        return readyQueue.empty();
    }
};


#endif //EX2_QUEUEWRAPPER_H
