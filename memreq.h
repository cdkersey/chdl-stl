#ifndef CHDL_MEMREQ_H
#define CHDL_MEMREQ_H

#include <chdl/chdl.h>
#include <chdl/cassign.h>

#include "ag.h"
#include "net.h"

namespace chdl {

  // Memory request N B-bit bytes across with A address bits (word addressed)
  // All requests are paired with responses by unique I-bit-long identifiers
  template <unsigned B, unsigned N, unsigned A, unsigned I>
    using mem_req_payload =
      ag<STP("wr"), node,
      ag<STP("mask"), bvec<N>, // Write mask
      ag<STP("addr"), bvec<A>,
      ag<STP("data"), vec<N, chdl::bvec<B> >,
      ag<STP("llsc"), node, // Load linked/store conditional for atomics
      ag<STP("id"), bvec<I> > > > > > >;

  // Memory response corresponding to mem_req<B, N, A, I> for any A.
  template <unsigned B, unsigned N, unsigned I> using mem_resp_payload =
      ag<STP("data"), vec<N, bvec<B> >,
      ag<STP("id"), bvec<I>,
      ag<STP("wr"), node,
      ag<STP("llsc"), node, // Was an atomic
      ag<STP("llsc_suc"), node> > > > >; // Atomic was successful

  // Wrap the payload types with filts for use with CHDL net.
  template <unsigned B, unsigned N, unsigned A, unsigned I> using mem_req =
    flit<mem_req_payload<B, N, A, I>>;

  template <unsigned B, unsigned N, unsigned I> using mem_resp =
    flit<mem_resp_payload<B, N, I>>;

  // Input/output types for directional port connections.
  template <unsigned B, unsigned N, unsigned A, unsigned I> using in_mem_req =
    in_flit<mem_req_payload<B, N, A, I>>;

  template <unsigned B, unsigned N, unsigned A, unsigned I> using out_mem_req =
    out_flit<mem_req_payload<B, N, A, I>>;

  template <unsigned B, unsigned N, unsigned I> using in_mem_resp =
    in_flit<mem_resp_payload<B, N, I>>;

  template <unsigned B, unsigned N, unsigned I> using out_mem_resp =
    out_flit<mem_resp_payload<B, N, I>>;

  // Directed and undirected ports; both the req and the resp!
  template <unsigned B, unsigned N, unsigned A, unsigned I> using mem_port =
    ag<STP("req"), mem_req<B, N, A, I>,
    ag<STP("resp"), mem_resp<B, N, I> > >;

  template <unsigned B, unsigned N, unsigned A, unsigned I> using out_mem_port =
    ag<STP("req"), out_mem_req<B, N, A, I>,
    ag<STP("resp"), in_mem_resp<B, N, I> > >;

  template <unsigned B, unsigned N, unsigned A, unsigned I> using in_mem_port =
    ag<STP("req"), in_mem_req<B, N, A, I>,
    ag<STP("resp"), out_mem_resp<B, N, I> > >;

  // Connect memory port, preserving proper signal directions.
  template <unsigned B, unsigned N, unsigned A, unsigned I>
    void Connect(mem_port<B,N,A,I> &out, mem_port<B, N, A, I> &in)
  {
    Connect(_(in,"resp"), _(out,"resp"));
    Connect(_(out,"req"), _(in,"req"));
  }
    
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
      _(_(resp, "contents"), "data")[i] = Syncmem(Zext<SZ>(addr), d[i], wr[i]);

    _(_(resp, "contents"), "wr") = Wreg(next, _(_(req, "contents"), "wr"));
    _(_(resp, "contents"), "id") = Wreg(next, _(_(req, "contents"), "id"));    

