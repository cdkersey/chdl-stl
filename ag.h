#ifndef CHDL_AG_H
#define CHDL_AG_H

#include <string>
#include <sstream>
#include <type_traits>

#include <chdl/chdl.h>

namespace chdl {

struct strtype_term {
  static constexpr char VAL() { return '\0'; }
  template <typename S2> static constexpr bool MATCH() {
    return VAL() == S2::VAL();
  }

  static std::string str() { return ""; }

  typedef strtype_term NEXT_t;
};

template <char C, typename NEXT> struct strtype {
  static constexpr char VAL() { return V; }
  template <typename S2> static constexpr bool MATCH() {
    return VAL() == S2::VAL() && (NEXT::template MATCH<typename S2::NEXT_t>());
  }

  static std::string str() { return C + NEXT_t::str(); }

  static const char V = C;
  typedef NEXT NEXT_t;
};

template <char C, typename T> struct append_char {
  typedef strtype<T::V, typename append_char<C, typename T::NEXT_t>::X > X;
};

template <char C> struct append_char<C, strtype_term> {
  typedef strtype<C, strtype_term> X;
};

template <> struct append_char<'\0', strtype_term> {
  typedef strtype_term X;
};

template <typename T> struct append_char<'\0', T> {
  typedef strtype<T::V, typename T::NEXT_t> X;
};

template <unsigned N> constexpr char _xt(const char (&s)[N], unsigned i) {
  return (i >= N) ? 0 : s[i];
}

#define STRTYPE(x) \
  chdl::append_char<chdl::_xt(x,31), chdl::append_char<chdl::_xt(x,30), \
  chdl::append_char<chdl::_xt(x,29), chdl::append_char<chdl::_xt(x,28), \
  chdl::append_char<chdl::_xt(x,27), chdl::append_char<chdl::_xt(x,26), \
  chdl::append_char<chdl::_xt(x,25), chdl::append_char<chdl::_xt(x,24), \
  chdl::append_char<chdl::_xt(x,23), chdl::append_char<chdl::_xt(x,22), \
  chdl::append_char<chdl::_xt(x,21), chdl::append_char<chdl::_xt(x,20), \
  chdl::append_char<chdl::_xt(x,19), chdl::append_char<chdl::_xt(x,18), \
  chdl::append_char<chdl::_xt(x,17), chdl::append_char<chdl::_xt(x,16), \
  chdl::append_char<chdl::_xt(x,15), chdl::append_char<chdl::_xt(x,14), \
  chdl::append_char<chdl::_xt(x,13), chdl::append_char<chdl::_xt(x,12), \
  chdl::append_char<chdl::_xt(x,11), chdl::append_char<chdl::_xt(x,10), \
  chdl::append_char<chdl::_xt(x, 9), chdl::append_char<chdl::_xt(x, 8), \
  chdl::append_char<chdl::_xt(x, 7), chdl::append_char<chdl::_xt(x, 6), \
  chdl::append_char<chdl::_xt(x, 5), chdl::append_char<chdl::_xt(x, 4), \
  chdl::append_char<chdl::_xt(x, 3), chdl::append_char<chdl::_xt(x, 2), \
  chdl::append_char<chdl::_xt(x, 1), chdl::append_char<chdl::_xt(x, 0), \
  chdl::strtype_term \
  >::X >::X >::X >::X >::X >::X >::X >::X \
  >::X >::X >::X >::X >::X >::X >::X >::X \
  >::X >::X >::X >::X >::X >::X >::X >::X \
  >::X >::X >::X >::X >::X >::X >::X >::X

#define STP(x) STRTYPE(x)

struct ag_endtype {
  void tap(std::string prefix) {}
  ag_endtype &operator=(const ag_endtype &r) { return *this; }
  operator chdl::bvec<0>() const { return chdl::bvec<0>(); }
};

template <> struct sz<ag_endtype> { static const unsigned value = 0; };

template <typename NAME, typename T, typename NEXT = ag_endtype>
  struct ag
{
  ag() {}

