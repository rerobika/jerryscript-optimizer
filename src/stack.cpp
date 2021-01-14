/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "stack.h"

namespace optimizer {

void Stack::pop(uint32_t delta) {
  assert(stackDelta() > 0);
  stack_delta_ -= delta;
}

void Stack::push(uint32_t delta) {
  assert(stackDelta() + delta < stackSize());
  stack_delta_ += delta;
}
} // namespace optimizer
