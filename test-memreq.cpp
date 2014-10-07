#include <cstdlib>
#include <iostream>
#include <stack>
#include <map>
#include <set>
#include <functional>

#include "memreq.h"
#include "lfsr.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

struct delete_on_reset {
  delete_on_reset() {
    cout << "Constructing a d.o.r.\n";
    dors.insert(this);
  }
  virtual ~delete_on_reset() {
    cout << "Deleting a d.o.r.\n";
    dors.erase(this);
  }

  static set<delete_on_reset*> dors;
};

set<delete_on_reset*> delete_on_reset::dors;

void reset_delete_on_reset() {
  while(!delete_on_reset::dors.empty()) delete *delete_on_reset::dors.begin();
}

CHDL_REGISTER_RESET(reset_delete_on_reset);

map<cycle_t, set<function<void()> > > eq;
static void advance_eq(cycle_t cyc) {
  if (eq.count(cyc) ) {
    for (auto &f : eq[cyc]) f();
    eq.erase(cyc);
  }
}

void clear_eq() { eq.clear(); }

CHDL_REGISTER_RESET(clear_eq);

template <unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
  class random_delay_mem : public delete_on_reset
{
  public:
  random_delay_mem(mem_resp<B, N, I> &resp, mem_req<B, N, A, I> &req) {
  }

  bool req_valid, req_wr, req_mask[N];
  unsigned long req_addr;
  unsigned req_id;
  unsigned long req_d[N];
  unsigned long contents[1<<SZ][N];

  struct respval {
    bool wr;
    unsigned long q[N];
    unsigned id;

    random_delay_mem *mem;
  };
};


template<unsigned SZ, unsigned B, unsigned N, unsigned A, unsigned I>
  void RandomDelayMem(mem_resp<B, N, I> &resp, mem_req<B, N, A, I> &req)
{
  new random_delay_mem<SZ,B,N,A,I>(resp, req);
}

template <unsigned B, unsigned N, unsigned A, unsigned I, typename T>
  bool test_Memreq(const T &MemUnit)
{
  srand(0);

  node write, read, ready;
  bvec<N> mask;
  vec<N, bvec<B> > d, q;
  bvec<I> req_id, resp_id;

  bool resp_validv, req_readyv, resp_readyv, writev, readv, readyv, maskv[N];
  unsigned long qv[N], dv[N], req_idv, resp_idv, addrv;

  write = Ingress(writev);
  read = Ingress(readv);
  ready = Ingress(readyv);
  req_id = IngressInt<I>(req_idv);
  EgressInt(resp_idv, resp_id);
  for (unsigned i = 0; i < N; ++i) {
    d[i] = IngressInt<B>(dv[i]);
    mask[i] = Ingress(maskv[i]);
    EgressInt(qv[i], q[i]);
  }

  mem_req<B, N, A, I> req;
  mem_resp<B, N, I> resp;
  _(req, "valid") = (read || write);
  _(_(req, "contents"), "wr") = write;
  _(_(req, "contents"), "mask") = mask;
  _(_(req, "contents"), "addr") = IngressInt<A>(addrv);
  _(_(req, "contents"), "data") = d;
  _(_(req, "contents"), "id") = req_id;
  _(_(req, "contents"), "llsc") = Lit(0);
  _(resp, "ready") = !AndN(Lfsr<2, 31, 3, 1234>());

  Egress(req_readyv, _(req, "ready"));
  Egress(resp_validv, _(resp, "valid"));
  _(resp, "valid") = _(resp, "valid");
  q = _(_(resp, "contents"), "data"); 
  resp_id = _(_(resp, "contents"), "id"); 

  Egress(resp_readyv, _(resp, "ready"));

  MemUnit(resp, req);

  TAP(req);
  TAP(resp);

  optimize();
  if (cycdet()) { cout << "Cycle detected." << endl; abort(); }

  ofstream vcd("memreq.vcd");
  print_vcd_header(vcd);

  struct req_record {
    unsigned long val[N];
  };

  set<int> outstanding_wr;
  map<int, req_record> outstanding_rd;
  map<unsigned long, req_record > cur_mem;

  bool holding(false);
  int cur_id(0);

  for (unsigned i = 0; i < 10000; ++i) {
    if (!holding &&
        !outstanding_rd.count(cur_id) &&
        !outstanding_wr.count(cur_id))
    {
      writev = ((rand() & 7) == 0);
      if (writev) {
        readv = 0;
        for (unsigned j = 0; j < N; ++j) {
          maskv[j] = (rand() & 1);
          dv[j] = rand() & ((1ull<<B) - 1);
        }
      } else {
        readv = (rand() & 1);
      }

      if (readv || writev) {
        addrv = rand() & ((1ull<<A)-1);
        if (writev) {
          outstanding_wr.insert(cur_id);
          for (unsigned j = 0; j < N; ++j)
            if (maskv[j]) cur_mem[addrv].val[j] = dv[j];
        } else {
          for (unsigned j = 0; j < N; ++j)
            outstanding_rd[cur_id].val[j] = cur_mem[addrv].val[j];
        }
        req_idv = cur_id++;
        if (cur_id == (1ull<<I)) cur_id = 0;
      }
    } else {
      readv = writev = 0;
    }

    print_time(vcd);
    print_taps(vcd);
    advance_eq(sim_time());
    advance();

    holding = !req_readyv;
    if (resp_readyv && resp_validv) {
      if (!outstanding_rd.count(resp_idv) && !outstanding_wr.count(resp_idv)) {
        cout << "Response ID other than that of outstanding request in "
             << i << endl;
        return false;
      }

      bool wr(outstanding_wr.count(resp_idv));
      if (wr) outstanding_wr.erase(resp_idv);

      if (!wr) {
        for (unsigned j = 0; j < N; ++j) {
          if (outstanding_rd[resp_idv].val[j] != qv[j]) {
            cout << "Response read value mismatch in " << i << " req "
                 << resp_idv << " byte " << j << " got " << qv[j]
                 << ", expected " << outstanding_rd[resp_idv].val[j] << endl;
            return false;
          }
        }
        outstanding_rd.erase(resp_idv);
      }
    }
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Memreq<8, 4, 7, 6>(Scratchpad<7, 8, 4, 7, 6>)) return 1;

  reset();

  if (!test_Memreq<8, 8, 6, 6>(Scratchpad<6, 8, 8, 6, 6>)) return 1;

  reset();

  if (!test_Memreq<32, 16, 5, 6>(Scratchpad<5, 32, 16, 5, 6>)) return 1;

  reset();

  if (!test_Memreq<16, 16, 4, 6>(Scratchpad<4, 16, 16, 4, 6>)) return 1;

  return 0;
}
