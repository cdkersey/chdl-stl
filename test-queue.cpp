#include <cstdlib>
#include <iostream>
#include <queue>

#include "queue.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

// #define DEBUG

template <unsigned N, unsigned SZ> bool test_Queue(bool sync) {
  srand(0);

  node push, pop;
  bvec<SZ+1> sz;
  bvec<N> qin, qout;
  
  if (sync)
    qout = SyncQueue<SZ>(qin, push, pop, sz);
  else
    qout = Queue<SZ>(qin, push, pop, sz);

  bool pushv, popv;
  unsigned szv, qinv, qoutv;

  push = Ingress(pushv);
  pop = Ingress(popv);
  qin = IngressInt<N>(qinv);

  EgressInt(qoutv, qout);
  EgressInt(szv, sz);

  optimize();

  unsigned long max_length(1<<SZ), prev_front(0);
  queue<int> comp_queue;

  for (unsigned i = 0; i < 4000; ++i) {
    qinv = i % (1ul<<N);
    pushv = !(rand() % 4);
    popv = !(rand() % 8);

    #ifdef DEBUG
    cout << sim_time() << ": qout = " << qoutv << ", sz = " << szv;
    if (comp_queue.size() > 0)
      cout << ", cqo = " << prev_front;
    cout << ", cqsz = " << comp_queue.size() << endl;
    #endif

    if (comp_queue.size() != szv) return false;
    if (comp_queue.size() > 0 && prev_front != qoutv) return false;

    bool full(szv == max_length), empty(szv == 0);

    if (sync) {
      if (comp_queue.size() != 0) prev_front = comp_queue.front();
      else prev_front = 0;
    }

    if (pushv && (!full || popv)) {
      #ifdef DEBUG
      cout << "PUSH " << qinv << endl;
      #endif
      comp_queue.push(qinv);
    }

    if (popv && (!empty || pushv)) {
      #ifdef DEBUG
      cout << "POP" << endl;
      #endif
      comp_queue.pop();
    }
 
    if (!sync) {
      if (comp_queue.size() != 0) prev_front = comp_queue.front();
      else prev_front = 0;
    }

    advance();
  }

  return true;
}

int main(int argc, char** argv) {

  if (!test_Queue<8, 8>(false)) return 1;
  reset();
  if (!test_Queue<8, 8>(true)) return 1;
  reset();
  if (!test_Queue<9, 4>(false)) return 1;
  reset();
  if (!test_Queue<9, 4>(true)) return 1;

  return 0;
}
