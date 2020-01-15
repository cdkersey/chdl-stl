#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <map>
#include <string>
#include <vector>
#include <chdl/chdl.h>

#define CHDL_ALG_MACROS
#include "alg.h"
#undef CHDL_ALG_MACROS

using namespace chdl;
using namespace std;

void NumConsole(bvec<32> x, node valid) {
  unsigned int *p = new unsigned int(0);
  EgressInt(*p, x);
  EgressFunc([=](bool v) {
    if (v) cout << *p << endl;
  }, valid, false);
}

template <unsigned N> void demo() {
  const unsigned NN(CLOG2(2*N + 2));
  var<bvec<NN> > i, j;
  var<bvec<N> > a;

  alg_st()->label();
  FOR(i = Lit<NN>(2), i < Lit<NN>(N), i = i + Lit<NN>(1), {
    a = bvec<N>(a) | (Lit<N>(1) << Zext<CLOG2(N)>(bvec<NN>(i)));
  });

  FOR(i = Lit<NN>(2), i < Lit<NN>(sqrt(N)), i = i + Lit<NN>(1), {
    IF(Mux(Zext<CLOG2(N)>(bvec<NN>(i)), bvec<N>(a)), {
      FOR(j = i * i, j < Lit<NN>(N), j = j + bvec<NN>(i), {
        a = bvec<N>(a) & ~(Lit<N>(1) << Zext<CLOG2(N)>(bvec<NN>(j)));
      });
    });
  });

  FOR(i = Lit<NN>(2), i < Lit<NN>(N), i = i + Lit<NN>(1), {
    IF(Mux(Zext<CLOG2(N)>(bvec<NN>(i)), bvec<N>(a)), {
      NumConsole(Zext<32>(bvec<NN>(i)), alg_st()->in_cur());
    });
  });

  alg_st()->br(alg_st()->label());

  TAP(i);
  TAP(j);
  TAP(a);

  alg_st()->gen();
}

int main() {
  demo<64>();

  optimize();

  ofstream vcd("test-alg.vcd");
  run(vcd, 1000);
  
  return 0;
}
