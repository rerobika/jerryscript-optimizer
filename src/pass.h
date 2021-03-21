/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef PASS_H
#define PASS_H

#include "bytecode.h"
#include "common.h"

namespace optimizer {

class Optimizer;

enum PassKind { NONE, IR_BUILDER, DOMINATOR };

class Pass {
public:
  Pass();
  virtual ~Pass();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);
};

} // namespace optimizer

#endif // PASS_H
