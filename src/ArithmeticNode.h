#ifndef ARITHMETICNODE_H
#define ARITHMETICNODE_H

#include <stdexcept>
#include <iostream>
#include <memory>
#include <atomic>
#include <string>
#include <boost/optional.hpp>

#include "ArithmeticTree.h"
#include "Evaluator.h"
#include "Worker.h"

// Forward declaration.
template <typename T>
class ArithmeticTree;

// This is a generic container class for describing arithmetic trees. The
// template parameter should be a type that implements operators +, *, = and ==.
template <typename T>
class ArithmeticNode {

friend class ArithmeticTree<T>;
friend class Evaluator<T>;
friend class Worker<T>;

public:
  typedef boost::optional<T> Value_t;
  typedef std::shared_ptr<Value_t> ValuePtr_t;

  // Note: indirectly returns a pointer by using boost::optional.
  Value_t get_data() const {
    return *this->data;
  }

  // Mark this node to be evaluated.
  void eval() {
    this->tree->eval(*this);
  }

  ArithmeticNode<T>& operator+(ArithmeticNode<T>& rhs) {
    return this->get_operator_result(rhs, SUM);
  }

  ArithmeticNode<T>& operator*(ArithmeticNode<T>& rhs) {
    return this->get_operator_result(rhs, PROD);
  }

  // Compare and swap. T must be a GF2-like class for this to have meaning.
  ArithmeticNode<T>& CAS(ArithmeticNode<T>& condition, ArithmeticNode<T>& result_true,
                         ArithmeticNode<T>& result_false) {
    auto one = this->tree.new_node(T(1));
    return condition * result_true + (condition + one) * result_false;
  }

  bool operator==(const ArithmeticNode<T>& rhs) const {
    bool ret = &this->tree == &rhs.tree && this->state == rhs.state;
    // Explicit comparison because of boost::optional semantics.
    if (this->state == SUM || this->state == PROD)
      ret &= this->left == rhs.left && this->right == rhs.right;
    if (this->state == RESOLVED)
      ret &= this-> data == rhs.data;
    return ret;
  }

  std::string get_label() {
    return this->label;
  }

private:
  ArithmeticNode();
  ArithmeticNode(ArithmeticTree<T> &tree_, const std::string &label_ = "")
    : tree(tree_) {
    this->data.reset(new Value_t);
    if (label_.empty())
      this->label = "N" + std::to_string(this->n_nodes++);
    else
      this->label = label_;
  }

  std::string label;  // Useful for debugging, etc.

  // boost::optional wrapped in a shared_ptr to prevent copies.
  ValuePtr_t data;

  // Parent tree.
  ArithmeticTree<T> &tree;

  // Edges to parent nodes.
  typedef boost::optional<ArithmeticNode<T> &> Edge_t;
  Edge_t left, right;

  // Sum/prod for the operation that resolves the node.
  enum State {SUM, PROD, RESOLVED};
  State state = RESOLVED;

  // Used for naming.
  static std::atomic<unsigned int> n_nodes;

  ArithmeticNode<T>& get_operator_result(ArithmeticNode<T>& rhs, State state_) {
    if (&rhs.tree != &this->tree)
      throw std::runtime_error("Nodes belong to different arithmetic trees.");

    std::string label_;
    std::string op;
    if (state_ == SUM)
      op = "+";
    else if (state_ == PROD)
      op = "*";
    auto format = [] (const std::string &s) { return s.size() <= 1 ? s : "(" + s + ")"; };
    if (! this->label.empty() && ! rhs.label.empty())
      label_ = format(this->label) + " " + op + " " + format(rhs.label);

    auto *node = new ArithmeticNode<T>(this->tree, label_);
    node->left = *this;
    node->right = rhs;
    node->state = state_;
    // node->label = label_;

    return this->tree.new_node(node);  // Insert in tree and return the reference.
  }

};

template <typename T>
std::atomic<unsigned int> ArithmeticNode<T>::n_nodes(0);

#endif //ARITHMETICNODE_H
