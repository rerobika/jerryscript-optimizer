/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "bytecode.h"
#include "common.h"
#include "snapshot-readwriter.h"

namespace optimizer {

class Optimizer {
public:
  Optimizer(std::shared_ptr<Bytecode> bytecode);
  ~Optimizer();

  auto bytecode() const { return bytecode_; }

private:
  std::shared_ptr<Bytecode> bytecode_;
};

} // namespace optimizer

#endif // OPTIMIZER_H
