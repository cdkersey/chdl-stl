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

bool response_eaten(0), resp_valid(0);
map<cycle_t, function<void()> > eq;
static void advance_eq(cycle_t cyc) {
  if (response_eaten && eq.size() > 0) { eq.erase(eq.begin()); resp_valid = 0; }
  if (eq.size() > 0 && eq.begin()->first <= cyc) {
    eq.begin()->second();
  }
}

void clear_eq() { eq.clear(); }

CHDL_REGISTER_RESET(clear_eq);

template<typename T> void eq_sched(cycle_t t, T &f) {
  if (eq.count(t) == 0) {
    eq[t] = f;
  } else {
    cout << "Warning: event lost. Tried to schedule over existing event.";
  }
}

bool can_sched(cycle_t t) { return !eq.count(t); }

template <unsigned SZ, unsigned T,
          unsigned B, unsigned N, unsigned A, unsigned I>
  class random_delay_mem : public delete_on_reset
{
  public:
  random_delay_mem(mem_resp<B, N, I> &resp, mem_req<B, N, A, I> &req) {
    for (unsigned long i = 0; i < (1ull<<SZ); ++i)
      for (unsigned j = 0; j < N; ++j)
        contents[i][j] = 0;

    _(req, "ready") = _(resp, "ready");

    Egress(resp_ready, _(resp, "ready"));
    Egress(req_wr, _(_(req, "contents"), "wr"));
    for (unsigned i = 0; i < N; ++i) {
      Egress(req_mask[i], _(_(req, "contents"), "mask")[i]);
      EgressInt(req_d[i], _(_(req, "contents"), "data")[i]);
      _(_(resp, "contents"), "data")[i] = IngressInt<B>(resp_q[i]);
    }
    _(_(resp, "contents"), "wr") = resp_wr;
    _(resp, "valid") = Ingress(resp_valid);
    EgressInt(req_addr, _(_(req, "contents"), "addr"));
    EgressInt(req_id, _(_(req, "contents"), "id"));
    _(_(resp, "contents"), "id") = IngressInt<I>(resp_id);

    EgressFunc([this](bool x){
      if (x) {
        do_req(req_wr);
      }
    }, _(req, "valid") && _(req, "ready"));

    node eaten(_(resp, "ready") && _(resp, "valid")); TAP(eaten);
    Egress(response_eaten, _(resp, "ready") && _(resp, "valid"));
  }

  bool req_mask[N], req_wr, resp_wr, resp_ready;
  unsigned long req_addr;
  unsigned req_id, resp_id;
  unsigned long req_d[N], resp_q[N];
  unsigned long contents[1<<SZ][N];

  struct respval {
    bool wr;
    unsigned long q[N];
    unsigned id;
  };

  cycle_t resp_time() { return sim_time() + (rand() % T) + 1; }

  void do_req(bool wr) {
    respval v;

    if (wr) {
      for (unsigned i = 0; i < N; ++i) {
        if (req_mask[i]) contents[req_addr][i] = req_d[i];
      }
      v.wr = true;
    } else {
      // cout << "Reading at " << req_addr << ":\n";
      for (unsigned i = 0; i < N; ++i) {
        v.q[i] = contents[req_addr][i];
      }
      v.wr = false;
    }
    v.id = req_id;

    cycle_t t(resp_time());
    while (!can_sched(t)) ++t;

    function<void()> action = [v, this]()
    {
      resp_valid = true;
      resp_id = v.id;
      resp_wr = v.wr;
      if (!v.wr)
        for (unsigned i = 0; i < N; ++i)
          resp_q[i] = v.q[i];
    };

    eq_sched(t, action);
  }
};


template<unsigned SZ, unsigned T,
         unsigned B, unsigned N, unsigned A, unsigned I>
  void RandomDelayMem(mem_resp<B, N, I> &resp, mem_req<B, N, A, I> &req)
{
  new random_delay_mem<SZ,T,B,N,A,I>(resp, req);
}

template <unsigned B, unsigned N, unsigned A, unsigned I>
  bool test_Memreq(void (*MemUnit)(mem_resp<B, N, I>&, mem_req<B, N, A, I>&))
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
    } else if (!holding) {
      readv = writev = 0;
    }

    print_time(vcd);
    print_taps(vcd);

    for (auto &t : tickables()[0]) t->pre_tick(0);
    for (auto &t : tickables()[0]) t->tick(0);
    advance_eq(sim_time());
    for (auto &t : tickables()[0]) t->tock(0);
    for (auto &t : tickables()[0]) t->post_tock(0);
    now[0]++;

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
  if (!test_Memreq<8, 4, 7, 6>(RandomDelayMem<7, 5, 8, 4, 7, 6>)) return 1;
  reset();

  if (!test_Memreq<8, 8, 6, 6>(RandomDelayMem<6, 10, 8, 8, 6, 6>)) return 1;
  reset();

  if (!test_Memreq<32, 16, 5, 6>(RandomDelayMem<5, 5, 32, 16, 5, 6>)) return 1;
  reset();

  if (!test_Memreq<16, 16, 4, 6>(RandomDelayMem<4, 8, 16, 16, 4, 6>)) return 1;
  reset();

  if (!test_Memreq<8, 4, 7, 6>(Scratchpad<7, 8, 4, 7, 6>)) return 1;
  reset();

  if (!test_Memreq<8, 8, 6, 6>(Scratchpad<6, 8, 8, 6, 6>)) return 1;
  reset();

  if (!test_Memreq<32, 16, 5, 6>(Scratchpad<5, 32, 16, 5, 6>)) return 1;
  reset();

  if (!test_Memreq<16, 16, 4, 6>(Scratchpad<4, 16, 16, 4, 6>)) return 1;

  return 0;
}
