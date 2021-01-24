/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "basic-block.h"
namespace optimizer {
void BasicBlock::addInst(InstWeakRef inst) { insts().push_back(inst); }

void BasicBlock::addPredecessor(BasicBlockWeakRef bb) {
  std::cout << "Add " << bb.lock()->id() << "to " << this->id() << " as pred"
            << std::endl;
  predesessors().push_back(bb);
}

void BasicBlock::addSuccessor(BasicBlockWeakRef bb) {
  std::cout << "Add " << bb.lock()->id() << "to " << this->id() << " as suc"
            << std::endl;
  successors().push_back(bb);
}

} // namespace optimizer
