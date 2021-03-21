/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef DOMINATOR_H
#define DOMINATOR_H

#include "bytecode.h"
#include "common.h"
#include "pass.h"

namespace optimizer {

class Optimizer;

class Dominator : public Pass {
public:
  Dominator();
  ~Dominator();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

private:
  void computeDominators(BasicBlockList &bbs);
  void computeDominated(BasicBlockList &bbs);

  bool dominates(BasicBlock *who, BasicBlock *whom, BasicBlock *root);
  void checkDominates(BasicBlockList &stack, bool &dominates, BasicBlock *who,
                      BasicBlock *whom, BasicBlock *current);
};

} // namespace optimizer

#endif // DOMINATOR_H
