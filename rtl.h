#ifndef CHDL_RTL_H
#define CHDL_RTL_H

#include <chdl/chdl.h>
#include <chdl/nodeimpl.h>
#include <stack>
#include <iostream>

namespace chdl {
const unsigned MAX_RTL = 64;

node Reg(node d, node init) {
  return Reg(d, nodes[(nodeid_t)init]->eval(0));
}

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

template <typename T, unsigned N> T Mux1Hot(bvec<N> sel, vec<N, T> in) {
  bvec<sz<T>::value> rv;
  T r;
  Flatten(r) = rv;

  vec<N, bvec<sz<T>::value> > inv;
  vec<sz<T>::value, bvec<N> > invt;

  for (unsigned i = 0; i < N; ++i) {
    inv[i] = Flatten(in[i]);

    for (unsigned j = 0; j < sz<T>::value; ++j)
      invt[j][i] = inv[i][j];
  }
 
  for (unsigned i = 0; i < sz<T>::value; ++i)
    rv[i] = OrN(sel & invt[i]);
  
  return r;
}

template <typename T, unsigned N>
  vec<N, T> Repl(const vec<N, T> &in, bvec<CLOG2(N)> idx, const T &x)
{
  vec<N, T> r;
  bvec<N> idx1h(Zext<N>(Decoder(idx)));
  
  for (unsigned i = 0; i < N; ++i)
    r[i] = Mux(idx1h[i], in[i], x);

  return r;
}

std::stack<node*> rtl_pred_stack, rtl_prev_stack;

template <typename T> struct rtl_reg;

template <typename T, unsigned M> struct rtl_assigner {
  rtl_assigner(rtl_reg<T> &r, bvec<M> idx): idx(idx), r(r) {}

  template <typename U> U operator=(const U &x) {
    r.assign_idx(idx, x);
    return x;
  }

  template <typename U>
    U operator+=(const U &x) { return r.assign_idx(idx, Mux(idx, r) + x); }
  template <typename U>
    U operator-=(const U &x) { return r.assign_idx(idx, Mux(idx, r) - x); }
  template <typename U>
    U operator*=(const U &x) { return r.assign_idx(idx, Mux(idx, r) * x); }
  template <typename U>
    U operator/=(const U &x) { return r.assign_idx(idx, Mux(idx, r) / x); }
  template <typename U>
    U operator%=(const U &x) { return r.assign_idx(idx, Mux(idx, r) % x); }
  template <typename U>
    U operator&=(const U &x) { return r.assign_idx(idx, Mux(idx, r) & x); }
  template <typename U>
    U operator|=(const U &x) { return r.assign_idx(idx, Mux(idx, r) | x); }
  template <typename U>
    U operator^=(const U &x) { return r.assign_idx(idx, Mux(idx, r) ^ x); }

  bvec<M> idx;
  rtl_reg<T> &r;
};

template <typename T> struct rtl_reg : public T {
  rtl_reg(): sources(0) { Flatten(initial) = Lit<sz<T>::value>(0); }
  rtl_reg(const T& initial): sources(0), initial(initial) {}

  ~rtl_reg() {
    node wr = OrN(preds);
    T(*this) = Wreg(OrN(preds), Mux1Hot(preds, src), initial);
  }
  
  T operator=(const T& r) {
    if (rtl_pred_stack.empty()) preds[sources] = Lit(1);
    else                        preds[sources] = *rtl_pred_stack.top();
    src[sources] = r;
    ++sources;

    return r;
  }

  T operator=(const rtl_reg<T>& r) {
    *this = (T)r;
    return (T)*this;
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

  template <typename U, unsigned M>
    U assign_idx(bvec<M> idx, const U &x)
  {
    *this = Repl(T(*this), idx, x);
    return x;
  }

  template <unsigned M> rtl_assigner<T, M> operator[](bvec<M> idx) {
    return rtl_assigner<T, M>(*this, idx);
  }

  template <typename U> auto operator[](const U& idx)
  {
    return ((T)*this)[idx];
  }
  
  int sources;
  bvec<MAX_RTL> preds;
  vec<MAX_RTL, T> src;
  T initial;
};

template <typename T> struct rtl_wire : public T {
  rtl_wire(): sources(0) {}

  ~rtl_wire() {
    T(*this) = Mux1Hot(preds, src);
  }
  
  T operator=(const T& r) {
    if (rtl_pred_stack.empty()) preds[sources] = Lit(1);
    else                        preds[sources] = *rtl_pred_stack.top();
    src[sources] = r;
    ++sources;

    return r;
  }

#if 0
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


  template <typename U, unsigned M>
    U assign_idx(bvec<M> idx, const U &x)
  {
    *this = Repl(T(*this), idx, x);
    return x;
  }
#endif

  int sources;
  bvec<MAX_RTL> preds;
  vec<MAX_RTL, T> src;
  T initial;
};

void rtl_end() {
  delete rtl_pred_stack.top();
  delete rtl_prev_stack.top();
  rtl_pred_stack.pop();
  rtl_prev_stack.pop();
}

void rtl_if(node x) {
  node prev, anyprev;

  if (rtl_pred_stack.empty()) {
    prev = Lit(1);
  } else {
    prev = *rtl_pred_stack.top();
    anyprev = *rtl_prev_stack.top();
  }

  rtl_pred_stack.push(new node);
  rtl_prev_stack.push(new node);

  *rtl_pred_stack.top() = x && prev;
  *rtl_prev_stack.top() = *rtl_pred_stack.top();
}

void rtl_elif(node x) {
  if (rtl_pred_stack.empty()) {
    std::cerr << "CHDL RTL: else without for." << std::endl;
    std::abort();
  }

  node up;
  node *anyprev = rtl_prev_stack.top();

  delete rtl_pred_stack.top();
  
  rtl_pred_stack.pop();
  rtl_prev_stack.pop();

  if (rtl_pred_stack.empty()) up = Lit(1);
  else                        up = *rtl_pred_stack.top();

  rtl_pred_stack.push(new node);
  rtl_prev_stack.push(new node);

  *rtl_pred_stack.top() = up && !*anyprev && x;
  *rtl_prev_stack.top() = *rtl_pred_stack.top() || *anyprev;
  
  delete anyprev;
}

void rtl_else() { rtl_elif(Lit(1)); }

void tap_pred(std::string name) {
  tap(name, *rtl_pred_stack.top());
}
};

#ifndef RTL_CONDITIONAL_MACROS
#define IF(x) rtl_if(x);
#define ELIF(x) rtl_elif(x);
#define ELSE rtl_else();
#define ENDIF rtl_end();
#endif

#endif
