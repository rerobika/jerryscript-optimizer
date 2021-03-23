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

class Edge {
public:
  Edge(BasicBlock *from, BasicBlock *to) : from_(from), to_(to) {}

  auto from() const { return from_; }
  auto to() const { return to_; }
  auto &live() { return live_; }

private:
  BasicBlock *from_;
  BasicBlock *to_;
  std::vector<int8_t> live_;
};

struct EdgeHasher {
  size_t operator()(const Edge *edge) const {
    return reinterpret_cast<uintptr_t>(edge->from()) ^
           reinterpret_cast<uintptr_t>(edge->to());
  };
};

struct EdgeComparator {
  bool operator()(const Edge *a, const Edge *b) const {
    return a->from() == b->from() && a->to() == b->to();
  };
};

using RegBBMap = std::unordered_map<uint32_t, BasicBlockSet>;
using EdgeSet = std::unordered_set<Edge *, EdgeHasher, EdgeComparator>;

class LivenessAnalyzer : public Pass {
public:
  LivenessAnalyzer();
  ~LivenessAnalyzer();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "LivenessAnalyzer"; }

  virtual PassKind kind() { return PassKind::LIVENESS_ANALYZER; }

private:
  void findDirectPath(BasicBlockOrderedSet &path, BasicBlock *from,
                      BasicBlock *to);
  bool setsEqual(RegSet &a, RegSet &b);

  bool isLive(uint32_t reg, BasicBlock *from, BasicBlock *to);
  bool isLiveInAt(uint32_t reg, BasicBlock *bb);
  bool isLiveOutAt(uint32_t reg, BasicBlock *bb);
  void computeDefsUses(BasicBlockList &bbs, InstList &insns);
  void computeLiveInOut(BasicBlockList &bbs);
  void computeInOut(BasicBlockList &bbs);

  uint32_t regs_number_;
  uint32_t current_reg_;

  RegBBMap defs_;
  RegBBMap uses_;
  EdgeSet edges_;
};

} // namespace optimizer

#endif // LIVENESS_ANALYZER_H
