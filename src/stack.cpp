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
  assert(size() > registerSize());
  ValueRef value = data().back();
  data().pop_back();
  return value;
}

void Stack::pop(size_t count) {
  assert(size() - count > registerSize());
  data().resize(data().size() - count);
}

void Stack::push() { push(Value::_undefined()); };

void Stack::push(ValueRef value) {
  assert(size() + 1 < fullSize());
  data().push_back(value);
}

void Stack::resetOperands() {
  setLeft(Value::_undefined());
  setRight(Value::_undefined());
}

void Stack::shift(size_t from, size_t offset) {
  for (size_t i = 0; i < offset; i++) {
    data_[size() - registerSize() + from + i] =
        data_[size() - registerSize() + from + i - 1];
  }
}

void Stack::setRegister(size_t index, ValueRef value) {
  assert(index > registerSize());
  data_[index] = value;
}

void Stack::setStack(int32_t offset, ValueRef value) {
  assert(offset <= static_cast<int32_t>(stackLimit()));
  data_[size() + offset] = value;
}
} // namespace optimizer
