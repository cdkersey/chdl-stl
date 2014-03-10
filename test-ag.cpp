#include "ag.h"

#include <chdl/chdl.h>
#include <chdl/ingress.h>
#include <chdl/egress.h>

#include <typeinfo>
#include <cstdlib>

// #define DEBUG

using namespace std;
using namespace chdl;

template <typename T> using flit =
  ag<STP("a"), node,
  ag<STP("b"), node,
  ag<STP("c"), T> > >;

template <unsigned N, typename T> using routable =
  ag<STP("a"), bvec<N>,
  ag<STP("b"), T> >;

typedef flit<vec<8, node> > flit_t;
typedef routable<1, flit_t> packet_t;

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
  int in_audio_val[4][2], out_audio_val[2];
  for (unsigned i = 0; i < 4; ++i) {
    video_t vid(Lookup<STP("video")>(av_t(sources[i])));
    for (unsigned j = 0; j < 3; ++j) {
      bvec<8> v(Lookup<STP("value")>(vid)[j]);
      v = IngressInt<8>(vid_val[i][j]);
    }
    _(_(av_t(sources[i]),"audio"),"lpcm")=IngressInt<16>(in_audio_val[i][0]);
    _(_(av_t(sources[i]),"audio"),"rpcm")=IngressInt<16>(in_audio_val[i][1]);
  }

  dest = Mux(sel, sources);
  
  video_t out_vid(_(dest, "video"));
  for (unsigned i = 0; i < 3; ++i)
    EgressInt(out_vid_val[i],
              Lookup<STP("value")>(out_vid)[i]);
  EgressInt(out_audio_val[0], _(_(dest, "audio"), "lpcm"));
  EgressInt(out_audio_val[1], _(_(dest, "audio"), "rpcm"));

  packet_t p;
  flit_t x(_(p, "b"));
  _(x, "c");
  //_(_(p, "b"), "c")[3];
  cout << "fun: " << typeid(match_type<STP("c"), flit_t>::type).name() << endl;

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
