#ifndef CHDL_ALG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <map>
#include <string>
#include <vector>
#include <chdl/chdl.h>

namespace chdl {
  const unsigned ALG_MAX_BR(128);

struct alg_state {
  alg_state(): nextb(0), nextn(0) {}

  std::string lname() {
    std::ostringstream oss;
    oss << "__label_" << nextn++;
    return oss.str();    
  }
  
  void label(std::string name) {
    // Fallthrough path
    if (nextb > 0) br(name);

    cur = nextb++;
    labels[name] = cur;
    ref[name] = Lit<32>(cur);
    prev_branch.change_net(Lit(0));
  }

  std::string label() {
    std::string name = lname();
    label(name);
    return name;
  }

  node in_cur() { return pc == Lit<32>(cur); }

  void br(std::string dest, node pred = Lit(1)) {
    TAP(pred);
    node taken = pred && in_cur() && !prev_branch;
    TAP(taken);
    branches.push_back(make_pair(pred && in_cur() && !prev_branch, dest));
    prev_branch.change_net(prev_branch || taken);
  }

  void gen() {
    TAP(pc);
    TAP(next_pc);

    vec<ALG_MAX_BR, bvec<32> > next_pc_opt;

    int i = 0;
    for (auto &x : branches) {
      bvec<32> prev = Lit<32>(0);
      if (i > 0) prev = next_pc_opt[i - 1];

      next_pc_opt[i] = (bvec<32>(x.first) & ref[x.second]) | prev;

      ++i;
    }
    if (i > 0) next_pc = next_pc_opt[i - 1];
    TAP(next_pc_opt);

    for (unsigned i = 0; i < 32; ++i) {
      if (i < CLOG2(cur + 1)) pc[i] = Reg(next_pc[i]);
      else pc[i] = Lit(0);
    }
  }

