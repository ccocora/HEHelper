// A naive scheduler. Assumes CPU-bound workloads so it only attempts to keep all
// workers busy. Producer-consumer model, (1) Evaluator -> (1) Scheduler
// -> (n) Workers. Owns all workers assigned to it.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <mutex>
#include <condition_variable>
#include <memory>
#include <set>
#include <queue>
#include <exception>

#include "Worker.h"

template <typename T>
struct Task {
    ArithmeticNode<T> &node;
    std::function<void ()> pre_exec;
    std::function<void ()> post_exec;
    std::function<void ()> on_fail;
};

template <typename T>
class Scheduler {

friend class Worker<T>;

public:
  Scheduler() : log(Log("Scheduler")) {}

  void add_task(ArithmeticNode<T> &node, std::function<void ()> pre_exec,
                std::function<void ()> post_exec, std::function<void ()> on_fail) {
    std::lock_guard<std::mutex> lock(this->mutex);

    this->tasks.push({node, pre_exec, post_exec, on_fail});
    log.dbg("Added task " + node.get_label());
    if (this->tasks.size() == 1)
      for (auto exec : this->workers)
        exec->notify_work.notify_one();
  }

  std::set<Worker<T>* > get_workers() {
    return this->workers;
  }

  virtual ~Scheduler() {
    log.info("Cleaning up workers...");

    for (auto exec : this->workers)
      delete exec;

    log.info("Done cleaning");
  }

  // Note: will own the worker after registering.
  void register_worker(Worker<T> *worker) {
    std::lock_guard<std::mutex> lck(this->mutex);
    this->workers.insert(worker);
    log.info("Registered worker " + worker->name);
  }

  void unregister_worker(Worker<T> *worker) {
    std::lock_guard<std::mutex> lck(this->mutex);
    this->workers.erase(worker);
    log.info("Unregistered worker " + worker->name);
  }

private:

  std::set<Worker<T>* > workers;

  // Object-global lock.
  std::mutex mutex;

  // The actual task queue.
  std::queue<Task<T> > tasks;

  Log log;
};

#endif  // SCHEDULER_H
