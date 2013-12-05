#include <map>
#include <vector>

#include <chdl/chdl.h>
#include <chdl/cassign.h>

namespace chdl {
  template <unsigned M, unsigned N, unsigned MR, unsigned R, typename T>
    vec<R, bvec<N>> InitSyncRam(node            &stall,
                                vec<R, bvec<M>> qa,
                                bvec<N>         d,
                                bvec<M>         da,
                                node            w,
                                std::vector<T>  init);

  template <unsigned M, unsigned N, unsigned MR, unsigned R, typename T>
    bvec<N> InitSyncRam(node           &stall,
                        bvec<M>        qa,
                        bvec<N>        d,
                        bvec<M>        da,
                        node           w,
                        std::vector<T> init);
};

template <unsigned M, unsigned N, unsigned MR, unsigned R, typename T>
  chdl::vec<R, chdl::bvec<N>>
    chdl::InitSyncRam(chdl::node                  &stallOut,
                      chdl::vec<R, chdl::bvec<M>> qa,
                      chdl::bvec<N>               dIn,
                      chdl::bvec<M>               daIn,
                      chdl::node                  w,
                      std::vector<T>              init)
{
  using namespace chdl;
  using namespace std;

  bvec<MR> next_init_ctr, init_ctr(Reg(next_init_ctr));
  Cassign(next_init_ctr)
    .IF(init_ctr == ~Lit<MR>(0), ~Lit<MR>(0))
    .ELSE(init_ctr + Lit<MR>(1));

  node initPhase(!Reg(init_ctr == ~Lit<MR>(0)));
  stallOut = initPhase;
  bvec<N> d(Mux(initPhase, dIn, LLRom<MR, N>(init_ctr, init)));
  bvec<M> da(Mux(initPhase, daIn, Zext<M>(init_ctr)));

  return Syncmem(qa, d, da, w || initPhase);
}

template <unsigned M, unsigned N, unsigned MR, unsigned R, typename T>
  chdl::bvec<N> chdl::InitSyncRam(chdl::node     &stallOut,
                                  chdl::bvec<M>  qa,
                                  chdl::bvec<N>  d,
                                  chdl::bvec<M>  da,
                                  chdl::node     w,
                                  std::vector<T> init)
{
  using namespace chdl;
  using namespace std;

  vec<1, bvec<N>> qav(qa);

  return InitSyncRam(qav, da, d, w, init)[0];
}
