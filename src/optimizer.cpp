/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

extern "C" {
#include "jerryscript.h"
}

#include "optimizer.h"

namespace optimizer {
Optimizer::Optimizer(BytecodeRefList &list) : list_(std::move(list)) {}

Optimizer::~Optimizer() {}
} // namespace optimizer
