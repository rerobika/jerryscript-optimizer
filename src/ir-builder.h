/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "basic-block.h"
#include "common.h"
#include "inst.h"
#include "pass.h"
#include "snapshot-readwriter.h"

namespace optimizer {

class Optimizer;

class IRBuilder : public Pass {
public:
  IRBuilder();
  ~IRBuilder();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "IRBuilder"; }

  virtual PassKind kind() { return PassKind::IR_BUILDER; }

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

#endif // IR_BUILDER_H
