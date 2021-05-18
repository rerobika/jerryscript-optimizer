/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef CFG_BUILDER_H
#define CFG_BUILDER_H

#include "basic-block.h"
#include "common.h"
#include "inst.h"
#include "pass.h"
#include "snapshot-readwriter.h"

namespace optimizer {

class Optimizer;

class ControlFlowAnalysis : public Pass {
public:
  ControlFlowAnalysis();
  ~ControlFlowAnalysis();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "ControlFlowAnalysis"; }

  virtual PassKind kind() { return PassKind::CONTROL_FLOW_ANALYSIS; }

private:
  void findLeaders();
  void buildBlocks();
  void connectBlocks();
  BasicBlock *newBB();

private:
  std::vector<Ins *> leaders_;
  std::vector<BasicBlock *> bbs_;
  Bytecode *byte_code_;
  BasicBlockID bb_id_;
};

} // namespace optimizer

#endif // CFG_BUILDER_H
