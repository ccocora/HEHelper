// Computational primitive (e.g. 1 process/cpu, local or remote, etc).
// Different worker types should subclass this.
// Throwing exceptions from do_sum or do_prod is supported, but changes to the
// provided ArithmeticNode& must be atomic.

#ifndef WORKER_H
#define WORKER_H

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <string>
#include <set>
#include <memory>

#include "ArithmeticNode.h"
#include "Scheduler.h"
#include "Log.h"

// Forward declarations.
template <typename T>
class Scheduler;
template <typename T>
class ArithmeticNode;


template <typename T>
class Worker {

friend class Scheduler<T>;

public:
  // Will feed from the task queue of the specified scheduler.
  Worker(Scheduler<T> &scheduler, const std::string &name_ = "Worker")
    : sched(scheduler), log(Log(name_, scheduler.log)), name(name_) {
    this->sched.register_worker(this);
    this->thrd = std::thread([&] (Worker *exec) {exec->run(); }, this);
  }

  virtual ~Worker() {
    log.dbg("Terminating...");
    {
        std::lock_guard<std::mutex> lck(this->sched.mutex);
        this->end = true;
        this->notify_work.notify_one();
    }
    this->thrd.join();
    log.dbg("Terminated");
  }

protected:
  Scheduler<T> &sched;  // Scheduler responsible for this Worker.
  Log log;

private:
  std::thread thrd;  // Thread this->loop runs on.
  // CV used to wake up this Worker when work is available.
  std::condition_variable notify_work;
  bool end = false;  // End looping so thread can be joined.

  std::string name;

   void run() {
    this->loop();
  }

  void loop() {
    while (true) {
      std::unique_lock<std::mutex> lck(this->sched.mutex);

      if (this->sched.tasks.empty()) {
        log.dbg("Waiting for work");
        this->notify_work.wait(lck,
                               [this] () { return ! this->sched.tasks.empty()
                                           || this->end; });
      }
      if (this->end)
        break;

      auto tsk = this->sched.tasks.front();
      this->sched.tasks.pop();

      lck.unlock();

      try {
        log.dbg("Starting task " + tsk.node.get_label());
        tsk.pre_exec();
        this->solve_node(tsk.node);
        tsk.post_exec();
        log.dbg("Finished task " + tsk.node.get_label());

      } catch (std::exception &e) {
        log.err("Failure on task " + tsk.node.get_label() + ": " + e.what());
        tsk.on_fail();
        this->sched.unregister_worker(this);
        delete this;
        break;

      } catch(...) {
        log.err("Unrecognized exception on task " + tsk.node.get_label());
        tsk.on_fail();
        this->sched.unregister_worker(this);
        delete this;
        break;
      }
    }
  }

  // Task<T> get_task() {
  //   std::unique_lock<std::mutex> lck(this->sched.mutex);

  //   if (this->sched.tasks.empty()) {
  //     log.dbg("Waiting for work");
  //     this->notify_work.wait(lck,
  //                            [this] () { return ! this->sched.tasks.empty();
  //                                        && ! this->end; });
  //   }

  //   auto tsk = this->sched.tasks.front();
  //   this->sched.tasks.pop();
  //   return tsk;
  // }

  // Actually calculates the value of the node.
  void solve_node(ArithmeticNode<T> &node) {
    T result;
    switch(node.state) {
      case ArithmeticNode<T>::RESOLVED: break;
      case ArithmeticNode<T>::SUM:
        result = do_sum(node.left->data->get(), node.right->data->get());
        break;
      case ArithmeticNode<T>::PROD:
        result = do_prod(node.left->data->get(), node.right->data->get());
        break;
    }

    // It's ok to noy synchronize the above reads because we're the only writer.
    std::unique_lock<std::mutex> lck(node.tree.get_evaluator()->mutex);
    *node.data = result;
    node.state = ArithmeticNode<T>::RESOLVED;
  }

  // Ideally subclasses only need to override these.
  virtual T do_sum(const T &left, const T &right) = 0;

  virtual T do_prod(const T &left, const T &right) = 0;
};

// Simplest possible implementation, computes the operations inside the local thread.
// Mainly for debugging and testing.
template <typename T>
class WorkerStub : public Worker<T> {
public:
  WorkerStub(Scheduler<T> &scheduler, const std::string &name_ = "Worker")
    : Worker<T>(scheduler, name_) {}

  // Create n WorkerStubs and assign them to the given scheduler.
  static std::set<WorkerStub<T>* >
  create_n(Scheduler<T> &scheduler, unsigned int n) {
    auto ret = std::set<WorkerStub<T>* >();
    for (unsigned int i = 1; i <= n; i++)
      ret.insert(new WorkerStub(scheduler, "WorkerStub_" + std::to_string(i)));
    return ret;
  }

private:
  virtual T do_sum(const T &left, const T &right) {
    return T(left + right);
  }

  virtual T do_prod(const T &left, const T &right) {
    return T(left * right);
  }
};
#endif //WORKER_H
