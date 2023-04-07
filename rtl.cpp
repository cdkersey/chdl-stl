#include "rtl.h"

using namespace chdl;
using namespace std;

node chdl::Reg(node d, node init) {
  return Reg(d, nodes[(nodeid_t)init]->eval(0));
}

std::stack<node *> chdl::rtl_pred_stack, chdl::rtl_prev_stack;

void chdl::rtl_end() {
  delete rtl_pred_stack.top();
  delete rtl_prev_stack.top();
  rtl_pred_stack.pop();
  rtl_prev_stack.pop();
}

void chdl::rtl_if(node x) {
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

void chdl::rtl_elif(node x) {
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

void chdl::rtl_else() { rtl_elif(Lit(1)); }

node chdl::rtl_pred() {
  return rtl_pred_stack.empty() ? Lit(1) : *rtl_pred_stack.top();
}

void chdl::tap_pred(std::string name) {
  tap(name, rtl_pred());
}
