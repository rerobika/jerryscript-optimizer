/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "basic-block.h"
namespace optimizer {
void BasicBlock::addInst(InstRef inst) { insts().push_back(inst); }

void BasicBlock::addPredecessor(BasicBlockRef bb) {
  predesessors().push_back(bb);
}

void BasicBlock::addSuccessor(BasicBlockRef bb) { successors().push_back(bb); }

BasicBlockRef BasicBlock::addSuccessor() {
  BasicBlockID bb_id =
      (successors().empty() ? id() : successors().back()->id()) + 1;
  BasicBlockRef bb = BasicBlock::create(bb_id);

  addSuccessor(bb);
  return bb;
}

} // namespace optimizer
