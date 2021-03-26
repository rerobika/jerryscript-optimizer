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
  Stack() : Stack(0, 0) {}
  Stack(uint32_t stack_limit, uint32_t register_count)
      : stack_limit_(stack_limit + 1), register_count_(register_count),
        block_result_(Value::_undefined()), result_(Value::_undefined()),
        left_(Value::_undefined()), right_(Value::_undefined()) {
    data_.reserve(stack_limit); // TODO fix ternary
    registers_.reserve(register_count);

    for (uint32_t i = 0; i < register_count; i++) {
      registers_.emplace_back(Value::_any());
    }
  }

  auto &data() { return data_; }
  auto &registers() { return registers_; }
  auto stackSize() const { return data_.size(); }
  auto regSize() const { return registers_.size(); }
  auto registerCount() const { return register_count_; }
  auto stackLimit() const { return stack_limit_; }
  auto fullSize() const { return stack_limit_; }
  auto left() const { return left_; }
  auto right() const { return right_; }
  auto result() const { return result_; }
  auto blockResult() const { return block_result_; }

  void setLeft(ValueRef value) { left_ = value; }
  void setRight(ValueRef value) { right_ = value; }
  void setBlockResult(ValueRef value) { block_result_ = value; }
  void setResult(ValueRef value) { block_result_ = value; }
  void setRegister(size_t i, ValueRef value);
  void setStack(int32_t offset, ValueRef value);

  void resetOperands();
  void shift(size_t from, size_t offset);

  ValueRef &getRegister(int i) { return registers_[i]; }
  ValueRef &getStack(int i) { return data_[0]; }
  ValueRef &getStack() { return data_[stackSize()]; }

  ValueRef pop();
  void pop(size_t count);
  void push();
  void push(size_t count);
  void push(ValueRef value);

private:
  ValueRefList data_;
  ValueRefList registers_;
  uint32_t stack_limit_;
  uint32_t register_count_;
  ValueRef block_result_;
  ValueRef result_;
  ValueRef left_;
  ValueRef right_;
};

} // namespace optimizer

#endif // STACK_H
