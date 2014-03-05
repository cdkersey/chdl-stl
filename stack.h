#ifndef CHDL_STACK_H
#define CHDL_STACK_H

#include <chdl/chdl.h>
#include <chdl/cassign.h>

namespace chdl {
  constexpr bool ISPOW2(unsigned long x) { return !(x & (x-1)); }

  template <unsigned SZ, typename T>
    T Stack(T input, node push, node pop, bvec<SZ+1> &sz);
}

template <unsigned SZ, typename T>
  T chdl::Stack(T input, chdl::node push, chdl::node pop, chdl::bvec<SZ+1> &szi)
{
  using namespace chdl;

  bvec<SZ+1> next_top, top(Reg(next_top));
  szi = next_top;

  node full(top == Lit<SZ+1>(1ul<<SZ)), empty(top == Lit<SZ+1>(0)),
       canPush(!full && !pop), canPop(!empty && !push),
       doPush(push && canPush), doPop(pop && canPop);

  Cassign(next_top).
    IF(doPush, top + Lit<SZ+1>(1)).
    IF(doPop, top - Lit<SZ+1>(1)).
    ELSE(top);

  bvec<SZ> wrAddr(top[range<0,SZ-1>()]),
           rdAddr(next_top[range<0,SZ-1>()] - Lit<SZ>(1));

  const unsigned T_SZ(T::SZ());
  bvec<T_SZ> head(LLRam(rdAddr, bvec<T_SZ>(input), wrAddr, doPush && !pop));

  return T(Mux(doPush, head, input));
}

#endif
