#ifndef ARITHMETICTREE_H
#define ARITHMETICTREE_H

#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <string>

#include "ArithmeticNode.h"
#include "Evaluator.h"

// Forward declarations.
template <typename T>
class ArithmeticNode;
template <typename T>
class Evaluator;


// Container for a set of related ArithmeticNode objects. Owns(ie. creates and
// frees) all nodes assigned to it.
template <typename T>
class ArithmeticTree {

friend class ArithmeticNode<T>;

public:
  typedef ArithmeticNode<T> Node_t;
  typedef std::shared_ptr<Evaluator<T> > EvaluatorPtr_t;

  ArithmeticTree(EvaluatorPtr_t evaluator_ = EvaluatorPtr_t(new Evaluator<T>()))
   : evaluator(evaluator_) {}

  Node_t& new_node(const T& value, const std::string &label = "") {
    auto &node = this->new_node(label);
    *node.data = T(value);
    return node;
  }

  EvaluatorPtr_t get_evaluator() {
    return this->evaluator;
  }

  void eval_all() {
    for (auto node : this->nodes)
      this->evaluator->add(*node);
  }

  void eval(Node_t &node) {
    if (&node.tree != this)
      throw std::runtime_error("Node does not belong to the specified tree.");

    this->evaluator->add(node);
  }

  virtual ~ArithmeticTree() {
    for (auto node : this->nodes)
      delete node;
  }

private:
  // Create an empty node.
  ArithmeticNode<T>& new_node(const std::string &label = "") {
    auto *node = new ArithmeticNode<T>(*this, label);
    this->nodes.insert(node);
    return *node;
  }

  // Add an already created node to the tree. Checks for and returns an already
  // existing equivalent node.
  ArithmeticNode<T>& new_node(ArithmeticNode<T> *node) {
    for (auto &n : this->nodes)
      if (n == node)
        return *n;
    this->nodes.insert(node);
    return *node;
  }

  // Set of all nodes managed by this tree.
  std::set<ArithmeticNode<T>* > nodes;

  EvaluatorPtr_t evaluator;
};

#endif //ARITHMETICTREE_H