  node prev_branch;
  std::vector<std::pair<node, std::string> > branches;
  std::map<std::string, int> labels;
  std::map<std::string, bvec<32> > ref;
  int cur, nextb, nextn;
  bvec<32> next_pc, pc;
};

alg_state *alg_st() {
  static alg_state *ptr = new alg_state();
  return ptr;
}

template <typename T> struct var {
  var(): cur(-1) { next = reg; }
  ~var() { reg = Reg(next); }

  operator T() {
    if (alg_st()->cur != cur) {
      cur = alg_st()->cur;
      for (unsigned i = 0; i < sz<T>::value; ++i)
        val[i].change_net(reg[i]);
    }

    T x;
    Flatten(x) = val;
    return x;
  }

  T operator=(T x) {
    cur = alg_st()->cur;
    bvec<sz<T>::value> xv = Flatten(x);
    for (unsigned i = 0; i < sz<T>::value; ++i)
      val[i].change_net(xv[i]);

    node sel = alg_st()->in_cur();
    bvec<sz<T>::value> new_next(Mux(sel, next, xv));
    for (unsigned i = 0; i < sz<T>::value; ++i)
      next[i].change_net(new_next[i]);
    
    return T(*this);
  }

  T operator=(var<T> x) {
    return ((*this) = T(x));
  }



  int cur;

  bvec<sz<T>::value> val, next, reg;
};

template <typename T>
  T operator+(var<T> &x) { return +T(x); }
template <typename T>
  T operator-(var<T> &x) { return -T(x); }

template <unsigned N>
  bvec<N> operator<<(var<bvec<N>> &a, var<bvec<CLOG2(N)>> &b) {
    return bvec<N>(a) << bvec<N>(b);
  }
template <unsigned N>
  bvec<N> operator>>(var<bvec<N>> &a, var<bvec<CLOG2(N)>> &b) {
    return bvec<N>(a) >> bvec<N>(b);
  }
template <unsigned N>
  bvec<N> operator<<(var<bvec<N>> &a, bvec<CLOG2(N)> b) {
    return bvec<N>(a) << b;
  }
template <unsigned N>
  bvec<N> operator>>(var<bvec<N>> &a, bvec<CLOG2(N)> b) {
    return bvec<N>(a) >> b;
  }

template <typename T>
  T operator+(var<T> &a, const T &b) { return T(a) + b; }
template <typename T>
  T operator+(const T &a, var<T> &b) { return a + T(b); }
template <typename T>
  T operator-(var<T> &a, const T &b) { return T(a) - b; }
template <typename T>
  T operator-(const T &a, var<T> &b) { return a - T(b); }
template <typename T>
  T operator-(var<T> &a, var<T> &b) { return T(a) - T(b); }
template <typename T>
  T operator*(var<T> &a, const T &b) { return T(a) * b; }
template <typename T>
  T operator*(const T &a, var<T> &b) { return a * T(b); }
template <typename T>
  T operator*(var<T> &a, var<T> &b) { return T(a) * T(b); }
template <typename T>
  T operator&(var<T> &a, const T &b) { return T(a) & b; }
template <typename T>
  T operator&(const T &a, var<T> &b) { return a & T(b); }
template <typename T>
  T operator|(var<T> &a, const T &b) { return T(a) | b; }
template <typename T>
  T operator|(const T &a, var<T> &b) { return a | T(b); }
template <typename T>
  T operator^(var<T> &a, const T &b) { return T(a) ^ b; }
template <typename T>
  T operator^(const T &a, var<T> &b) { return a ^ T(b); }


template <typename T>
  node operator>(var<T> &a, const T &b) { return T(a) > b; }
template <typename T>
  node operator>(const T &a, var<T> &b) { return a > T(b); }
template <typename T>
  node operator>(var<T> &a, var<T> &b) { return T(a) > T(b); }

template <typename T>
  node operator>=(var<T> &a, const T &b) { return T(a) >= b; }
template <typename T>
  node operator>=(const T &a, var<T> &b) { return a >= T(b); }
template <typename T>
  node operator>=(var<T> &a, var<T> &b) { return T(a) >= T(b); }

template <typename T>
  node operator<(var<T> &a, const T &b) { return T(a) < b; }
template <typename T>
  node operator<(const T &a, var<T> &b) { return a < T(b); }
template <typename T>
  node operator<(var<T> &a, var<T> &b) { return T(a) < T(b); }

template <typename T>
  node operator<=(var<T> &a, const T &b) { return T(a) <= b; }
template <typename T>
  node operator<=(const T &a, var<T> &b) { return a <= T(b); }
template <typename T>
  node operator<=(var<T> &a, var<T> &b) { return T(a) <= T(b); }

template <typename T>
  node operator==(var<T> &a, const T &b) { return T(a) == b; }
template <typename T>
  node operator==(const T &a, var<T> &b) { return a == T(b); }
template <typename T>
  node operator!=(var<T> &a, const T &b) { return T(a) != b; }
template <typename T>
  node operator!=(const T &a, var<T> &b) { return a != T(b); }

template <typename T>
  T operator+=(var<T> &a, const T &b) { return (a = T(a) + T(b)); }
template <typename T>
  T operator+=(var<T> &a, var<T> &b) { return (a = T(a) + T(b)); }

template <typename T>
  T operator-=(var<T> &a, const T &b) { return (a = T(a) - T(b)); }
template <typename T>
  T operator-=(var<T> &a, var<T> &b) { return (a = T(a) - T(b)); }


template <typename T> void tap(std::string name, const var<T> &x) {
  T xt;
  Flatten(xt) = x.reg;
  tap(name, xt);
}
};

#ifdef CHDL_ALG_MACROS

#define LABEL(str) do { \
  alg_st()->label(str); \
} while(0)

#define GOTO(str) do { \
  alg_st()->br(str, Lit(1)); \
} while(0)

#define BR(cond, str) do { \
  alg_st()->br((str), (cond)); \
} while(0)

#define WHILE(x, y) { \
  std::string __clabel = alg_st()->lname(); \
  alg_st()->br(__clabel, !(x)); \
  std::string __rlabel = alg_st()->label(); \
  y \
  alg_st()->br(__rlabel, (x)); \
  alg_st()->label(__clabel); \
}

#define FOR(i, w, inc, y) { \
  (i); \
  WHILE((w), { \
    y \
    (inc); \
  }); \
}

#define IF(x, y) {\
  std::string __clabel = alg_st()->lname();\
  alg_st()->br(__clabel, !(x));\
  alg_st()->label();\
  y\
  alg_st()->label(__clabel);\
}

#endif

#endif
