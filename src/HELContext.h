#ifndef HELCONTEXT_H
#define HELCONTEXT_H

#include <iostream>
#include <memory>

#include "EncryptedArray.h"
#include "FHE.h"

// Convenience class for moving around HELib data required for computation
// (FHEContext + associated key)
template <typename Key>
class HELContext {

public:
  HELContext();

  template <typename Key2>
  HELContext(const HELContext<Key2> &other);

  HELContext(std::shared_ptr<FHEcontext> context_, std::shared_ptr<Key> key_);

  // This constructor generates a FHEContext and secret key for you.
  // key_security: log2(nr_possible_keys) (parameter k in HELib)
  // depth: maximum depth for leveled FHE (parameter L in HELib)
  HELContext(long key_security, long depth, long p = 2, long r = 1,
             long w = 64, long c = 2, long d = 0, long s = 0);

  std::shared_ptr<Key> key;
  std::shared_ptr<FHEcontext> context;
  std::shared_ptr<EncryptedArray> ea;
  std::shared_ptr<PlaintextArray> pa;

  // Check if both the key and context ptrs are initialized, throws if not.
  void assert_init() const;

  bool operator==(const HELContext<Key> &rhs);

  template <typename T>
  friend istream& operator>>(istream &in, HELContext<T> &ctx);

  template <typename T>
  friend ostream& operator<<(ostream &out, const HELContext<T> &ctx);

protected:
  void init_arrays();
};

// Helper function to help with bugged copy constructor of FHEContext.
FHEcontext copy_helib_context(const FHEcontext& other);

// Stream operators.
template <typename Key>
istream& operator>> (istream &in, HELContext<Key> &ctx);

template <typename Key>
ostream& operator<< (ostream &out, const HELContext<Key> &ctx);

// Commodity.
typedef HELContext<FHEPubKey> PublicCtx;
typedef HELContext<FHESecKey> PrivateCtx;

#endif //HELCONTEXT_H
