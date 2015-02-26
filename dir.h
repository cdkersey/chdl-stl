#ifndef CHDL_DIR_H
#define CHDL_DIR_H

#include "ag.h"

namespace chdl {
  // Direction wrappers for interface types. Directions are w.r.t. the module.
  enum direction_t { DIR_IN, DIR_OUT, DIR_INOUT, DIR_ALL };

  // A directed<DIRECTION, TYPE> is a TYPE that has direction DIRECTION.
  template <direction_t D, typename T> struct directed : public T {
    directed(): T() {}
    directed(const T &x): T(x) {}
    directed(const directed &d): T(d) {}

    operator T() { return *this; }
    operator T&() { return *this; }
    operator const T() { return *this; }
    operator const T&() { return *this; }

    directed &operator=(const T& r) { T(*this) = r; return *this; }
  };

  template <direction_t D, typename T>
    struct sz<directed<D, T> > { static const unsigned value = sz<T>::value; };

  // These are the usual names for directed types.
  template <typename T> using in = directed<DIR_IN, T>;
  template <typename T> using out = directed<DIR_OUT, T>;
  template <typename T> using inout = directed<DIR_INOUT, T>;

  // Reverse a directed aggregate
  template <typename T> struct reverse { typedef T type; }; // Base

  template <typename NAME, typename T, typename NEXT>
    struct reverse<ag<NAME, T, NEXT> >
  {
    typedef ag<
      NAME,
      typename reverse<T>::type,
      typename reverse<NEXT>::type
    > type;
  };

  // in->out
  template <typename NAME, typename T, typename NEXT>
    struct reverse<ag<NAME, in<T>, NEXT> >
  {
    typedef ag<NAME, out<T>, typename reverse<NEXT>::type > type;
  };

  // out->in
  template <typename NAME, typename T, typename NEXT>
    struct reverse<ag<NAME, out<T>, NEXT> >
  {
    typedef ag<NAME, in<T>, typename reverse<NEXT>::type > type;
  };

  // inout->inout
  template <typename NAME, typename T, typename NEXT>
    struct reverse<ag<NAME, inout<reverse<T> >, NEXT> >
  {
    typedef ag<NAME, inout<T>, typename reverse<NEXT>::type > type;
  };

  // Turn a directed aggregate into an undirected aggregate
  template <typename T> struct strip_dir
  { typedef T type; };

  // Strip direction from directed types thmselves
  template <direction_t D, typename T>
    struct strip_dir<directed<D, T> >
  { typedef T type; };

  // Strip direction from aggregate
  template <typename NAME, typename T, typename NEXT>
    struct strip_dir<ag<NAME, T, NEXT> >
  {
    typedef ag<
      NAME,
      typename strip_dir<T>::type,
      typename strip_dir<NEXT>::type
    > type;
  };

  // Connect signals in one directed aggregate to those in a complementary
  // directed aggregate.
  template <typename T>
    void Connect(T &x, T &y) {} // base

  // In case the top level is undirected but lower levels are.
  template <typename NAME, typename T, typename NEXT>
    void Connect(ag<NAME, T, NEXT> &x,
                 typename chdl::reverse<ag<NAME, T, NEXT> >::type &y)
  {
    Connect(x.contents, y.contents);

    Connect(x.next, y.next);
  }
  
  template <typename NAME, typename T, typename NEXT>
    void Connect(ag<NAME, in<T>, NEXT> &x,
                 typename chdl::reverse<ag<NAME, in<T>, NEXT> >::type &y)
  {
    x.contents = y.contents;

    Connect(x.next, y.next);
  }

  template <typename NAME, typename T, typename NEXT>
    void Connect(ag<NAME, out<T>, NEXT> &x,
                 typename chdl::reverse<ag<NAME, out<T>, NEXT> >::type &y)
  {
    y.contents = x.contents;

    Connect(x.next, y.next);
  }
}

#endif
