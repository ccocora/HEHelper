// This class is responsible for deciding the order in which a tree's nodes are
// evaluated. Closely integrated with the Scheduler class.

#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>

#include "ArithmeticNode.h"
#include "Scheduler.h"
#include "Worker.h"
#include "Log.h"

// Forward declarations.
template <typename T>
class ArithmeticNode;

template <typename T>
class Evaluator {

friend class Worker<T>;  // For access to the mutex.

public:
  typedef ArithmeticNode<T> Node_t;
  typedef std::shared_ptr<Scheduler<T> > SchedPtr_t;

  Evaluator(SchedPtr_t sched_ = SchedPtr_t(new Scheduler<T>()))
    : sched(sched_), log(Log("Evaluator")) {}

  // Schedule work, delaying actual execution until exec() is called.
  void add(Node_t &node) {
    log.info("node " + node.label + " required");
    if (node.state != Node_t::RESOLVED) {
      this->outputs.insert(&node);
    }
  }

  SchedPtr_t get_scheduler() {
    return this->sched;
  }

  // Actually perform the work scheduled by add().
  // Will block until all nodes have been evaluated even if the scheduler has no
  // workers assigned.
  void exec() {
    this->prepare();
    this->schedule();
  }

  void reset() {
    this->outputs.clear();
    this->nodes.clear();
  }

  virtual ~Evaluator() {};

private:
  typedef std::set<Node_t *> NodeSet_t;
  NodeSet_t outputs;  // Requested nodes.

  enum State {PENDING, IN_PROG, DONE};
  typedef std::unordered_map<Node_t *, State> NodeState_t;
  NodeState_t nodes;  // All the nodes to be evaluated.

  SchedPtr_t sched;

  std::mutex mutex;  // Object-global lock.
  std::condition_variable notify_progress;  // Wait for work to be done.

  Log log;

  // Recursively adds required nodes to be evaluated. Subclasses can implement
  // optimizations by overriding this.
  virtual void prepare() {
    for (auto node : this->outputs)
      this->recurse_node(*node);
  }

  void recurse_node(Node_t &node) {
    if (node.state != Node_t::RESOLVED && this->nodes.find(&node) == this->nodes.end()) {
      this->nodes.insert({&node, PENDING});
      this->recurse_node(*node.left);
      this->recurse_node(*node.right);
    }
  }

  // Go through the nodes vector, evaluating what is possible then waiting for
  // more results.
  void schedule() {
    typedef std::unique_lock<std::mutex> Lock_t;
    Lock_t lck(this->mutex);

    auto all_done = [this] () { for (const auto &n : this->nodes)
                                  if (n.second != DONE)
                                    return false;
                                return true; };

    while (! all_done()) {
      log.dbg("Checking for work.");
      for (auto &node : this->nodes)
        if (node.second == PENDING && check_solvable(node.first)) {
          node.second = IN_PROG;

          this->sched->add_task(*node.first,
                                [] () {},
                                [&] () { Lock_t l(this->mutex);
                                         node.second = DONE;
                                         this->notify_progress.notify_all(); },
                                [&] () { Lock_t l(this->mutex);
                                         node.second = PENDING;
                                         this->notify_progress.notify_one();} );
        }

      if (! all_done()){
        log.dbg("Waiting for tasks to complete");
        this->notify_progress.wait(lck);
      }
    }
    log.dbg("All done");
    lck.unlock();
  }

  bool check_solvable(Node_t *node) {
    if(node->state == Node_t::RESOLVED)
      return false;
    if(node->left->state == Node_t::RESOLVED &&
       node->right->state == Node_t::RESOLVED &&
       node->left->data && node->right->data)
      return true;
    return false;
  }

};

#endif //EVALUATOR_H
