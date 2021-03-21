/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "optimizer.h"
#include "ir-builder.h"

namespace optimizer {

Optimizer::Optimizer(BytecodeList &list) : list_(list) {}

Optimizer::~Optimizer() {
  for (auto pass : passes_) {
    delete pass;
  }
}

bool Optimizer::run() {
  for (auto &it : list_) {
    for (auto pass : passes_) {
      if (!pass->run(it)) {
        return false;
      }
    }
  }

  return true;
}

} // namespace optimizer
