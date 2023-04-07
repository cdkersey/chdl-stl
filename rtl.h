#ifndef CHDL_RTL_H
#define CHDL_RTL_H

#include <chdl/chdl.h>
#include <chdl/nodeimpl.h>
#include <stack>
#include <iostream>
#include <vector>

namespace chdl {

//extern std::stack<node *> rtl_pred_stack, rtl_prev_stack;

node Reg(node d, node init);

void rtl_end();
void rtl_if(node x);
void rtl_elif(node x);
void rtl_else();
node rtl_pred();
void tap_pred(std::string name);

template <typename T> T Reg(const T& d, const T &init) {
  bvec<sz<T>::value> qv, dv, iv;
  T q;
  Flatten(q) = qv;
  dv = Flatten(d);
  iv = Flatten(init);

  for (unsigned i = 0; i < sz<T>::value; ++i)
    qv[i] = Reg(dv[i], iv[i]);

  return q;
}

template <typename T> T Wreg(node wr, const T &d, const T &init) {
  T q;
  q = Reg(Mux(wr, q, d), init);
  return q;
}

struct rtl_nodelist_t {
  rtl_nodelist_t(node x, node n, rtl_nodelist_t **p): x(x), n(n), next(*p) {
    *p = this;
  }

  ~rtl_nodelist_t() { if (next) delete next; }

  node OrN() {
    if (next) return n || next->OrN();
    else return n;
  }

  node MuxN(node dft) {
    if (next) return Mux(n, next->MuxN(dft), x);
    else return Mux(n, dft, x);
  }

