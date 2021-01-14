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
  Stack(uint32_t stack_limit, uint32_t register_size)
      : stack_limit_(stack_limit), register_size_(register_size),
        block_result_(Value::_undefined()) {
    data_.resize(stack_limit + register_size_);

    for (uint32_t i = 0; i < register_size; i++) {
      data_.push_back(Value::_any());
    }
  }

  auto data() const { return data_; }
  auto size() const { return data().size(); }
  auto registerSize() const { return register_size_; }
  auto fullSize() const { return stack_limit_; }
  auto left() const { return left_; }
  auto right() const { return right_; }

  void setLeft(ValueRef value) { left_ = value; }
  void setRight(ValueRef value) { right_ = value; }
  void setBlockResult(ValueRef value) { block_result_ = value; }

  void resetOperands();

  ValueRef &getRegister(int i) { return data_[i]; }
  ValueRef &getStack(int i) { return data_[registerSize() + i]; }

  ValueRef pop();
  void push(ValueRef value);

private:
  std::vector<ValueRef> data_;
  uint32_t stack_limit_;
  uint32_t register_size_;
  ValueRef block_result_;
  ValueRef left_;
  ValueRef right_;
};

} // namespace optimizer

#endif // STACK_H
