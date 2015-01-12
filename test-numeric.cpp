// TODO: Automate this test; the hex numbers mean nothing.

#include <random>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <chdl/chdl.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>

#include "numeric.h"

const unsigned TRIALS(100);

using namespace std;
using namespace chdl;

template<unsigned N> uint64_t RandInt() {
  static default_random_engine e(0);
  uniform_int_distribution<uint64_t> d(0,(1ull<<N)-1);
  return d(e);
}

template<> uint64_t RandInt<64>() {
  static default_random_engine e(0);
  uniform_int_distribution<uint64_t> d(0, ~0ull);
  return d(e);
}

template <unsigned N> uint64_t Trunc(uint64_t x) {
  uint64_t mask((1ull<<N)-1);
  return x & mask;
}

template<> uint64_t Trunc<64>(uint64_t x) { return x; }

// Run a binary operation on two inputs of type T across TRIALS trials
template <typename T, typename H>
  void TestFunc(const H &h)
{
  const unsigned N(sz<T>::value);

  static uint64_t out, in_a, in_b;

  T a, b, s(h(a, b));
  Flatten(a) = IngressInt<N>(in_a);
  Flatten(b) = IngressInt<N>(in_b);

  EgressInt(out, Flatten(s));

  optimize();

  for (unsigned i = 0; i < TRIALS; ++i) {
    in_a = RandInt<N>();
    in_b = RandInt<N>();
    advance();
    cout << hex << in_a << ' ' << in_b << ' ' << out << endl << dec;
  }
}

template <typename T> void test_add() {
  TestFunc<T>([](const T &a, const T& b) { return a + b; });
}

int main(int argc, char** argv) {
  test_add<fp16      >(); reset();
  test_add<fp32      >(); reset();
  test_add<fp64      >(); reset();
  test_add<fxp<7,7>  >(); reset();
  test_add<fxp<13,13>>(); reset();
  test_add<fxp<8,24> >(); reset();
  test_add<si<5>     >(); reset();
  test_add<si<6>     >(); reset();
  test_add<ui<7>     >(); reset();

  return 0;
}
