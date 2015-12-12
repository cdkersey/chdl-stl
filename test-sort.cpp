#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <stack>

#include <chdl/chdl.h>

#include "sort.h"

#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

const unsigned ITER = 1;

// #define DEBUG

// Output is '1' only when input array is sorted.
template <unsigned N, typename T> node Sorted(const vec<N, T> &x) {
  bvec<N-1> s;
  for (unsigned i = 0; i < N-1; ++i)
    s[i] = (x[i] <= x[i+1]);
  return AndN(s);
}

template <unsigned N, unsigned M> bool test_Sort() {
  unsigned in_v[M], out_v[M];
  bool sorted_v;
  
  vec<M, bvec<N> > in, out(Sort(in));

  node sorted(Sorted(out));

  for (unsigned i = 0; i < M; ++i) {
    in[i] = IngressInt<N>(in_v[i]);
    EgressInt(out_v[i], out[i]);
  }
  Egress(sorted_v, sorted);

  optimize();
  
  srand(0);
  for (unsigned i = 0; i < ITER; ++i) { 
    for (unsigned j = 0; j < M; ++j)
      if (N < 32)
	in_v[j] = rand() & ((1<<N)-1);
      else
        in_v[j] = rand();

    advance();

    #ifdef DEBUG
    cout << "Values:" << endl;
    for (unsigned j = 0; j < M; ++j)
      cout << setw(16) << in_v[j] << setw(16) << out_v[j] << endl;
    cout << endl;
    #endif
    
    if (!sorted_v) return false;
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Sort<16, 4>()) return 1;

  reset();

  if (!test_Sort<32, 8>()) return 1;

  reset();

  if (!test_Sort<52, 16>()) return 1;

  reset();

  if (!test_Sort<31, 32>()) return 1;

  return 0;
}
