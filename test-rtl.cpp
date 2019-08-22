#include <cstdlib>
#include <iostream>
#include <queue>

#include <chdl/egress.h>

#include "rtl.h"

using namespace std;
using namespace chdl;

void NumConsole(bvec<32> x, node valid) {
  unsigned int *p = new unsigned int(0);
  EgressInt(*p, x);
  EgressFunc([=](bool v) {
    if (v) cout << *p << endl;
  }, valid, false);
}

bool test_rtl() {
  { const unsigned N(250), NN(2*CLOG2(N + 1));

    rtl_reg<bvec<N> > a;
    rtl_reg<bvec<NN> > i(Lit<NN>(2)), j;
    rtl_reg<node> init(Lit(1)), find, mark, readout;

    IF(init) {
      IF(i == Lit<NN>(N)) {
        i = Lit<NN>(2); init = Lit(0);  find = Lit(1);
      } ELSE {
        a[Zext<CLOG2(N)>(i)] = Lit(1);  i += Lit<NN>(1);
      } ENDIF;
    } ELIF(find) {
      IF(i * i >= Lit<NN>(N)) {
        i = Lit<NN>(0); find = Lit(0); readout = Lit(1);
      } ELIF(Mux(Zext<CLOG2(N)>(i), a)) {
        find = Lit(0);  mark = Lit(1);  j = i * i;
      } ELSE {
        i += Lit<NN>(1);
      } ENDIF;
    } ELIF(mark) {
      IF(j >= Lit<NN>(N)); {
        i += Lit<NN>(1);  find = Lit(1);  mark = Lit(0);
      } ELSE {
        a[Zext<CLOG2(N)>(j)] = Lit(0);  j += i;
      } ENDIF;
    } ELIF(readout) {
      IF(i == Lit<NN>(N)) {
	readout = Lit(0);
      } ELSE {
        i += Lit<NN>(1);
      } ENDIF;
    } ENDIF;

    NumConsole(Zext<32>(i), readout && Mux(Zext<CLOG2(N)>(i), a));

    TAP(i); TAP(j); TAP(mark); TAP(readout); TAP(a); TAP(init); TAP(find);
  }

  optimize();

  for (unsigned i = 0; i < 5000; ++i) {
    advance();
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_rtl()) return 1;
  reset();

  return 0;
}
