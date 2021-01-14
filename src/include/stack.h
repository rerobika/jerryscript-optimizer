/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef STACK_H
#define STACK_H

#include "common.h"
#include "value.h"

namespace optimizer {

class Stack {
public:
  Stack() : Stack(0) {}
  Stack(uint32_t stack_size)
      : stack_size_(stack_size), stack_delta_(0),
        block_result_(Value::_undefined()) {
    stack_ = new Value[stack_size];
  }

  ~Stack() { delete[] stack_; }

  auto stackDelta() const { return stack_delta_; }
  auto stackSize() const { return stack_size_; }
  auto stack() const { return stack_; }

  auto stackTop() const { return stack() + stackDelta(); }

  void setStackDelta(int32_t delta) { stack_delta_ = delta; }

  void pop(uint32_t delta = 1);
  void push(uint32_t delta = 1);

private:
  uint32_t stack_size_;
  uint32_t stack_delta_;
  Value *stack_;
  Value block_result_;
};

} // namespace optimizer

#endif // STACK_H
