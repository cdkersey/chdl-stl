#ifndef CHDL_COUNTER
#define CHDL_COUNTER

#include <string>

#include <chdl/chdl.h>

namespace chdl {
  // The default counter size
  const unsigned CTRSIZE(32);

  // Value is returned primarily for testing
  template <unsigned N = CTRSIZE> bvec<N> Counter(std::string name, node x) {
    bvec<N> count;
    count = Wreg(x, count + Lit<N>(1));

    tap(std::string("counter_") + name, count);

    return count;
  }

  #define COUNTER(x) do { Counter(#x, x); } while(0)
}
#endif
