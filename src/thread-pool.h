/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector

#include "Semaphore.h"
#include <queue>

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool;  // Forward declaration

// class Worker {
// public:
//     Worker(size_t ID) : ID(ID), th([this, &ID] { work(&ID); }) {}

//     Worker(const Worker&) = delete;
//     Worker& operator=(const Worker&) = delete;

//     Worker(Worker&& other) noexcept : ID(other.ID), th(std::move(other.th)) {
//         // Move constructor: transfer ownership of resources
//     }

//     Worker& operator=(Worker&& other) noexcept {
//         if (this != &other) {
//             ID = other.ID;
//             th = std::move(other.th);
//         }
//         return *this;
//     }

//     ~Worker() {
//         if (th.joinable()) {
//             th.join();
//         }
//     }
//     bool working = false;
//     bool kill = false;
//     std::mutex workLock;
//     Semaphore workSem;
//     std::function<void(void)> thunk;

//     thread getThread() {
//         return std::move(th);
//     }

// private:
//     size_t ID;
//     std::thread th;
//     void work(size_t * ID);  // Only declare here, define in .cc file
// };





class ThreadPool {
 public:
/**
 * Constructs a ThreadPool configured to spawn up to the specified
 * number of threads.
 */
  ThreadPool(size_t numThreads);

/**
 * Schedules the provided thunk (which is something that can
 * be invoked as a zero-argument function without a return value)
 * to be executed by one of the ThreadPool's threads as soon as
 * all previously scheduled thunks have been handled.
 */
  void schedule(const std::function<void(void)>& thunk);

/**
 * Blocks and waits until all previously scheduled thunks
 * have been executed in full.
 */
  void wait();

/**
 * Waits for all previously scheduled thunks to execute, and then
 * properly brings down the ThreadPool and any resources tapped
 * over the course of its lifetime.
 */
  ~ThreadPool();
  
 private:
  struct Worker{
        std::function<void(void)> thunk;
        bool kill = false;
        bool working = false;
        Semaphore workSem;
        std::mutex workLock;
        size_t ID;
        std::thread th;
  };

  std::thread dt;                // dispatcher thread handle
  std::vector<Worker> wts;  // worker thread handles
  Semaphore dtSem;
  Semaphore wSem;
  std::queue<std::function<void(void)>> thunks;
  mutex thunkLock;
  mutex semLock;
  bool kill = false;
  std::condition_variable_any dtCV;
  std::condition_variable_any wCV;

  void dispatcher();
  void work(Worker * worker);

/**
 * ThreadPools are the type of thing that shouldn't be cloneable, since it's
 * not clear what it means to clone a ThreadPool (should copies of all outstanding
 * functions to be executed be copied?).
 *
 * In order to prevent cloning, we remove the copy constructor and the
 * assignment operator.  By doing so, the compiler will ensure we never clone
 * a ThreadPool.
 */
  ThreadPool(const ThreadPool& original) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif