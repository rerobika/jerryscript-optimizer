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
#include "pass.h"

namespace optimizer {

class Optimizer {
public:
  Optimizer(BytecodeRefList &list);
  ~Optimizer();

  virtual bool run();
  auto &list() { return list_; }

  void addPass(Pass *pass) { passes_.push_back(pass); }

private:
  BytecodeRefList list_;
  std::vector<Pass *> passes_;
};

} // namespace optimizer

#endif // OPTIMIZER_H
