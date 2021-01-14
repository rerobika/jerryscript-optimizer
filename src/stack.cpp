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

void Stack::push(ValueRef value) { assert(size() + 1 < fullSize()); }

void Stack::resetOperands() {
  setLeft(Value::_undefined());
  setRight(Value::_undefined());
}

} // namespace optimizer
