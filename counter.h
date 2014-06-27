#ifndef CHDL_COUNTER
#define CHDL_COUNTER

#include <string>

#include <chdl/chdl.h>

namespace chdl {
  // The default counter size
  const unsigned CTRSIZE(32);

  // Value is returned primarily for testing
  template <unsigned M, unsigned N = CTRSIZE>
    bvec<N> Counter(std::string name, bvec<M> x)
  {
    bvec<N> count;
    count = Reg(count + Zext<N>(x));

    tap(std::string("counter_") + name, count);

    unsigned long *cval(new unsigned long());
    *cval = 0;
    EgressInt(*cval, count);
    finally([cval, name]{
      std::cout << "Counter \"" << name << "\":" << *cval << std::endl;
    });

    return count;
  }

  template <unsigned N = CTRSIZE>
    bvec<N> Counter(std::string name, node x)
  { return Counter(name, bvec<1>(x)); }

  #define COUNTER(x) do { Counter(#x, x); } while(0)
}
#endif
