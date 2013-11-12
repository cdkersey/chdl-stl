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

template <unsigned N, unsigned SZ> bool test_Queue() {
  srand(0);

  node push, pop;
  bvec<SZ+1> sz;
  bvec<N> qin, qout(Queue<SZ>(qin, push, pop, sz));

  bool pushv, popv;
  unsigned szv, qinv, qoutv;

  push = Ingress(pushv);
  pop = Ingress(popv);
  qin = IngressInt<N>(qinv);

  EgressInt(qoutv, qout);
  EgressInt(szv, sz);

  optimize();

  unsigned long max_length(1<<SZ);
  queue<int> comp_queue;

  for (unsigned i = 0; i < 4000; ++i) {
    qinv = i % (1ul<<N);
    pushv = !(rand() % 4);
    popv = !(rand() % 8);

    #ifdef DEBUG
    cout << sim_time() << ": qout = " << qoutv << ", sz = " << szv;
    if (comp_queue.size() > 0)
      cout << ", cqo = " << comp_queue.front();
    cout << ", cqsz = " << comp_queue.size() << endl;
    #endif

    if (comp_queue.size() != szv) return false;
    if (comp_queue.size() > 0 && comp_queue.front() != qoutv) return false;

    bool full(szv == max_length), empty(szv == 0);

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

    advance();
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Queue<8, 8>()) return 1;

  reset();

  if (!test_Queue<9, 4>()) return 1;

  return 0;
}
