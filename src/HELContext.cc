#include <iostream>
#include <exception>
#include <sstream>

#include "FHE.h"
#include "EncryptedArray.h"

#include "HELContext.h"

template <typename Key>
HELContext<Key>::HELContext() {}

template<typename K1> template<typename K2>
HELContext<K1>::HELContext(const HELContext<K2> &other)
  : key(other.key), context(other.context) {
  this->init_arrays();
}

// template <>
// HELContext<FHEPubKey> HELContext
template <typename Key>
HELContext<Key>::HELContext(std::shared_ptr<FHEcontext> context_,
                            std::shared_ptr<Key> key_)
    : key(key_), context(context_) {
  this->init_arrays();
}

template <typename Key>
HELContext<Key>::HELContext(long key_security, long depth, long p, long r,
                            long w, long c, long d, long s) {
    // Gen context.
    long m = FindM(key_security, depth ,c ,p , d, s, 0);
    this->context.reset(new FHEcontext(m, p, r));
    buildModChain(*this->context, depth, c);

    // Gen key.
    auto key_ptr = new FHESecKey(*this->context);
    key_ptr->GenSecKey(w);
    addSome1DMatrices(*key_ptr);
    this->key.reset(key_ptr);

    this->init_arrays();
}

template <typename Key>
void HELContext<Key>::assert_init() const {
  if (! this->key)
    throw std::runtime_error("Member key not initialized");
  if (! this->context)
    throw std::runtime_error("Member context not initialized");
}

template <typename Key>
void HELContext<Key>::init_arrays() {
  this->ea.reset(new EncryptedArray(*this->context));
  this->pa.reset(new PlaintextArray(*this->ea));
}

template <typename Key>
bool HELContext<Key>::operator==(const HELContext<Key> &rhs) {
    this->assert_init();
    rhs.assert_init();
    return *this->key == *rhs.key; // && *this->context == *rhs.context;
}

template <typename Key>
istream& operator>> (istream &in, HELContext<Key> &ctx) {
  unsigned long m, p, r;
  readContextBase(in, m, p, r);
  ctx.context.reset(new FHEcontext(m, p, r));
  // in >> *ctx.context;
  ctx.key.reset(new Key(*ctx.context));
  in >> *ctx.key;
  ctx.init_arrays();
  return in;
}

template <typename Key>
ostream& operator<< (ostream &out, const HELContext<Key> &ctx) {
  writeContextBase(out, *ctx.context);
  // out << *ctx.context;
  out << *ctx.key;
  return out;
}

FHEcontext copy_helib_context(const FHEcontext &other) {
  unsigned long m, p, r;
  std::stringstream buff;
  writeContextBase(buff, other);
  readContextBase(buff, m, p, r);

  FHEcontext ret = FHEcontext(m, p, r);
  buff << other;
  buff >> ret;

  return ret;
}

// Explicit instantiation.
template class HELContext<FHEPubKey>;
template class HELContext<FHESecKey>;
template HELContext<FHEPubKey>::HELContext(const HELContext<FHESecKey> &other);
template istream& operator>> (istream &in, HELContext<FHEPubKey> &ctx);
template istream& operator>> (istream &in, HELContext<FHESecKey> &ctx);
template ostream& operator<< (ostream &out, const HELContext<FHEPubKey> &ctx);
template ostream& operator<< (ostream &out, const HELContext<FHESecKey> &ctx);
