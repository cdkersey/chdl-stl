// Binary matrix hash
#ifndef CHDL_HASH
#define CHDL_HASH
#include <fstream>
#include <random>
#include <vector>
#include <chdl/chdl.h>
#include <chdl/counter.h>

namespace chdl {
  template <unsigned M, unsigned N>
    bvec<M> Hash(bvec<N> x, vec<M, bvec<N> > m)
  {
    HIERARCHY_ENTER();
    bvec<M> r;

    for (unsigned i = 0; i < M; ++i)
      r[i] = XorN(m[i] & x);

    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned M, unsigned N>
    bvec<M> Hash(bvec<N> x, const vec<M, unsigned long> &m)
  {
    vec<M, bvec<N> > mv;
    for (unsigned i = 0; i < M; ++i)
      mv[i] = Lit<N>(m[i]);

    return Hash(x, mv);
  }

  template <unsigned M, unsigned N>
    bvec<M> Hash(bvec<N> x, unsigned long seed, float pop = 0.5)
  {
    std::mt19937 rng(seed);
    std::bernoulli_distribution d(pop);

    vec<M, unsigned long> m;
    for (unsigned i = 0; i < M; ++i) {
      m[i] = 0;
      for (unsigned j = 0; j < N; ++j) {
        m[i] <<= 1;
        if (d(rng)) m[i] |= 1;
      }
    }
  
    return Hash(x, m);
  }
}
#endif
