#include <cstdlib>
#include <iostream>
#include <stack>

#include "hash.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

//#define DEBUG

template <unsigned M, unsigned N>
  unsigned long computeHash(unsigned long x, const vec<M, unsigned long> &m)
{
  unsigned long r = 0;

  for (unsigned long i = 0; i < M; ++i) {
    int count(0);
    unsigned long a(x & m[i]);
    for (unsigned long j = 0; j < N; ++j)
      if ((a >> j)&1) ++count;
    r |= (count&1)<<i;
  }

  return r & ((1ul<<M)-1ul);
}

template <unsigned M, unsigned N, unsigned SEED = 0x5eed> bool test_Hash() {
  unsigned long in_val, out_val;

  bvec<N> input(IngressInt<N>(in_val));
  bvec<M> hash(Hash<M>(input, SEED));

  EgressInt(out_val, hash);

  optimize();

  mt19937 rng(SEED);
  bernoulli_distribution d;
  vec<M, unsigned long> local_hash;
  for (unsigned i = 0; i < M; ++i) {
    local_hash[i] = 0;
    for (unsigned j = 0; j < N; ++j) {
      local_hash[i] <<= 1;
      if (d(rng)) local_hash[i] |= 1;
    }
  }

  unsigned long local_out;
  uniform_int_distribution<unsigned long> id(0, ((1ull<<N) - 1));

  for (unsigned i = 0; i < 4000; ++i) {
    in_val = id(rng);    

    advance();

    local_out = computeHash<M, N>(in_val, local_hash);

    #ifdef DEBUG
    cout << sim_time() << ": local = " << hex << local_out
         << ", out = " << out_val << dec << endl;
    #endif

    if (out_val != local_out) {
      cout << "Discrepancy in output: chdl = " << out_val
           << ", local = " << local_out << endl;
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Hash<8, 32>()) return 1;
  reset();
  if (!test_Hash<8, 64>()) return 1;
  reset();
  if (!test_Hash<16, 64>()) return 1;
  reset();
  if (!test_Hash<17, 60>()) return 1;
  reset();
  if (!test_Hash<32, 64>()) return 1;
  reset();

  return 0;
}
