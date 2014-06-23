#include <cstdlib>
#include <iostream>
#include <stack>

#include "lfsr.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

//#define DEBUG

template <unsigned M, unsigned N, unsigned TAP> bool test_Lfsr() {
  bvec<M> l = Lfsr<M, N, TAP, 0x5eed5eed5eed5eedull>(Lit(1));

  unsigned long l_val;
  EgressInt(l_val, l);

  optimize();

  vector<bool> local(N);
  unsigned long local_out;
  for (unsigned i = 0; i < N; ++i) local[i] = (0x5eed5eed5eed5eedull>>i)&1;

  for (unsigned i = 0; i < 4000; ++i) {

    advance();

    local_out = 0;
    for (unsigned i = 0; i < M; ++i) {
      bool new_val = local[N-1] != local[TAP-1];
      for (unsigned j = N-1; j > 0; --j) local[j] = local[j-1];
      local[0] = new_val;
      local_out <<= 1;
      local_out |= new_val;
    }

    #ifdef DEBUG
    cout << sim_time() << ": l = " << l_val << ", " << local_out << endl;
    #endif

    if (l_val != local_out) {
      cout << "Discrepancy on sim cycle " << sim_time() << "; Lfsr<"
           << M << ',' << N << ',' << TAP << '>' << endl;
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Lfsr<8, 15, 1>()) return 1;
  reset();
  if (!test_Lfsr<16, 17, 3>()) return 1;
  reset();
  if (!test_Lfsr<32, 33, 13>()) return 1;
  reset();
  if (!test_Lfsr<32, 1023, 7>()) return 1;
  reset();

  return 0;
}
