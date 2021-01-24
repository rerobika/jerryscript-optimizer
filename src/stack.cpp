/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "stack.h"

namespace optimizer {

ValueRef Stack::pop() {
  assert(stackSize() >= 0);
  ValueRef value = data_.back();
  data_.pop_back();
  return value;
}

void Stack::pop(size_t count) {
  assert(stackSize() - count >= 0);
  data_.resize(data_.size() - count);
}

void Stack::push(size_t count) {
  for (size_t i = 0; i < count; i++) {
    push();
  }
};
void Stack::push() { push(Value::_undefined()); };

void Stack::push(ValueRef value) {
  assert(stackSize() < stackLimit());
  data_.push_back(value);
}

void Stack::resetOperands() {
  setLeft(Value::_undefined());
  setRight(Value::_undefined());
}

void Stack::shift(size_t from, size_t offset) {
  for (size_t i = 0; i < offset; i++) {
    data_[stackSize() + from + i] = data_[stackSize() + from + i - 1];
  }
}

void Stack::setRegister(size_t index, ValueRef value) {
  assert(index < registerCount());
  registers_[index] = value;
}

void Stack::setStack(int32_t offset, ValueRef value) {
  assert(offset <= static_cast<int32_t>(stackLimit()));
  data_[stackSize() + offset] = value;
}
} // namespace optimizer
