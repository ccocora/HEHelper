#ifndef GFN_H
#define GFN_H

#include <cstdint>

// Class that models modular arithmetic %N.
// Useful as a test stub.
template <unsigned int N>
class GFN {
public:
  GFN(uint64_t value_ = 0) {
    this->set(value_);
  }

  GFN<N> operator+(const GFN<N> &rhs) const {
    return GFN(this->value + rhs.value);
  }

  GFN<N> operator*(const GFN<N> &rhs) const {
    return GFN(this->value * rhs.value);
  }

  bool operator==(const GFN<N> &rhs) const {
    return this->value == rhs.value;
  }

  uint64_t get() const {
    return this->value;
  }

  void set(uint64_t value_) {
    this->value = value_ % N;
  }

  // These only make sense for N=2, should probably move to template specialization.
  GFN<N> operator&&(const GFN<N> &rhs) const {
    return this->operator*(rhs);
  }

  GFN<N> operator!() const {
    return (*this) + GFN<N>(1);
  }

  GFN<N> operator||(const GFN<N> &rhs) const {
    return ! (! (*this) && !rhs);
  }

  GFN<N> operator^(const GFN<N> &rhs) const {
    return (*this) + rhs;
  }

private:
  uint64_t value;
};

#endif  // GFN_H