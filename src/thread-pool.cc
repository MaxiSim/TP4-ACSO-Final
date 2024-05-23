#include "thread-pool.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

// void ThreadPool::dispatcher() {
//     while (1) {
//         dtSem.wait();
//         wSem.wait();
//         thunkLock.lock();
//         if (kill) {
//             thunkLock.unlock();
//             return;
//         }
//         auto thunk = thunks.front();
//         thunks.pop();
//         thunkLock.unlock();

//         for (auto& worker : wts) {
//             lock_guard<mutex> lk(worker.workLock);
//             if (!worker.working) {
//                 worker.thunk = thunk;
//                 worker.working = true;
//                 worker.workSem.signal();
//                 break;
//             }
//         }
//     }
// }

// void Worker::work(size_t *ID) {
//     while (true) {
//         workSem.wait();
//         workLock.lock();
//         if (kill) {
//             working = false;
//             workLock.unlock();
//             return;
//         }
//         workLock.unlock();
//         thunk();
//         workLock.lock();
//         working = false;
//         workLock.unlock();
//     }
// }

// ThreadPool::ThreadPool(size_t numThreads) : wts(), wSem(numThreads) {
//     wts.reserve(numThreads);
//     for (size_t i = 0; i < numThreads; ++i) {
//         wts.emplace_back(i);
//     }
//     dt = thread([this] { dispatcher(); });
// }

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
        worker->working = false;
        worker->workLock.unlock();
        wSem.signal();
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
    dtSem.signal();
}

// void ThreadPool::wait() {
//     int counter = 0;
//     while(true){
//         if (thunks.size() == 0){
//             for (auto& worker: wts) {
//                 worker.workLock.lock();
//                 if (worker.working == false){
//                     counter++;
//                 }
//                 worker.workLock.unlock();
//             }
//             if (counter == wts.size()){
//                 break;
//             }
//             counter = 0;
//         }
//     }
// }

void ThreadPool::wait() {
    thunkLock.lock();
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
