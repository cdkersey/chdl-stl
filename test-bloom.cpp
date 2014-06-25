#include <cstdlib>
#include <iostream>
#include <stack>

#include "bloom.h"

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>
#include <chdl/reset.h>

using namespace std;
using namespace chdl;

// #define DEBUG
// #define VCD

template <unsigned SZ, unsigned H, unsigned N, unsigned SEED = 0x5eed>
  bool test_BloomFilter()
{
  bool wr_val, clear_val, hit_val;
  unsigned long insert_val, lookup_val;

  node wr(Ingress(wr_val)), clear(Ingress(clear_val));

  bvec<N> insert(IngressInt<N>(insert_val)), lookup(IngressInt<N>(lookup_val));
  
  node hit = BloomFilter<SZ, H>(Hash<N>(lookup, ~SEED),
                                Hash<N>(insert, ~SEED), wr,
                                clear, SEED);

  Egress(hit_val, hit);

  TAP(hit); TAP(wr); TAP(clear); TAP(lookup); TAP(insert);

  optimize();
  
  mt19937 rng(SEED);
  uniform_int_distribution<unsigned long> id(0, 1000);
  bernoulli_distribution bd(0.01);

  #ifdef VCD
  ofstream vcd("test-bloom.vcd");
  #endif

  set<unsigned long> local;
  unsigned false_positives = 0;

  #ifdef VCD
  print_vcd_header(vcd);
  print_time(vcd);
  #endif

  for (unsigned i = 0; i < 10000; ++i) {
    clear_val = false;
    wr_val = bd(rng);
    insert_val = id(rng);
    lookup_val = id(rng);

    #ifdef VCD
    print_taps(vcd);
    #endif

    advance();

    #ifdef VCD
    print_time(vcd);
    #endif

    #ifdef DEBUG
    if (wr_val) cout << "Insert " << insert_val << endl;
    if (hit_val && !local.count(lookup_val)) cout << sim_time() << ": False positive on value " << lookup_val << endl;
    if (!hit_val && local.count(lookup_val)) cout << sim_time() << ": False negative." << endl;
    #endif

    if (clear_val) local.clear();
    if (hit_val && !local.count(lookup_val)) false_positives++;
    if (!hit_val && local.count(lookup_val)) {
      cout << "ERROR: false negative on value " << lookup_val << "." << endl;
      return false;
    }
    if (wr_val) local.insert(insert_val);
  }
  cout << false_positives << " false positives on <"
       << SZ << ", " << H << ">.\n";

  return true;
}

int main(int argc, char** argv) {
  if (!test_BloomFilter<6, 1, 10>()) return 1;
  reset();
  if (!test_BloomFilter<7, 1, 20>()) return 1;
  reset();
  if (!test_BloomFilter<7, 2, 17>()) return 1;
  reset();
  if (!test_BloomFilter<7, 3, 14>()) return 1;
  reset();
  if (!test_BloomFilter<8, 2, 19>()) return 1;
  reset();

  return 0;
}
