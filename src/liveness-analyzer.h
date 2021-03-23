/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef LIVENESS_ANALYZER_H
#define LIVENESS_ANALYZER_H

#include "bytecode.h"
#include "common.h"
#include "pass.h"

namespace optimizer {

class Optimizer;

class LivenessAnalyzer : public Pass {
public:
  LivenessAnalyzer();
  ~LivenessAnalyzer();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "LivenessAnalyzer"; }

  virtual PassKind kind() { return PassKind::LIVENESS_ANALYZER; }

private:
  bool setsEqual(RegSet &a, RegSet &b);
  void computeDefsUses(BasicBlockList &bbs, InsList &insns);
  void computeLiveOut(BasicBlock *bb);
  void computeLiveOuts(BasicBlockList &bbs);

  uint32_t regs_count_;
};

} // namespace optimizer

#endif // LIVENESS_ANALYZER_H
