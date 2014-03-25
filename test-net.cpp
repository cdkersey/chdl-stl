#include <cstdlib>
#include <iostream>
#include <typeinfo>

#include <chdl/chdl.h>
#include <chdl/cassign.h>
#include <chdl/egress.h>
#include <chdl/ingress.h>

#include "net.h"

#define DEBUG

using namespace chdl;
using namespace std;

void test_net() {
  bool valid_val, ready_val;
  unsigned payload_val, addr_val;

  bool routed_valid_val[16], routed_ready_val[16],
       output_ready_val, output_valid_val;
  unsigned routed_addr_val[16], routed_payload_val[16],
           output_payload_val, output_address_val;

  typedef bvec<8> payload_t;
  typedef flit<routable<4, payload_t> > flit_t;

  //
  //                               +-------+
  //                               | Check |
  // +--------+                    +-------+
  // | Random |   +--------+           ^  +---------+   +-----+ 
  // | Packet |-->| Router |====(x16)==*=>| Arbiter |-->| Buf |--//
  // |  Gen   |   +--------+              +---------+   +-----+
  // +--------+    
  //

  flit_t src, arbitrated, out;
  vec<16, flit_t> routed;

  Router(routed, RouteAddrBits<16, 0, 4, payload_t>, src);
  Arbiter(arbitrated, ArbRR<16>, routed);
  Buffer<4>(out, arbitrated);

  // Set up the inputs and outputs for the simulation.
  _(src, "valid") = Ingress(valid_val);
  _(_(src, "contents"), "address") = IngressInt<4>(addr_val);
  _(_(src, "contents"), "contents") = IngressInt<8>(payload_val);
  _(out, "ready") = Ingress(output_ready_val);

  for (unsigned i = 0; i < 16; ++i) {
    Egress(routed_valid_val[i], _(routed[i], "valid"));
    Egress(routed_ready_val[i], _(routed[i], "ready"));
    EgressInt(routed_addr_val[i], _(_(routed[i], "contents"), "address"));
    EgressInt(routed_payload_val[i], _(_(routed[i], "contents"), "contents"));
  }

  EgressInt(output_payload_val, _(_(out, "contents"), "contents"));
  EgressInt(output_address_val, _(_(out, "contents"), "address"));
  Egress(output_valid_val, _(out, "valid"));
  Egress(ready_val, _(src, "ready"));

  if (cycdet()) abort();
  optimize();

  map<unsigned, unsigned> in_flight; // Payload => address
  unsigned id = 0;
  for (unsigned cyc = 0; cyc < 1000; ++cyc) {
    if (cyc < 900 && rand()%2) {
      valid_val = 1;
      payload_val = id++ % 256;
      addr_val = rand()%16;
      in_flight[payload_val] = addr_val;
    } else {
      valid_val = 0;
    }

    output_ready_val = rand()%2;

    advance();

    if (valid_val && !ready_val) {
      --id;
      in_flight.erase(id);
    }

    for (unsigned i = 0; i < 16; ++i) {
      if (routed_valid_val[i] && routed_ready_val[i]) {
        #ifdef DEBUG
        cout << "Found packet " << routed_payload_val[i] << " on " << i << endl;
        #endif

        if (in_flight[routed_payload_val[i]] != i || routed_addr_val[i] != i) {
          cout << "Address mismatch." << endl;
          abort();
        }
      }
    }

    if (output_valid_val && output_ready_val) {
      #ifdef DEBUG
      cout << "Got output packet: " << output_payload_val << ", "
           << output_address_val << endl;
      #endif
      in_flight.erase(output_payload_val);
    }
  }

  if (!in_flight.empty()) {
    cout << in_flight.size() << " packets remain in flight." << endl;
    for (auto x : in_flight)
      cout << "  " << x.first << ", " << x.second << endl;
    abort();
  }
}

int main(int argc, char **argv) {
  srand(0);

  test_net();

  return 0;
}
