// Bloom Filter
#ifndef CHDL_BLOOM
#define CHDL_BLOOM

#include "hash.h"

namespace chdl {
  template <unsigned SZ, unsigned H, unsigned N>
    node BloomFilter(bvec<N> query, bvec<N> insert, node wr, node clear,
                     unsigned long seed = 0)
  {
    std::ranlux48 rng(seed);
    std::uniform_int_distribution<int> d(1);

    HIERARCHY_ENTER();
    bvec<(1<<SZ)> state, set, check;

    for (unsigned i = 0; i < (1<<SZ); ++i)
      state[i] = Wreg(set[i] || clear, !clear);   

    vec<H, bvec<SZ> > insert_hashes, query_hashes;
    for (unsigned i = 0; i < H; ++i) {
      unsigned seed(d(rng));
      insert_hashes[i] = Hash<SZ>(insert, seed, 0.5);
      query_hashes[i] = Hash<SZ>(query, seed, 0.5);
    }

    for (unsigned i = 0; i < (1<<SZ); ++i) {
      bvec<H> qh_match, ih_match;
      for (unsigned j = 0; j < H; ++j) {
        qh_match[j] = query_hashes[j] == Lit<SZ>(i);
        ih_match[j] = insert_hashes[j] == Lit<SZ>(i);
      }

      set[i] = OrN(ih_match) && wr;
      check[i] = OrN(qh_match);
    }

    node hit = ((check & state) == check);
    HIERARCHY_EXIT();
    return hit;
  }
}
#endif
