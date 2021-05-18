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

class DominatorAnalysis : public Pass {
public:
  DominatorAnalysis();
  ~DominatorAnalysis();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);
  bool dominatedBy(BasicBlock *who, BasicBlock *by);

  virtual const char* name() {
    return "DominatorTree";
  }

  virtual PassKind kind() {
    return PassKind::DOMINATOR_ANALYSIS;
  }

private:
  void computeDominators(BasicBlockList &bbs);
  void computeImmDominator(BasicBlockList &bbs);

};

} // namespace optimizer

#endif // DOMINATOR_H
