#ifndef UINT_H
#define UINT_H

#include <cstdint>
#include <string>
#include <atomic>

#include "ArithmeticTree.h"

// Unsiged int arithmetic class.
// T is a class with GF2-like behavior, N is the bit-width.
template <typename T, unsigned int N=8>
class UInt {
public:
  UInt(ArithmeticTree<T> &tree_, uint64_t value, const std::string &label_ = "")
   : tree(tree_) {
    if (label_.empty())
      this->label = "I" + std::to_string(this->n_nodes++);
    else
      this->label = label_;

    for (auto i = 0; i < N; i++) {
      this->bits[i] = &this->tree.new_node(value % 2, this->label + "b" +
                                          std::to_string(i));
      value /= 2;
    }
  }

  ArithmeticNode<T>** get_bits() {
    return this->bits;
  }

private:
  ArithmeticTree<T> &tree;

  // Used for naming.
  std::string label;
  static std::atomic<unsigned int> n_nodes;

  ArithmeticNode<T>* bits[N];
};

template <typename T, unsigned int N>
std::atomic<unsigned int> UInt<T, N>::n_nodes(0);

// Only works if T is the testing stub type GFN.
template <typename T, unsigned int N=8>
uint64_t get_value(UInt<T, N> nr) {
  uint64_t ret = 0;
  auto bits = nr.get_bits();
  for (int i = N - 1; i >= 0; i--) {
    ret = ret * 2 + bits[i]->get_data().get().get();
  }
  return ret;
}

#endif  // UINT_H
