#ifndef CHDL_SORT_H
#define CHDL_SORT_H

#include <chdl/chdl.h>

namespace chdl {
  namespace sort_internal {
    template <typename T> void Compare(T &ao, T &bo, const T &a, const T &b) {
      node swap(b < a);

      ao = Mux(swap, a, b);
      bo = Mux(swap, b, a);
    }
  }

  // Base cases for merge.
  template <typename T> vec<0, T> Merge(const vec<0, T> &x) { return x; }
  template <typename T> vec<1, T> Merge(const vec<1, T> &x) { return x; }
  template <typename T> vec<2, T> Merge(const vec<2, T> &x) {
    vec<2, T> out;
    sort_internal::Compare(out[0], out[1], x[0], x[1]);
    return out;
  }

  // Odd/evern merge (Batcher's merge sort algorithm)
  template <unsigned N, typename T> vec<N, T> Merge(const vec<N, T> &x)
  {
    const unsigned E(N - N/2), O(N/2);

    vec<E, T> e, e_merged(Merge(e));
    vec<O, T> o, o_merged(Merge(o));

    for (unsigned i = 0; i < N; ++i) {
      if (i & 1) o[i/2] = x[i];
      else       e[i/2] = x[i];
    }

    vec<N, T> y, out;

    for (unsigned i = 0; i < N; ++i) {
      if (i & 1) y[i] = o_merged[i/2];
      else       y[i] = e_merged[i/2];
    }

    out[0] = y[0];
    out[N-1] = y[N-1];
    for (unsigned i = 1; i < N-2; i += 2)
      sort_internal::Compare(out[i], out[i + 1], y[i], y[i + 1]);

    return out;
  }

  template <typename T> vec<0, T> Sort(const vec<0, T> &x) { return x; }
  template <typename T> vec<1, T> Sort(const vec<1, T> &x) { return x; }

  template <unsigned N, typename T> vec<N, T> Sort(const vec<N, T> &x)
  {
    // Recursively sort halves of the array.
    vec<N/2, T> a(x[range<0, N/2-1>()]), a_sorted(Sort(a)),
                b(x[range<N/2, N-1>()]), b_sorted(Sort(b));

    // Merge the halves.
    return Merge(Cat(b_sorted, a_sorted));
  }
  
}

#endif
