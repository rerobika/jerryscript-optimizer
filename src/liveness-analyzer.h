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

using RegInstMap =
    std::unordered_map<uint32_t, std::unordered_set<BasicBlock *>>;
using BBRegMap = std::unordered_map<BasicBlock *, RegSet>;

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

  bool isLive(uint32_t reg, BasicBlock *from, BasicBlock *to);
  bool isLiveInAt(uint32_t reg, BasicBlock *bb);
  bool isLiveOutAt(uint32_t reg, BasicBlock *bb);
  void computeDefsUses(InstList &insns);
  void computeLiveInOut(BasicBlockList &bbs);
  void computeInOut(BasicBlockList &bbs);

  uint32_t regs_number_;
};

} // namespace optimizer

#endif // LIVENESS_ANALYZER_H