    node next_valid, valid(Reg(next_valid));
    _(resp, "valid") = valid;
    Cassign(next_valid).
      IF(!next && valid && _(resp, "ready"), Lit(0)).
      IF(next, Lit(1)).
      ELSE(valid);
  }

  template<unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
    void Scratchpad(mem_port<B, N, A, I> &port)
  { Scratchpad<SZ>(_(port, "resp"), _(port, "req")); }

  // memreq-based ROM
  template <unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
    void Rom(mem_resp<B, N, I> resp, mem_req<B, N, A, I> req,
             const char *filename)
  {
    node next_full, full(Reg(next_full)), read;
    read = _(req, "valid") && _(req, "ready") && !_(_(req, "contents"), "wr");

    Cassign(next_full).
      IF(!full && read, Lit(1)).
      IF(full && _(resp, "ready"), Lit(0)).
      ELSE(full);

    _(resp, "valid") = Reg(read) || full;
    _(req, "ready") = !full || _(resp, "ready");

    bvec<SZ> addr(Zext<SZ>(_(_(req, "contents"), "addr")));
    Flatten(_(_(resp, "contents"), "data")) =
      Wreg(read, LLRom<SZ, B*N>(addr, filename));
  }

  template <unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
    void Rom(mem_port<B, N, A, I> &port, const char *filename)
  { Rom<SZ>(_(port, "resp"), _(port, "req"), filename); }

  
  // Safe information-preserving expand-to-multiple-bytes
  template <unsigned B, unsigned N, unsigned A, unsigned I>
    void SplitBytes(mem_port<B, N, A, I> &out, mem_port<B*N, 1, A, I> &in)
  {
    mem_req<B, N, A, I> &req(_(in, "req"));
    mem_resp<B, N, I> &resp(_(out, "resp"));
    
    _(_(in, "req"),"ready") = _(_(out,"req"),"ready");
    _(_(out,"req"),"valid") = _(req,"valid");
    _(_(_(out,"req"),"contents"),"id") = _(_(req,"contents"),"id");
    Flatten(_(_(_(out,"req"),"contents"),"data")) =
      _(_(req,"contents"),"data")[0];
    _(_(_(out,"req"),"contents"),"addr") = (_(_(req,"contents"),"addr"));
    _(_(_(out,"req"),"contents"),"llsc") = _(_(req,"contents"),"llsc");
    _(_(_(out,"req"),"contents"),"wr") = _(_(req,"contents"),"wr");
    _(_(_(out,"req"),"contents"),"mask")[0] =
      OrN(_(_(req,"contents"),"mask"));

    _(_(in,"resp"),"valid") = _(resp,"valid");
    _(_(_(in,"resp"),"contents"),"id") = _(_(resp,"contents"),"id");
    _(_(_(in,"resp"),"contents"),"data")[0] =
      Flatten(_(_(resp,"contents"),"data"));
    _(_(_(in,"resp"),"contents"),"wr") = _(_(resp,"contents"),"wr");
    _(_(_(in,"resp"),"contents"),"llsc") = _(_(resp,"contents"),"llsc");
    _(_(_(in,"resp"),"contents"),"llsc_suc") = _(_(resp,"contents"),"llsc_suc");
  }

  // Potentially information-destroying reduce-to-single-byte
  template <unsigned B, unsigned N, unsigned A, unsigned I>
    void CombineBytes(mem_port<B*N, 1, A, I> &out, mem_port<B, N, A, I> &in)
  {
    mem_req<B, N, A, I> &req(_(in, "req"));
    mem_resp<B, N, I> &resp(_(out, "resp"));
    
    _(_(in, "req"),"ready") = _(_(out,"req"),"ready");
    _(_(out,"req"),"valid") = _(req,"valid");
    _(_(_(out,"req"),"contents"),"id") = _(_(req,"contents"),"id");
    _(_(_(out,"req"),"contents"),"data")[0] =
      Flatten(_(_(req,"contents"),"data"));
    _(_(_(out,"req"),"contents"),"addr") = (_(_(req,"contents"),"addr"));
    _(_(_(out,"req"),"contents"),"llsc") = _(_(req,"contents"),"llsc");
    _(_(_(out,"req"),"contents"),"wr") = _(_(req,"contents"),"wr");
    _(_(_(out,"req"),"contents"),"mask") = OrN(_(_(req,"contents"),"mask"));

    _(_(in,"resp"),"valid") = _(resp,"valid");
    _(_(_(in,"resp"),"contents"),"id") = _(_(resp,"contents"),"id");
    Flatten(_(_(_(in,"resp"),"contents"),"data")) =
      _(_(resp,"contents"),"data")[0];
    _(_(_(in,"resp"),"contents"),"wr") = _(_(resp,"contents"),"wr");
    _(_(_(in,"resp"),"contents"),"llsc") = _(_(resp,"contents"),"llsc");
    _(_(_(in,"resp"),"contents"),"llsc_suc") = _(_(resp,"contents"),"llsc_suc");
  }

  // Extend or shrink addresses
  template <unsigned B, unsigned N, unsigned A1, unsigned I, unsigned A2>
    void ExtAddr(mem_port<B, N, A2, I> &out, mem_port<B, N, A1, I> &in)
  {
    mem_req<B, N, A1, I> &req(_(in, "req"));
    mem_resp<B, N, I> &resp(_(out, "resp"));
    
    _(_(in, "req"),"ready") = _(_(out,"req"),"ready");
    _(_(out,"req"),"valid") = _(req,"valid");
    _(_(_(out,"req"),"contents"),"id") = _(_(req,"contents"),"id");
    _(_(_(out,"req"),"contents"),"data") = _(_(req,"contents"),"data");
    _(_(_(out,"req"),"contents"),"addr") =
      Zext<A2>(_(_(req,"contents"),"addr"));
    _(_(_(out,"req"),"contents"),"llsc") = _(_(req,"contents"),"llsc");
    _(_(_(out,"req"),"contents"),"wr") = _(_(req,"contents"),"wr");
    _(_(_(out,"req"),"contents"),"mask") = _(_(req,"contents"),"mask");

    _(_(in,"resp"),"valid") = _(resp,"valid");;
    _(_(_(in,"resp"),"contents"),"id") = _(_(resp,"contents"),"id");
    _(_(_(in,"resp"),"contents"),"data") = _(_(resp,"contents"),"data");
    _(_(_(in,"resp"),"contents"),"wr") = _(_(resp,"contents"),"wr");
    _(_(_(in,"resp"),"contents"),"llsc") = _(_(resp,"contents"),"llsc");
    _(_(_(in,"resp"),"contents"),"llsc_suc") = _(_(resp,"contents"),"llsc_suc");
  }

  // Add a fixed "tag" to IDs in memory requests, strip it from responses.
  template <unsigned T, unsigned B, unsigned N, unsigned A, unsigned I>
    void TagID(mem_port<B, N, A, I + T> &out, mem_port<B, N, A, I> &in, int t)
  {
    mem_req<B, N, A, I> req; Connect(req, _(in, "req"));
    mem_resp<B, N, I + T> resp; Connect(resp, _(out, "resp"));

    bvec<T> tag(Lit<T>(t)),
            resp_tag(_(_(resp, "contents"), "id")[range<I, I+T-1>()]);
    node resp_valid(_(resp, "valid")), req_valid(_(req, "valid"));

    _(_(in, "req"),"ready") = _(_(out,"req"),"ready");
    _(_(out,"req"),"valid") = req_valid;
    _(_(_(out,"req"),"contents"),"id") = Cat(tag, _(_(req,"contents"),"id"));
    _(_(_(out,"req"),"contents"),"data") = _(_(req,"contents"),"data");
    _(_(_(out,"req"),"contents"),"addr") = _(_(req,"contents"),"addr");
    _(_(_(out,"req"),"contents"),"llsc") = _(_(req,"contents"),"llsc");
    _(_(_(out,"req"),"contents"),"wr") = _(_(req,"contents"),"wr");
    _(_(_(out,"req"),"contents"),"mask") = _(_(req,"contents"),"mask");
  
    _(_(in,"resp"),"valid") = resp_valid;
    _(_(_(in,"resp"),"contents"),"id") =
      _(_(resp,"contents"),"id")[range<0,I-1>()];
    _(_(_(in,"resp"),"contents"),"data") = _(_(resp,"contents"),"data");
    _(_(_(in,"resp"),"contents"),"wr") = _(_(resp,"contents"),"wr");
    _(_(_(in,"resp"),"contents"),"llsc") = _(_(resp,"contents"),"llsc");
    _(_(_(in,"resp"),"contents"),"llsc_suc") = _(_(resp,"contents"),"llsc_suc");

    _(resp,"ready") = _(_(in,"resp"),"ready");
  }

  template <unsigned M, unsigned B, unsigned N, unsigned I>
    void RouteByTag(bvec<M> &valid, const mem_resp_payload<B, N, I> &r, node v)
  {
    for (unsigned i = 0; i < M; ++i)
      valid[i] = v && (_(r, "id")[range<I-CLOG2(M),I-1>()] == Lit<CLOG2(M)>(i));
  }
  
  // Funnel many input ports to a shared memory device
  template <unsigned M, unsigned B, unsigned N, unsigned A, unsigned I>
    void Share(mem_port<B,N,A,I+CLOG2(M)> &out, vec<M, mem_port<B,N,A,I> > &in)
  {
    vec<M, mem_port<B, N, A, I + CLOG2(M)> > in_tagged;
    vec<M, mem_req<B, N, A, I + CLOG2(M)> > in_tagged_reqs;
    vec<M, mem_resp<B, N, I + CLOG2(M)> > resps;
    bvec<M> resp_sel(
      Decoder(_(_(_(out,"resp"),"contents"),"id")[range<I,I+CLOG2(M)-1>()],
	      _(_(out,"resp"),"valid"))
    );
    for (unsigned t = 0; t < M; ++t) {
      TagID<CLOG2(M)>(in_tagged[t], in[t], t);
      Connect(in_tagged_reqs[t],_(in_tagged[t], "req"));
      Connect(_(in_tagged[t],"resp"),resps[t]);
    }
    Arbiter(_(out,"req"), ArbRR<M>, in_tagged_reqs);
    Router(resps, RouteByTag<M,B,N,I+CLOG2(M)>,_(out,"resp"));
  }
};
#endif