  ag(const ag &r): contents(r.contents), next(r.next) {}

  ag(const bvec<sz<T>::value + sz<NEXT>::value> &r)
  {
    bvec<sz<T>::value + sz<NEXT>::value> bv(*this);
    bv = r;
  }

  ag &operator=(ag &r) { contents = r.contents; next = r.next; }

  // Identical to bvec-based constructor.
  ag &operator=(const bvec<sz<T>::value + sz<NEXT>::value> &r)
  {
    bvec<sz<T>::value + sz<NEXT>::value> bv(*this);
    bv = r;
  }

  operator bvec<sz<T>::value + sz<NEXT>::value>() const {
    return Cat(bvec<sz<T>::value>(Flatten(contents)),
                     bvec<sz<NEXT>::value>(next));
  }

  template <typename Q> static constexpr bool MATCH() {
    return Q::template MATCH<NAME>();
  }

  ag &operator=(const ag &r) {
    contents = r.contents;
    next = r.next;
    return *this;
  }

  void tap(std::string prefix = "");

  T contents;
  NEXT next;
  typedef NEXT next_t;
  typedef T content_t;
};

  // Even though we do implicit (flattening) conversions, this is provided for
  // compatibility. (TODO: and may cause some bugs)
  template <typename NAME, typename T, typename NEXT>
    bvec<sz<ag<NAME, T, NEXT> >::value> Flatten(const ag<NAME, T, NEXT> &a)
  {
    return a;
  }

  template <typename NAME, typename T, typename NEXT>
    struct sz<ag<NAME, T, NEXT> > {
      static const unsigned value = sz<T>::value + sz<NEXT>::value;
    };

  template <typename NAME, typename T, typename NEXT>
    void tap(std::string prefix, ag<NAME, T, NEXT> &a)
  { a.tap(prefix); }

  template<typename NAME, typename T, typename NEXT>
    void ag<NAME, T, NEXT>::tap(std::string prefix)
  {
    chdl::tap(prefix + '_' + NAME::str(), contents);
    next.tap(prefix);
  }

template <typename A, typename B, bool X> struct try_assign {
  static void x(A &a, B& b) { a = b; }
};

template <typename A, typename B> struct try_assign<A, B, false> {
  static void x(A &a, B& b) {}
};

template <typename T, typename AG> T Extract(AG &a) {
  T r;

  try_assign<T, typename AG::content_t,
             std::is_assignable<T, typename AG::content_t>::value
  >::x(r, a.contents);

  return r;
}

template <typename NAME, typename T> T Lookup(const ag_endtype &x) {}

template <typename NAME, typename T, typename AG> T Lookup(AG a) {
  if (AG::template MATCH<NAME>()) return Extract<T>(a);
  else return Lookup<NAME, T>(a.next);
}

#ifndef CHDL_AG_DISABLE_UNDERSCORE
#define _(ag, type, name) Lookup<STRTYPE(name), type>(ag)
#endif

#define AG_1(t1, n1) ag<STRTYPE(n1), t1>

#define AG_2(t1, n1, t2, n2) ag<STRTYPE(n1), t1, ag<STRTYPE(n2), t2> >

#define AG_3(t1, n1, t2, n2, t3, n3) \
  ag<STRTYPE(n1), t1, ag<STRTYPE(n2), t2, ag<STRTYPE(n3), t3> > >

#define AG_4(t1, n1, t2, n2, t3, n3, t4, n4) \
  ag<STRTYPE(n1), t1, ag<STRTYPE(n2), t2, ag<STRTYPE(n3), t3, \
  ag<STRTYPE(n4), t4> > > >

#define AG_5(t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
  ag<STRTYPE(n1), t1, ag<STRTYPE(n2), t2, ag<STRTYPE(n3), t3, \
  ag<STRTYPE(n4), t4, ag<STRTYPE(n5), t5> > > >

}

#endif
