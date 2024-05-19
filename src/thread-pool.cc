/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

void ThreadPool::dispatcher() {
    while (1){
        sem.wait();
        thunkLock.lock();
        if (done && (thunks.size() == 0)){
            cout << "No thunks to execute" << endl;
            thunkLock.unlock();
            return;
        }
        auto thunk = thunks.front();
        thunks.pop();
        for (size_t i = 0; i < wts.size(); i++){
            wts[i].workLock.lock();
            if (!wts[i].working){
                wts[i].thnk.lock();
                wts[i].thunk = thunk;
                wts[i].workSem.signal();
                wts[i].thnk.unlock();
                wts[i].workLock.unlock();
                break;
            }
            wts[i].workLock.unlock();
        }
        thunkLock.unlock();
    }
}

void Worker::work(size_t * ID){
    while (1){
        workSem.wait();
        workLock.lock();
        working = true;
        thnk.lock();
        if (thunk == nullptr){
            working = false;
            workLock.unlock();
            thnk.unlock();
            return;
        }
        workLock.unlock();
        thunk();
        workLock.lock();
        thnk.unlock();
        working = false;
        workLock.unlock();
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts() {
    wts.reserve(numThreads);
    for (size_t i = 0; i < numThreads; i++){
        wts.emplace_back(i);  // Properly use emplace_back
    }
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    thunkLock.lock();
    thunks.push(thunk);
    sem.signal();
    thunkLock.unlock();
}

void ThreadPool::wait() {}

ThreadPool::~ThreadPool() {}