#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

void ThreadPool::dispatcher() {
    while (true) {
        sem.wait();
        thunkLock.lock();
        if (kill) {
            thunkLock.unlock();
            return;
        }
        if (done && thunks.empty()) {
            thunkLock.unlock();
            break;
        }
        if (thunks.empty()) {
            thunkLock.unlock();
            continue;
        }
        auto thunk = thunks.front();
        thunks.pop();
        thunkLock.unlock();

        for (auto& worker : wts) {
            lock_guard<mutex> lk(worker.workLock);
            if (!worker.working) {
                worker.thnk.lock();
                worker.thunk = thunk;
                worker.workSem.signal();
                worker.thnk.unlock();
                break;
            }
        }
    }
}

void Worker::work(size_t *ID) {
    while (true) {
        workSem.wait();
        workLock.lock();
        working = true;
        thnk.lock();
        if (kill) {
            thnk.unlock();
            workLock.unlock();
            return;
        }
        if (!thunk) {
            working = false;
            thnk.unlock();
            workLock.unlock();
            continue;
        }
        workLock.unlock();
        thunk();
        workLock.lock();
        working = false;
        thnk.unlock();
        workLock.unlock();
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts() {
    wts.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        wts.emplace_back(i);
    }
    dt = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    lock_guard<mutex> lk(thunkLock);
    thunks.push(thunk);
    sem.signal();
}

void ThreadPool::wait() {
    int counter = 0;
    while(true){
        if (thunks.size() == 0){
            for (auto& worker: wts) {
                worker.workLock.lock();
                if (worker.working == false){
                    counter++;
                }
                worker.workLock.unlock();
            }
            if (counter == wts.size()){
                break;
            }
            counter = 0;
        }
    }
}

ThreadPool::~ThreadPool() {
    {
        lock_guard<mutex> lk(thunkLock);
        kill = true;
        sem.signal();
    }
    dt.join();

    for (auto& worker : wts) {
        {
            lock_guard<mutex> wl(worker.workLock);
            worker.thnk.lock();
            worker.kill = true;
            worker.workSem.signal();
            worker.thnk.unlock();
        }
        worker.getThread().join();
    }
}
