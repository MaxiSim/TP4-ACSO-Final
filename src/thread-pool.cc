#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;


void ThreadPool::dispatcher() {
    while (1) {
        dtSem.wait();
        wSem.wait();
        thunkLock.lock();
        if (kill) {
            thunkLock.unlock();
            return;
        }
        auto thunk = thunks.front();
        thunks.pop();
        thunkLock.unlock();
        for (auto& worker : wts) {
            lock_guard<mutex> lk(worker.workLock);
            if (!worker.working) {
                worker.thunk = thunk;
                worker.working = true;
                worker.workSem.signal();
                break;
            }
        }
    }
}


void ThreadPool::work(Worker * worker) {
    while (true) {
        worker->workSem.wait();
        worker->workLock.lock();
        if (worker->kill) {
            worker->working = false;
            worker->workLock.unlock();
            return;
        }
        worker->workLock.unlock();
        worker->thunk();
        worker->workLock.lock();
        workercount--;
        if (workercount == 0){
            wCV.notify_all();
        }
        worker->working = false;
        wSem.signal();
        worker->workLock.unlock();
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), wSem(numThreads) {
    for (auto& worker : wts) {
        worker.th = thread([this, &worker] { work(&worker); });
    }
    dt = thread([this](){dispatcher();});
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    lock_guard<mutex> lk(thunkLock);
    thunks.push(thunk);
    done = false;
    workercount++;
    dtSem.signal();
}

void ThreadPool::wait() {
    unique_lock<mutex> lk (thunkLock);
    while (workercount > 0){
        wCV.wait(thunkLock);
    }
    return;
}

ThreadPool::~ThreadPool() {
    wait();
    {
        lock_guard<mutex> lk(thunkLock);
        kill = true;
        dtSem.signal();
    }
    dt.join();

    for (auto& worker : wts) {
        {
            lock_guard<mutex> wl(worker.workLock);
            worker.kill = true;
            worker.workSem.signal();
        }
        worker.th.join();
    }
}
