#include <cstdlib>
#include <iostream>
#include <queue>

#include "counter.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

// #define DEBUG

node Lfsr(unsigned seed) {
  bvec<16> sr;
  TAP(sr);
  for (unsigned i = 1; i < 16; ++i) sr[i] = Reg(sr[i-1], (seed>>i)&1);

  // This is a max-period LFSR for 16 bits
  bvec<2> taps;
  taps[0] = sr[0];
  taps[1] = sr[14];

  TAP(taps);
  node next = XorN(taps);
  TAP(next);

  sr[0] = Reg(!next, seed&1);

  return next;
}

bool test_Counter() {
  node x = Lfsr(0x5eed), y = Lfsr(0x1234);

  bvec<2> xy;
  xy[0] = y;
  xy[1] = x;

  bvec<CTRSIZE> cx = Counter("x", x), cy = Counter("y", y),
                cxy = Counter("xy", xy);;

  unsigned long cxval, cyval, cxyval, xyval;
  bool xval(false), yval(false);
  Egress(xval, x);
  Egress(yval, y);
  EgressInt(xyval, xy);
  EgressInt(cxval, cx);
  EgressInt(cyval, cy);
  EgressInt(cxyval, cxy);

  optimize();

  unsigned long ext_xcount(0), ext_ycount(0), ext_xycount(0);
  for (unsigned cyc = 0; cyc < 10000; ++cyc) {
    if (xval) { ext_xcount++; ext_xycount += 2; }
    if (yval) { ext_ycount++; ext_xycount++; }

    advance();

    if (ext_ycount != cyval) return false;
    if (ext_xcount != cxval) return false;
    if (ext_xycount != cxyval) return false;
  }

  call_final_funcs();

  return true;
}

int main(int argc, char** argv) {

  if (!test_Counter()) return 1;
  reset();

  return 0;
}
