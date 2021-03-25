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
  void computeLiveRanges(BasicBlockList &bbs);
  // void findLocals(BasicBlockList &bbs);
  void buildLiveRanges(Bytecode *byte_code, BasicBlockList &bbs);

  uint32_t regs_count_;
};

class LiveInterval {
public:
  LiveInterval() : LiveInterval(0, 0) {}
  LiveInterval(uint32_t start) : LiveInterval(start, 0) {}
  LiveInterval(uint32_t start, uint32_t end) : start_(start), end_(end) {}

  auto start() const { return start_; }
  auto end() const { return end_; }

  void setStart(uint32_t start) { start_ = start; }
  void setEnd(uint32_t end) { end_ = end; }

  friend std::ostream &operator<<(std::ostream &os, const LiveInterval &li) {
    os << "start: " << li.start() << " end: " << li.end();
    return os;
  }

private:
  uint32_t start_;
  uint32_t end_;
};

} // namespace optimizer

#endif // LIVENESS_ANALYZER_H
