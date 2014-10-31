#ifndef CHDL_MEMREQ_H
#define CHDL_MEMREQ_H

#include <chdl/chdl.h>
#include <chdl/cassign.h>

#include "ag.h"
#include "net.h"

namespace chdl {
  // Memory request N B-bit bytes across with A address bits (word addressed)
  // All requests are paired with responses by unique I-bit-long identifiers
  template <unsigned B, unsigned N, unsigned A, unsigned I> using mem_req =
    flit<
      ag<STP("wr"), node,
      ag<STP("mask"), bvec<N>, // Write mask
      ag<STP("addr"), bvec<A>,
      ag<STP("size"), bvec<chdl::CLOG2(N + 1)>,
      ag<STP("data"), vec<N, chdl::bvec<B> >,
      ag<STP("llsc"), node, // Load linked/store conditional for atomics
      ag<STP("id"), bvec<I>
    > > > > > > > >;

  // Memory response corresponding to mem_req<B, N, A, I> for any A.
  template <unsigned B, unsigned N, unsigned I> using mem_resp =
    flit<
      ag<STP("data"), vec<N, bvec<B> >,
      ag<STP("id"), bvec<I>,
      ag<STP("wr"), node,
      ag<STP("llsc"), node, // Was an atomic
      ag<STP("llsc_suc"), node // Atomic was successful
    > > > > > >;

  // A very basic 1-cycle latency SRAM using the mem_req interface. Addresses
  // are truncated.
  template<unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
    void Scratchpad(mem_resp<B, N, I> &resp, mem_req<B, N, A, I> &req)
  {
    _(req, "ready") = _(resp, "ready");

    node next(_(req, "valid") && _(req, "ready"));

    vec<N, bvec<B> > d = _(_(req, "contents"), "data");
    bvec<N> wr = bvec<N>(next && _(_(req, "contents"), "wr"))
                   & _(_(req, "contents"), "mask");
    bvec<A> addr = Latch(!next, _(_(req, "contents"), "addr"));

    for (unsigned i = 0; i < N; ++i)
      _(_(resp, "contents"), "data")[i] = Syncmem(addr, d[i], wr[i]);

    _(_(resp, "contents"), "wr") = Wreg(next, _(_(req, "contents"), "wr"));
    _(_(resp, "contents"), "id") = Wreg(next, _(_(req, "contents"), "id"));    

    node next_valid, valid(Reg(next_valid));
    _(resp, "valid") = valid;
    Cassign(next_valid).
      IF(!next && valid && _(resp, "ready"), Lit(0)).
      IF(next, Lit(1)).
      ELSE(valid);
  }

};
#endif
