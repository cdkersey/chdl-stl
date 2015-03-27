#ifndef CHDL_NET_H
#define CHDL_NET_H

#include <chdl/chdl.h>
#include "ag.h"
#include "dir.h"
#include "queue.h"

namespace chdl {
  template <typename T> using flit =
    ag<STP("ready"), node,
    ag<STP("valid"), node,
    ag<STP("contents"), T> > >;

  // Directional flit types for use on module boundaries.
  template <typename T> using out_flit =
    ag<STP("ready"), in<node>,
    ag<STP("valid"), out<node>,
    ag<STP("contents"), out<T> > > >;

  template <typename T> using in_flit =
    typename chdl::reverse<out_flit<T>>::type;


  // "Connect" function for undirected flits
  template <typename T> void Connect(flit<T> &out, flit<T> &in) {
    _(in, "ready") = _(out, "ready");
    _(out, "valid") = _(in, "valid");
    _(out, "contents") = _(in, "contents");   
  }
  // Simple buffer with no bypass.
  template <unsigned SZ, typename T> void Buffer(flit<T> &out, flit<T> &in) {
    HIERARCHY_ENTER();

    bvec<SZ + 1> occupancy_x, occupancy(Reg(occupancy_x));

    node full(occupancy == Lit<SZ+1>(1ul<<SZ)),
         empty(occupancy == Lit<SZ + 1>(0)),
         push(!full && _(in, "valid")),
         pop(!empty && _(out, "ready"));

    _(out, "valid") = !empty;
    _(in, "ready") = !full;

    bvec<sz<T>::value> in_vec(Flatten(_(in, "contents"))),
                       out_vec(Flatten(_(out, "contents")));

    out_vec = SyncQueue<SZ>(in_vec, push, pop, occupancy_x);

    HIERARCHY_EXIT();
  }

  template <unsigned N, typename T, typename F>
    void Arbiter(flit<T> &out, const F &func, vec<N, flit<T> > &in)
  {
    HIERARCHY_ENTER();

    bvec<N> valid, ready;
    vec<N, bvec<sz<T>::value> > invec;
    for (unsigned i = 0; i < N; ++i) {
      valid[i] = _(in[i], "valid");
      _(in[i], "ready") = ready[i];
      invec[i] = _(in[i], "contents");
    }

    bvec<CLOG2(N)> sel;
    func(sel, valid, _(out, "ready"));
    ready = Decoder(sel, _(out, "ready"));

    _(out, "valid") = OrN(ready & valid);
    _(out, "contents") = Mux(sel, invec);

    HIERARCHY_EXIT();
  }

  // A round-robin selection function for the arbiter.
  template <unsigned N>
    void ArbRR(bvec<CLOG2(N)> &sel, bvec<N> &v, node out_ready)
  {
    HIERARCHY_ENTER();

    bvec<CLOG2(N)> next_pos, pos(Reg(next_pos));

    next_pos = Mux(OrN(v) && out_ready, pos, sel + Lit<CLOG2(N)>(1));

    // Implicitly assumes N is a power of 2. Should probably just pad it if it
    // isn't. TODO: Document this or log it as a bug.
    sel = Lsb(RotR(v, pos)) + pos;

    tap("arb_rr_sel", sel);
    tap("arb_rr_pos", pos);
    tap("arb_rr_rotval", RotL(v, pos));
    tap("arb_rr_nextpos", next_pos);
    tap("arb_rr_v", v);

    HIERARCHY_EXIT();
  }

  // An arbiter selection function which expects only one input to be valid
  template <unsigned N>
    void ArbUniq(bvec<CLOG2(N)> &sel, bvec<N> &v, node out_ready)
  {
    sel = Log2(v);
    ASSERT(!OrN(~(Lit<N>(1)<<sel) & v));
  }

  // A priority arbiter; lower-numbered inputs are higher priority
  template <unsigned N>
    void ArbPriority(bvec<CLOG2(N)> &sel, bvec<N> &v, node out_ready)
  {
    sel = Lsb(v);
  }

  template <unsigned N, typename T, typename F>
    void Router(vec<N, flit<T> > &out, const F &func, flit<T> &in)
  {
    HIERARCHY_ENTER();

    bvec<N> valid, ready;

    for (unsigned i = 0; i < N; ++i) {
      _(out[i], "contents") = _(in, "contents");
      _(out[i], "valid") = valid[i];
      ready[i] = _(out[i], "ready");
    }

    func(valid, _(in, "contents"), _(in, "valid"));

    // Input is ready if all of the selected outputs are also ready
    _(in, "ready") = AndN((~Lit<N>(0) & ~valid) | ready);

    HIERARCHY_EXIT();
  }

  template <unsigned N, typename T> using routable =
    ag<STP("address"), bvec<N>,
    ag<STP("contents"), T> >;

  // Bit-range based routing function.
  template <unsigned M, unsigned A, unsigned N, typename T>
    void RouteAddrBits(bvec<M> &valid, const routable<N, T> &r, node in_valid)
  {
    HIERARCHY_ENTER();

    bvec<CLOG2(M)> sel(_(r, "address")[range<A, A+CLOG2(M)-1>()]);
    valid = Decoder(sel, in_valid);

    HIERARCHY_EXIT();
  }
}

#endif
