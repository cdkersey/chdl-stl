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
    void Scratchpad(mem_resp<B, N, I> &resp_buf, mem_req<B, N, A, I> &req)
  {
    mem_resp<B, N, I> resp;

    bvec<N> wr = 
        bvec<N>(_(_(req, "contents"), "wr")) & _(_(req, "contents"), "mask");
    bvec<SZ> addr(Zext<SZ>(_(_(req, "contents"), "addr")));
    vec<N, bvec<B> > d(_(_(req, "contents"), "data"));

    for (unsigned i = 0; i < N; ++i)
      _(_(resp, "contents"), "data")[i] = Syncmem(addr, d[i], wr[i]);

    // Load linked/store conditional can only be handled on multi-ported memory
    // interfaces. Since we don't support it, we'll always fail LL/SC.
    _(_(resp, "contents"), "llsc") = Reg(_(_(req, "contents"), "llsc"));
    _(_(resp, "contents"), "id") = Reg(_(_(req, "contents"), "id"));
    _(_(resp, "contents"), "llsc_suc") = Lit(0);


    // Backpressure passthrough. Don't accept any new requets while we're not
    // shipping back the previous ones. This can lead to deadlock. If this
    // happens, network buffers are needed.
    _(req, "ready") = _(resp, "ready");
    _(resp, "valid") = Reg(_(req, "valid"));
    _(_(resp, "contents"), "id") = Reg(_(_(req, "contents"), "id"));

    Buffer<1>(resp_buf, resp);
  }

};
#endif
