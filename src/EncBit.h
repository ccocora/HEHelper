#ifndef ENCBIT_H
#define ENCBIT_H

#include <memory>

#include "FHE.h"

#include "HELContext.h"

using namespace std;

// Wrapper for 1 bit of HELib.
class EncBit {

public:
  EncBit();

  EncBit(std::shared_ptr<PublicCtx> context_);

  EncBit(std::shared_ptr<PublicCtx> context_, bool data);

  void encrypt(bool data);

  bool decrypt(const PrivateCtx& context_);

  EncBit& operator=(const EncBit& rhs);

  EncBit operator!() const;

  EncBit operator^(const EncBit &rhs) const;

  EncBit operator&(const EncBit &rhs) const;

  EncBit operator|(const EncBit &rhs) const;

  EncBit operator==(const EncBit &rhs) const;

  EncBit operator&&(const EncBit& rhs) const;

  EncBit operator||(const EncBit& rhs) const;

  // Compare and swap operation. Stores rhs if conditional is true, unchanged otherwise.
  void cas(const EncBit& condition, const EncBit& rhs);


private:
  Ctxt data;
  FHEPubKey& m_key;
  EncryptedArray& m_ea;
};

#endif //ENCBIT_H
