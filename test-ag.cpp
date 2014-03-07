#include "ag.h"

#include <chdl/chdl.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>

#include <cstdlib>

// #define DEBUG

using namespace std;
using namespace chdl;

// First define a complex aggregate type. Most of the testing here is just that
// this _compiles_ at all.
typedef ag<STP("value"), vec<3, bvec<8> >, // RGB intensity values
        ag<STP("hsync"), node,
        ag<STP("vsync"), node> > > video_t;

typedef AG_2(bvec<16>, "lpcm",          // Left channel
             bvec<16>, "rpcm") audio_t; // Right channel

typedef AG_2(video_t, "video",
             audio_t, "audio") av_t;

void test_ag() {
  int sel_val;
  vec<4, bvec<sz<av_t>::value> > sources;
  av_t dest;
  bvec<2> sel(IngressInt<2>(sel_val));

  int vid_val[4][3], out_vid_val[3];
  for (unsigned i = 0; i < 4; ++i) {
    video_t vid(Lookup<STP("video")>(av_t(sources[i])));
    for (unsigned j = 0; j < 3; ++j) {
      bvec<8> v(Lookup<STP("value")>(vid)[j]);
      v = IngressInt<8>(vid_val[i][j]);
    }
  }

  dest = Mux(sel, sources);
  
  video_t out_vid(_(dest, "video"));
  for (unsigned i = 0; i < 3; ++i)
    EgressInt(out_vid_val[i],
              Lookup<STP("value")>(out_vid)[i]);

  optimize();

  srand(0);

  for (unsigned cyc = 0; cyc < 1000; ++cyc) {
    // Set up inputs.
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = 0; j < 3; ++j)
        vid_val[i][j] = rand() & 0xff;

    sel_val = rand() & 0x3;

    advance();

    #ifdef DEBUG
    cout << cyc << ": " << sel_val << ", " << out_vid_val[0] << ", "
         << vid_val[0][0] << ", " << vid_val[1][0] << ", " << vid_val[2][0]
         << ", " << vid_val[3][0] << endl;
    #endif

    for (unsigned i = 0; i < 3; ++i) {
      if (out_vid_val[i] != vid_val[sel_val][i]) cout << "ERROR!\n";
    }
  }
}

int main(int argc, char** argv) {
  test_ag();

  return 0;
}
