#ifndef CHDL_QUEUE_H
#define CHDL_QUEUE_H

#include <chdl/chdl.h>
#include <chdl/cassign.h>

namespace chdl {
  constexpr bool ISPOW2(unsigned long x) { return !(x & (x-1)); }

  template <unsigned SZ, typename T>
    T Queue(T input, node push, node pop, bvec<SZ+1> &sz);

  template <unsigned SZ, typename T>
    T SyncQueue(T input, node push, node pop, bvec<SZ+1> &sz);
}

template <unsigned SZ, typename T>
  T chdl::SyncQueue
    (T input, chdl::node push, chdl::node pop, chdl::bvec<SZ+1> &szi)
{
  using namespace chdl;

  bvec<SZ> next_front, front(Reg(next_front)), next_back, back(Reg(next_back));
  bvec<SZ+1> next_sz, sz(Reg(next_sz));
  szi = next_sz;

  node full(sz == Lit<SZ+1>(1ul<<SZ)), empty(sz == Lit<SZ+1>(0)),
       canPush(!full || pop), canPop(!empty || push),
       doPush(push && canPush), doPop(pop && canPop);

  Cassign(next_sz).
    IF(push).
      IF(pop || full, sz).
      ELSE(sz + Lit<SZ+1>(1)).
    END().IF(pop).
      IF(sz == Lit<SZ+1>(0), Lit<SZ+1>(0)).
      ELSE(sz - Lit<SZ+1>(1)).
    END().ELSE(sz);

  Cassign(next_front).
    IF(doPop, front + Lit<SZ>(1)).
    ELSE(front);

  Cassign(next_back).
    IF(doPush, back + Lit<SZ>(1)).
    ELSE(back);

  T head(Syncmem(next_front, input, back, doPush));

  return Mux(Reg(doPush && next_front == back), head, Reg(input));
}

template <unsigned SZ, typename T>
  T chdl::Queue(T input, chdl::node push, chdl::node pop, chdl::bvec<SZ+1> &szi)
{
  using namespace chdl;

  bvec<SZ> next_front, front(Reg(next_front)), next_back, back(Reg(next_back));
  bvec<SZ+1> next_sz, sz(Reg(next_sz));
  szi = next_sz;

  node full(sz == Lit<SZ+1>(1ul<<SZ)), empty(sz == Lit<SZ+1>(0)),
       canPush(!full || pop), canPop(!empty || push),
       doPush(push && canPush), doPop(pop && canPop);

  Cassign(next_sz).
    IF(push).
      IF(pop || full, sz).
      ELSE(sz + Lit<SZ+1>(1)).
    END().IF(pop).
      IF(sz == Lit<SZ+1>(0), Lit<SZ+1>(0)).
      ELSE(sz - Lit<SZ+1>(1)).
    END().ELSE(sz);

  Cassign(next_front).
    IF(doPop, front + Lit<SZ>(1)).
    ELSE(front);

  Cassign(next_back).
    IF(doPush, back + Lit<SZ>(1)).
    ELSE(back);

  T head(LLRam(next_front, input, back, doPush));

  return Mux(doPush && next_front == back, head, input);
}



#endif
