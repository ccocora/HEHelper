#include <iostream>
#include <cassert>
#include <cstdint>

#include "ArithmeticTree.h"
#include "Evaluator.h"
#include "Scheduler.h"
#include "Worker.h"
#include "Log.h"
#include "UInt.h"
#include "GFN.h"

using namespace std;

ArithmeticTree<int>::EvaluatorPtr_t eval(new Evaluator<int>());

// Assignment.
void test1() {
  auto tree = ArithmeticTree<int>(eval);
  auto node = tree.new_node(2);
  auto d = node.get_data();

  assert(*d == 2);
}

// Simple addition.
void test2() {
  eval->reset();
  auto tree = ArithmeticTree<int>(eval);
  auto node1 = tree.new_node(1);
  auto node2 = tree.new_node(2);
  auto& node3 = node1 + node2;
  tree.eval(node3);
  tree.get_evaluator()->exec();

  assert(*node3.get_data() == 3);
}

// Test exceptions.
void test3() {
  eval->reset();
  auto tree1 = ArithmeticTree<int>();
  auto tree2 = ArithmeticTree<int>();
  auto n1 = tree1.new_node(1);
  auto n2 = tree2.new_node(1);

  bool caught = false;
  try {
    auto n3 = n1 + n2;
    assert(false);
  } catch(...) {
    caught = true;
  }
  assert(caught);

  caught = false;
  try {
    tree1.eval(n2);
    assert(false);
  } catch(...) {
    caught = true;
  }
  assert(caught);
}

// More complex arithmetic expressions.
void test4() {
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

// Copy behavior.
void test5() {
  eval->reset();
  auto t = ArithmeticTree<int>(eval);
  auto n1 = t.new_node(5) * t.new_node(5);
  auto n2 = n1;
  auto n3(n1);
  auto n4 = n3 + t.new_node(2);
  t.eval_all();
  t.get_evaluator()->exec();

  assert(*n2.get_data() == 25);
  assert(*n3.get_data() == 25);
  assert(*n4.get_data() == 27);
}

// Comparison and shallow copy.
void test6() {
  eval->reset();
  auto t = ArithmeticTree<int>(eval);
  auto n1 = t.new_node(1);
  auto n2 = t.new_node(2);
  auto n3 = n1 + n2;

  assert(! (n1 == n2));
  assert(n1 + n2 == n3);
  assert(n1 + n2 == n1 + n2);
}

// Test GFN.
void test7() {
  GFN<2> zero(0);
  GFN<2> one(1);

  auto AND1 = one && one;
  auto AND2 = one && zero;
  auto XOR1 = zero ^ one;
  auto XOR2 = one ^ one;

  assert(zero.get() == 0);
  assert(zero + one == one);
  assert(one + one == zero);
  assert(AND1 == one);
  assert(AND2 == zero);
  assert(XOR1 == one);
  assert(XOR2 == zero);
  assert(!one == zero);
}

// Test CAS.
void test8() {
  GFN<2> zero(0);
  GFN<2> one(1);

  ArithmeticTree<GFN<2> >::EvaluatorPtr_t ev(new Evaluator<GFN<2> >());
  auto t = ArithmeticTree<GFN<2> >(ev);
  auto n1 = t.new_node(zero);
  auto n2 = t.new_node(one);
  auto n3 = t.new_node(zero);
  n3.CAS(n2, n1, n2);

  t.eval_all();
  assert(*n1.get_data() == zero);
}

// Test UInt.
void test9() {
  ArithmeticTree<GFN<2> >::EvaluatorPtr_t ev(new Evaluator<GFN<2> >());
  auto t = ArithmeticTree<GFN<2> >(ev);
  UInt<GFN<2> > n1(t, 10);
  UInt<GFN<2> > n2(t, 20);

  assert(get_value(n1) == 10);
  assert(get_value(n2) == 20);
}

int main(int argc, char **argv) {
  Log::set_level(Log::DISABLE);
  WorkerStub<int>::create_n(*eval->get_scheduler(), 5);  // Creates 5 threads.

  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();

  return 0;
}