#ifndef CHDL_AG_H
#define CHDL_AG_H

#include <string>
#include <sstream>
#include <type_traits>
#include <typeinfo>

#include <chdl/chdl.h>

namespace chdl {

// strtype: a way to represent strings as types
struct strtype_term { // Marks the end of a strtype
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

#ifndef CHDL_AG_DISABLE_STP
#define STP(x) STRTYPE(x)
#endif

struct ag_endtype {
  void tap(std::string prefix, bool output = false) const {}
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

  ag &operator=(ag &r) { contents = r.contents; next = r.next; return *this; }

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

  void tap(std::string prefix = "", bool output = false) const;

  T contents;
  NEXT next;
  typedef NEXT next_t;
  typedef NAME name_t;
  typedef T content_t;
};

  struct emptytype;

  template <typename NAME, typename AG> struct match_type {
    typedef emptytype type;
  };

  template <typename NAME, typename T, typename NEXT>
    struct match_type<NAME, ag<NAME, T, NEXT> >
  { typedef T type; };

  template <typename N1, typename N2, typename T, typename NEXT>
    struct match_type<N1, ag<N2, T, NEXT> >
  { typedef typename match_type<N1, NEXT>::type type; };

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
    void tap(std::string prefix, const ag<NAME, T, NEXT> &a, bool output=false)
  { a.tap(prefix, output); }

  // Pre-declare these so we can see unions when we tap aggregates containing
  // them.
  template <typename NAME, typename T, typename NEXT> struct un;
  template <typename NAME, typename T, typename NEXT>
    void tap(std::string prefix, const un<NAME, T, NEXT> &a, bool output=false);

  template<typename NAME, typename T, typename NEXT>
    void ag<NAME, T, NEXT>::tap(std::string prefix, bool output) const
  {
    chdl::tap(prefix + '_' + NAME::str(), contents, output);
    next.tap(prefix, output);
  }

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    struct lookup
  {
  lookup(ag<NAME, T, NEXT> &a):
    value(lookup<
      QNAME,
      typename ag<NAME, T, NEXT>::next_t::name_t,
      typename ag<NAME, T, NEXT>::next_t::content_t,
      typename ag<NAME, T, NEXT>::next_t::next_t
    >(a.next).value) {}

    typename match_type<QNAME, ag<NAME, T, NEXT> >::type &value;
  };

  template <typename NAME, typename T, typename NEXT>
    struct lookup<NAME, NAME, T, NEXT>
  {
    lookup(ag<NAME, T, NEXT> &a): value(a.contents) {}
    T &value;
  };

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    struct constlookup
  {
  constlookup(const ag<NAME, T, NEXT> &a):
    value(constlookup<
      QNAME,
      typename ag<NAME, T, NEXT>::next_t::name_t,
      typename ag<NAME, T, NEXT>::next_t::content_t,
      typename ag<NAME, T, NEXT>::next_t::next_t
    >(a.next).value) {}

    const typename match_type<QNAME, ag<NAME, T, NEXT> >::type &value;
  };

  template <typename NAME, typename T, typename NEXT>
    struct constlookup<NAME, NAME, T, NEXT>
  {
    constlookup(const ag<NAME, T, NEXT> &a): value(a.contents) {}
    const T &value;
  };

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    typename match_type<QNAME, ag<NAME, T, NEXT> >::type
      &Lookup(ag<NAME, T, NEXT> &a)
  { return lookup<QNAME, NAME, T, NEXT>(a).value; }

  template <typename QNAME, typename NAME, typename T, typename NEXT>
    const typename match_type<QNAME, ag<NAME, T, NEXT> >::type
      &Lookup(const ag<NAME, T, NEXT> &a)
  { return constlookup<QNAME, NAME, T, NEXT>(a).value; }

#ifndef CHDL_AG_DISABLE_UNDERSCORE
#define _(ag, name) Lookup<STRTYPE(name)>(ag)
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