  node n, x;
  rtl_nodelist_t *next;
};

template <typename T> struct element { typedef T type; };

template <unsigned N, typename T>
  struct element<vec<N, T> > { typedef T type; };

template <typename T>
  struct elements { const static unsigned value = 1; };

template <unsigned N, typename T>
  struct elements<vec<N, T> > { const static unsigned value = N; };

template <typename T> struct assigner {
  assigner() {}

  assigner(const T &x): x(x) {}

  assigner<typename element<T>::type>
    operator[](bvec<CLOG2(elements<T>::value)> idx)
  {
    const unsigned ESZ = sz<typename element<T>::type>::value;
    assigner<typename element<T>::type> r;
    bvec<ESZ> vr = Flatten(r);
    bvec<sz<T>::value> vx(Flatten(x));

    for (unsigned i = 0; i < sz<T>::value; ++i) {
      vx[i] = vr[i % ESZ];
      mask[i] = (idx == Lit<CLOG2(elements<T>::value)>(i / ESZ))
        && r.mask[i % ESZ];
    }

    return r;
  }

  assigner<typename element<T>::type> operator[](unsigned idx) {
    const unsigned ESZ = sz<typename element<T>::type>::value;
    assigner<typename element<T>::type> r;
    bvec<ESZ> vr = Flatten(r.x);
    bvec<sz<T>::value> vx(Flatten(x));

    for (unsigned i = idx*ESZ; i < (idx + 1)*ESZ; ++i) {
      vx[i] = vr[i % ESZ];
      mask[i] = r.mask[i % ESZ];
    }

    return r;
  }

  template <unsigned A, unsigned B>
    assigner<vec<B - A + 1, typename element<T>::type> >
      operator[](range<A, B> l)
  {
    const unsigned N = B - A + 1;
    const unsigned ESZ = sz<typename element<T>::type>::value;
    assigner<vec<N, typename element<T>::type> > r;
    bvec<ESZ*N> vr = Flatten(r.x);
    bvec<sz<T>::value> vx(Flatten(x));

    for (unsigned i = A*ESZ; i < (B + 1)*ESZ; ++i) {
      vx[i] = vr[i - A*ESZ];
      mask[i] = r.mask[i - A*ESZ];
    }

    return r;
  }

  T operator=(const T &y) {
    mask = bvec<sz<T>::value>(rtl_pred());
    x = y;
    return x;
  }

  operator T() { return x; }

  bvec<sz<T>::value> mask;
  T x;
};


template <typename T, unsigned DLY = 1> struct rtl_reg : public T {
  rtl_reg(): preds(sz<T>::value) {}
  rtl_reg(const T& initial): initial(initial), preds(sz<T>::value) {}

  node regchain(node d, node initial, unsigned n) {
    if (n == 0) return d;

    return Reg(regchain(d, initial, n - 1), initial);
  }
  
  ~rtl_reg() {
    bvec<sz<T>::value> v = Flatten(*this), iv = Flatten(initial);

    for (unsigned i = 0; i < sz<T>::value; ++i) {
      if (preds[i]) {
        v[i] = regchain(preds[i]->MuxN(DLY>0 ? v[i] : iv[i]), iv[i], DLY);
        delete preds[i];
      } else {
        v[i] = iv[i];
      }
    }
  }

  T operator=(const T& r) {
    bvec<sz<T>::value> rv = Flatten(r);

    for (unsigned i = 0; i < sz<T>::value; ++i)
      new rtl_nodelist_t(rv[i], rtl_pred(), &preds[i]);

    return *this;
  }

  T operator=(const rtl_reg<T>& r) {
    *this = ((T)r);

    return *this;
  }

  assigner<typename element<T>::type>
    operator[](bvec<CLOG2(elements<T>::value)> idx)
  {
    const unsigned SZ(sz<T>::value),
                   ESZ(sz<typename element<T>::type>::value);

    assigner<typename element<T>::type> r;
    bvec<ESZ> rv(Flatten(r.x));

    for (unsigned i = 0; i < SZ; ++i) {
      node indexed = (idx == Lit<CLOG2(elements<T>::value)>(i/ESZ))
        && r.mask[i%ESZ];
      new rtl_nodelist_t(rv[i % ESZ], rtl_pred() && indexed, &preds[i]);
    }

    return r;
  }

  assigner<typename element<T>::type> operator[](unsigned idx) {
    const unsigned ESZ(sz<typename element<T>::type>::value);

    assigner<typename element<T>::type> r;
    bvec<ESZ> rv(Flatten(r.x));

    for (unsigned i = idx*ESZ; i < (idx + 1)*ESZ; ++i) {
      node indexed = r.mask[i%ESZ];
      new rtl_nodelist_t(rv[i % ESZ], rtl_pred() && indexed, &preds[i]);
    }

    return r;
  }

  template <unsigned A, unsigned B>
    assigner<vec<B - A + 1, typename element<T>::type> >
      operator[](range<A, B> l)
  {
    const unsigned ESZ(sz<typename element<T>::type>::value);

    assigner<vec<B - A + 1, typename element<T>::type> > r;
    bvec<ESZ*(B - A + 1)> rv(Flatten(r.x));

    for (unsigned i = A*ESZ; i < (B + 1)*ESZ; ++i) {
      node indexed = r.mask[i - A*ESZ];
      new rtl_nodelist_t(rv[i - A*ESZ], rtl_pred() && indexed, &preds[i]);
    }

    return r;
  }

  T operator++(int) { *this = *this + Lit<sz<T>::value>(1); return *this; }
  T operator++() { T x = *this + Lit<sz<T>::value>(1); *this = x; return x; }
  T operator--(int) { *this = *this - Lit<sz<T>::value>(1); return *this; }
  T operator--() { T x = *this - Lit<sz<T>::value>(1); *this = x; return x; }
  T operator+=(const T &x) { return (*this = *this + x); }
  T operator-=(const T &x) { return (*this = *this - x); }
  T operator*=(const T &x) { return (*this = *this * x); }
  T operator/=(const T &x) { return (*this = *this / x); }
  T operator%=(const T &x) { return (*this = *this % x); }
  T operator&=(const T &x) { return (*this = *this & x); }
  T operator|=(const T &x) { return (*this = *this | x); }
  T operator^=(const T &x) { return (*this = *this ^ x); }

  template <typename U>
    T operator<<=(const U &x) { return (*this = *this << x); }
  template <typename U>
    T operator>>=(const U &x) { return (*this = *this >> x); }

  T initial;
  std::vector<rtl_nodelist_t *> preds;
};

template <typename T> using rtl_wire = rtl_reg<T, 0>;

};

#ifndef RTL_CONDITIONAL_MACROS
#define IF(x) rtl_if(x);
#define ELIF(x) rtl_elif(x);
#define ELSE rtl_else();
#define ENDIF rtl_end();
#endif

#endif
