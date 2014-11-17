#ifndef CHDL_LFSR_H
#define CHDL_LFSR_H

#include <chdl/chdl.h>

namespace chdl {

  template <unsigned N, unsigned TAP> bvec<N> LfsrStage(bvec<N> &in) {
    HIERARCHY_ENTER();
    bvec<N> r;
    for (unsigned i = 1; i < N; ++i) r[i] = in[i-1];
    r[0] = Xor(in[N-1], in[TAP-1]);
    HIERARCHY_EXIT();
    return r;
  }

  template <unsigned M, unsigned N, unsigned TAP, unsigned long long SEED>
    bvec<M> Lfsr(node advance = Lit(1))
  {
    HIERARCHY_ENTER();
    vec<M+1, bvec<N> > stages;
    stages[0] = Wreg(advance, stages[M], SEED);
    for (unsigned i = 1; i < M + 1; ++i)
      stages[i] = LfsrStage<N, TAP>(stages[i-1]);
    return stages[M][range<0, M-1>()];
    HIERARCHY_EXIT();
  }

  template <unsigned M, unsigned N, unsigned TAP>
    bvec<M> Lfsr(node advance, unsigned long long seed)
  {
    HIERARCHY_ENTER();
    vec<M+1, bvec<N> > stages;
    stages[0] = Wreg(advance, stages[M], seed);
    for (unsigned i = 1; i < M + 1; ++i)
      stages[i] = LfsrStage<N, TAP>(stages[i-1]);
    return stages[M][range<0, M-1>()];
    HIERARCHY_EXIT();
  }
}

#endif
