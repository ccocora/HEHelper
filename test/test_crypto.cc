#include <iostream>
#include <cassert>
#include <memory>
#include <sstream>

#include "HELContext.h"
#include "EncBit.h"

using namespace std;

PrivateCtx priv(10, 5);
PublicCtx pub(priv);

// Context serialization.
void test1() {
  std::stringstream ss;
  ss << *priv.key;
  FHESecKey result;
  ss >> result;
  assert(*priv.key == result);
}


int main(int argc, char **argv) {
  test1();

  return 0;
}