#include <cstdlib>
#include <iostream>
#include <stack>

#include "initmem.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

//#define DEBUG

template <unsigned M, unsigned N, unsigned SR> bool test_Initmem() {
  srand(0);

  vector<unsigned> initVals(1<<SR);
  map<unsigned, unsigned> contents;
  for (unsigned i = 0; i < 1<<SR; ++i)
    contents[i] = initVals[i] = i;//rand() % (1ull<<N);

  bool wv;
  unsigned dav, qa0v, qa1v, dv;

  node w(Ingress(wv)), stall;
  vec<2, bvec<M>> qa;
  bvec<M> da(IngressInt<M>(dav));
  bvec<N> d(IngressInt<N>(dv));
  vec<2, bvec<N>> q(InitSyncRam<M,N,SR>(stall, qa, d, da, w, initVals));
  
  qa[0] = IngressInt<M>(qa0v);
  qa[1] = IngressInt<M>(qa1v);

  bool stallv;
  unsigned q0v, q1v;
  Egress(stallv, stall);
  EgressInt(q0v, q[0]);
  EgressInt(q1v, q[1]);

  optimize();

  do advance(); while(stallv);

  qa0v = qa1v = 0;
  unsigned prev_qa0v(qa0v), prev_qa1v(qa1v), prev_dav, prev_dv;
  advance();

  #ifdef DEBUG
  cout << "qa[0],q[0],expected q[0],qa[1],q[1],expected q[1]" << endl;
  #endif

  bool prev_wv(false);
  for (unsigned i = 0; i < 100000; ++i) {
    qa0v = rand() % (1ull<<M);
    qa1v = (10000 - i)%(1ull<<M);
    dav = rand() % (1ull<<M);
    dv = rand() % (1ull<<N);
    wv = !(rand() % 16);

    advance();

    #ifdef DEBUG
    cout << prev_qa0v << ',' << q0v << ',' << contents[prev_qa0v] << ','
         << prev_qa1v << ',' << q1v << ',' << contents[prev_qa1v] << endl;
    #endif

    if (q0v != contents[prev_qa0v]) return false;
    if (q1v != contents[prev_qa1v]) return false;

    if (prev_wv) contents[prev_dav] = prev_dv;
    prev_wv = wv;
    prev_dav = dav;
    prev_dv = dv;
 
    prev_qa0v = qa0v;
    prev_qa1v = qa1v;
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Initmem<8, 32, 6>()) return 1;

  reset();

  if (!test_Initmem<16, 32, 8>()) return 1;

  reset();

  if (!test_Initmem<8, 8, 8>()) return 1;

  return 0;
}
