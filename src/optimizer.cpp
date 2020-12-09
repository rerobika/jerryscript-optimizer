/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "jerryscript-compiler.h"
#include "jerryscript.h"

#include "optimizer.h"

namespace optimizer {
Optimizer::Optimizer(Bytecode *bytecode) : bytecode_(bytecode) {
  jerry_init(JERRY_INIT_EMPTY);
}

Optimizer::~Optimizer() { jerry_cleanup(); }
} // namespace optimizer
