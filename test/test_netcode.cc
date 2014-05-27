#include <iostream>
#include <cassert>
#include <cstdint>
#include <sstream>

#include "ArithmeticTree.h"
#include "Evaluator.h"
#include "Scheduler.h"
#include "NetWorker.h"
#include "Log.h"

using namespace std;

ArithmeticTree<int>::EvaluatorPtr_t eval(new Evaluator<int>());
auto sched = eval->get_scheduler();

// Message serialization.
void test1() {
  NetWorkerMsg<int> msg, result;
  msg.op = NetWorkerMsg<int>::PROD;
  msg.left = 10;
  msg.right = 20;

  std::string b = serialize((int) 1000000);
  assert(deserialize<int>(b) == 1000000);

  std::stringstream ss;
  ss << msg;
  ss >> result;

  assert(msg.op == result.op);
  assert(msg.left == result.left);
  assert(msg.right == result.right);
}

// Simple arithmetic.
void test2() {

  eval->reset();
  auto t = ArithmeticTree<int>(eval);
  auto x = t.new_node(2, "2") * t.new_node(2, "2") + t.new_node(5, "5") * t.new_node(5, "5");
  auto y = x + t.new_node(1, "1");
  t.eval(y);
  t.eval(x);
  t.get_evaluator()->exec();

  assert(*x.get_data() == 29);
  assert(*y.get_data() == 30);
}

int main(int argc, char **argv) {
  Log::set_level(Log::DISABLE);

  test1();
  fork_remote_worker<int>("localhost:9001");
  fork_remote_worker<int>("localhost:9001");
  fork_remote_worker<int>("localhost:9001");
  fork_remote_worker<int>("localhost:9001");

  auto *listener = new NetWorkerListener<int>(*sched, 9001);
  usleep(600000);
  // kill(pid, 9);
  usleep(100000);
  test1();
  test2();
  delete listener;

  return 0;
}