#ifndef CHDL_MAP_H
#define CHDL_MAP_H

#include <chdl/chdl.h>
#include <chdl/cassign.h>

namespace chdl {
  template <unsigned SZ, unsigned K, unsigned V>
    bvec<V> Map(node full, node inmap, bvec<K> qkey, bvec<K> dkey,
                bvec<V> d, node write, node erase);

  template <unsigned SZ, unsigned N>
    node Set(node full, bvec<N> qa, bvec<N> da, node insert, node erase);
}

static chdl::node RSReg(chdl::node r, chdl::node s) {
  using namespace chdl;

  node next, val(Reg(next));
  Cassign(next).
    IF(r && !s && val, Lit(0)).
    IF(!r && s && !val, Lit(1)).
    ELSE(val);

  return val;
}

template <unsigned SZ, unsigned K, unsigned V>
  chdl::bvec<V> chdl::Map(chdl::node full, chdl::node inmap, chdl::bvec<K> qkey,
                          chdl::bvec<K> dkey, chdl::bvec<V> d, chdl::node wr,
                          chdl::node erase)
{
  using namespace chdl;

  bus<V> b;
  bvec<1<<SZ> match, dmatch, valid, wrsel;
  bvec<SZ> wridx(Log2(~valid));
  wrsel = Decoder(wridx);
  full = AndN(valid);
  node dinmap;

  for (unsigned i = 0; i < 1<<SZ; ++i) {
    node write(wr && (!dinmap && !full && wrsel[i] || dinmap && dmatch[i]));
    node rvalid, svalid;
    bvec<K> key(Wreg(write, dkey));
    valid[i] = RSReg(rvalid, svalid);
    match[i] = valid[i] && qkey == key;
    dmatch[i] = valid[i] && dkey == key;
    svalid = write;
    rvalid = erase && dmatch[i];
    b.connect(Wreg(write, d), match[i]);
  }

  dinmap = OrN(dmatch);
  inmap = OrN(match);

  TAP(full);
  TAP(valid);
  TAP(match);
  TAP(dmatch);
  TAP(b);
  TAP(dkey);
  TAP(qkey);
  TAP(d);
  TAP(wrsel);
  TAP(inmap);
  TAP(dinmap);
  TAP(wr);

  return b;
}

template <unsigned SZ, unsigned N>
  chdl::node chdl::Set(chdl::node full, chdl::bvec<N> qa, chdl::bvec<N> da,
                       chdl::node insert, chdl::node erase)
{
  using namespace chdl;

  node inmap;
  bvec<0> d;
  Map<SZ, N, 0>(inmap, full, qa, da, d, insert, erase);

  return inmap;
}

#endif
