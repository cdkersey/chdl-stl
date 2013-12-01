#include <cstdlib>
#include <iostream>
#include <stack>

#include "map.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

//#define DEBUG

template <unsigned K, unsigned V, unsigned SZ> bool test_Map() {
  srand(0);

  node write, erase, full, found;
  bvec<K> dk, qk;
  bvec<V> dv, qv;

  bool writev, erasev, fullv, foundv;
  unsigned dkv, qkv, dvv, qvv;

  write = Ingress(writev);
  erase = Ingress(erasev);
  qk = IngressInt<K>(qkv);
  dk = IngressInt<K>(dkv);
  dv = IngressInt<V>(dvv);

  EgressInt(qvv, qv);
  Egress(foundv, found);
  Egress(erasev, erase);

  qv = Map<SZ, K, V>(full, found, qk, dk, dv, write, erase);

  optimize();
  if (cycdet()) { cout << "Cycle detected." << endl; abort(); }

  ofstream vcd("map.vcd");
  print_vcd_header(vcd);

  map<int, int> comp_map;

  for (unsigned i = 0; i < 4000; ++i) {
    dvv = i % (1ul<<V);
    dkv = rand() % (1ul<<K);
    qkv = rand() % (1ul<<K);
    erasev = !(rand() % 5);
    writev = !erasev && !(rand() % 3);

    #ifdef DEBUG
    cout << sim_time() << ": qk = " << qkv << ", qv = " << qkv;
    if (writev) cout << ", WRITE";
    if (erasev) cout << ", ERASE";
    cout << ", dk = " << dkv << ", dv = " << dvv << endl;
    #endif

    print_time(vcd);
    print_taps(vcd);
    advance();

    if (comp_map.find(qkv) != comp_map.end()) {
      if (qvv != comp_map[qkv]) {
        cout << "ERROR: comp_map[" << qkv << "] should be " << comp_map[qkv]
             << "; is " << qvv << endl;
        abort();
      }
    }

    if (writev && (comp_map.find(dkv)!=comp_map.end() || comp_map.size()!=SZ))
      comp_map[dkv] = dvv;
    if (erasev && comp_map.find(dkv) != comp_map.end())
      comp_map.erase(dkv);
  }

  return true;
}

int main(int argc, char** argv) {
  if (!test_Map<6, 16, 3>()) return 1;

  reset();

  if (!test_Map<8, 32, 4>()) return 1;

  reset();

  if (!test_Map<5, 0, 3>()) return 1;

  return 0;
}
