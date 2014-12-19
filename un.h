#ifndef CHDL_UN_H
#define CHDL_UN_H

#include <string>
#include <sstream>
#include <type_traits>
#include <typeinfo>

#include <chdl/chdl.h>
#include "ag.h"

namespace chdl {

template <typename T> static constexpr T max(T a, T b) { return a > b ? a : b; }
template <typename T> static constexpr T min(T a, T b) { return a < b ? a : b; }

struct un_endtype {
  void tap(std::string prefix, bool output = false) const {}
  un_endtype &operator=(const un_endtype &r) { return *this; }
  operator chdl::bvec<0>() const { return chdl::bvec<0>(); }
};

template <> struct sz<un_endtype> { static const unsigned value = 0; };

template <typename NAME, typename T, typename NEXT = un_endtype>
  struct un
{
  void postconstruct(un_endtype&) {} // Do nothing
  template <typename NEXT_> void postconstruct(NEXT_&) {
    const unsigned MINSZ(min(sz<NEXT>::value, sz<un>::value));
    contents[range<0, MINSZ-1>()] = next.contents[range<0, MINSZ-1>()];
  }

  un() { postconstruct(next); }

  un(const un &r): contents(r.contents), next(r.next) { postconstruct(next); }

  un(const T &r) {
    bvec<sz<un>::value> bv(*this);
    bv[range<0, sz<T>::value - 1>()] = Flatten(r);
    postconstruct();
  }

  un &operator=(un &r) { contents = r.contents; next = r.next; }

  operator bvec<sz<un>::value>() const { return contents; }

  template <typename Q> static constexpr bool MATCH() {
    return Q::template MATCH<NAME>();
  }

  un &operator=(const un &r) {
    contents = r.contents;
    next = r.next;
    return *this;
  }

  void tap(std::string prefix = "", bool output = false) const;

  bvec<sz<un>::value> contents;
  NEXT next;
  typedef NEXT next_t;
  typedef NAME name_t;
  typedef T content_t;
};

  template <typename NAME, typename T, typename NEXT>
    struct match_type<NAME, un<NAME, T, NEXT> >
  { typedef T type; };

  template <typename N1, typename N2, typename T, typename NEXT>
    struct match_type<N1, un<N2, T, NEXT> >
  { typedef typename match_type<N1, NEXT>::type type; };

  // Even though we do implicit (flattening) conversions, this is provided for
  // compatibility. (TODO: and may cause some bugs)
  template <typename NAME, typename T, typename NEXT>
    bvec<sz<un<NAME, T, NEXT> >::value> Flatten(const un<NAME, T, NEXT> &a)
  {
    return bvec<sz<un<NAME, T, NEXT> >::value>(a);
  }

  template <typename NAME, typename T, typename NEXT>
    struct sz<un<NAME, T, NEXT> > {
      static const unsigned value = max(sz<T>::value, sz<NEXT>::value);
    };

  template <typename NAME, typename T, typename NEXT>
    void tap(std::string prefix, const un<NAME, T, NEXT> &a, bool output=false)
  { a.tap(prefix, output); }

  template<typename NAME, typename T, typename NEXT>
    void un<NAME, T, NEXT>::tap(std::string prefix, bool output) const
  {
    T x;
    Flatten(x) = contents[range<0,sz<T>::value-1>()];
    chdl::tap(prefix + '_' + NAME::str(), x, output);
    next.tap(prefix, output);
  }

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    struct lookup_un
  {
  lookup_un(un<NAME, T, NEXT> &a):
    value(lookup_un<
      QNAME,
      typename un<NAME, T, NEXT>::next_t::name_t,
      typename un<NAME, T, NEXT>::next_t::content_t,
      typename un<NAME, T, NEXT>::next_t::next_t
    >(a.next).value[range<0, sz<typename match_type<QNAME, ag<NAME, T, NEXT> >::type>::value - 1>()]) {}

    bvec<sz<typename match_type<QNAME, ag<NAME, T, NEXT> >::type>::value> value;
  };

  template <typename NAME, typename T, typename NEXT>
    struct lookup_un<NAME, NAME, T, NEXT>
  {
    lookup_un(un<NAME, T, NEXT> &a): value(a.contents) {}
    bvec<sz<un<NAME, T, NEXT> >::value> value;
  };

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    typename match_type<QNAME, un<NAME, T, NEXT> >::type
      Lookup(un<NAME, T, NEXT> a)
  {
    typedef typename match_type<QNAME, un<NAME, T, NEXT> >::type x_t;
    x_t x;
    Flatten(x) =
      lookup_un<QNAME, NAME, T, NEXT>(a).value[range<0,sz<x_t>::value-1>()];
    return x;
  }

#define UN_1(t1, n1) un<STRTYPE(n1), t1>

#define UN_2(t1, n1, t2, n2) un<STRTYPE(n1), t1, un<STRTYPE(n2), t2> >

#define UN_3(t1, n1, t2, n2, t3, n3) \
  un<STRTYPE(n1), t1, un<STRTYPE(n2), t2, un<STRTYPE(n3), t3> > >

#define UN_4(t1, n1, t2, n2, t3, n3, t4, n4) \
  un<STRTYPE(n1), t1, un<STRTYPE(n2), t2, un<STRTYPE(n3), t3, \
  un<STRTYPE(n4), t4> > > >

#define UN_5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
  un<STRTYPE(n1), t1, un<STRTYPE(n2), t2, un<STRTYPE(n3), t3, \
  un<STRTYPE(n4), t4, un<STRTYPE(n5), t5> > > >

}

#endif
