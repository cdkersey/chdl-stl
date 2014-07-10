#ifndef CHDL_COUNTER
#define CHDL_COUNTER

#include <iostream>
#include <string>

#include <chdl/chdl.h>

namespace chdl {
  // The default counter size
  const unsigned CTRSIZE(32);

  // Value is returned primarily for testing
  template <unsigned M, unsigned N = CTRSIZE>
    bvec<N> Counter(std::string name, bvec<M> x, node enable = Lit(1))
  {
    bvec<N> count;
    count = Wreg(enable, count + Zext<N>(x));

    tap(std::string("counter_") + name, count);
    tap(std::string("increment_counter_") + name, x);

    unsigned *ctrval = new unsigned(0);
    EgressInt(*ctrval, count);
    finally([name, ctrval]() {
      std::cout << "Counter \"" << name << "\": " << *ctrval << std::endl;
    });

    return count;
  }

  template <unsigned N = CTRSIZE>
    bvec<N> Counter(std::string name, node x)
  { return Counter(name, Lit<1>(1), x); }

  #define COUNTER(x) do { Counter(#x, x); } while(0)
}
#endif
