/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "basic-block.h"
namespace optimizer {
void BasicBlock::addIns(Ins *inst) {
  LOG("Add:" << *inst << ", to: " << this->id());
  insns().push_back(inst);
  inst->setBasicBlock(this);
}

void BasicBlock::addPredecessor(BasicBlock *bb) {
  LOG("Add " << bb->id() << " to " << this->id() << " as pred");
  predecessors().push_back(bb);
  bb->successors().push_back(this);
}

void BasicBlock::addSuccessor(BasicBlock *bb) {
  LOG("Add " << bb->id() << " to " << this->id() << " as succ");
  successors().push_back(bb);
  bb->predecessors().push_back(this);
}

bool BasicBlock::removePredecessor(const BasicBlockID id) {
  BasicBlock *pred = nullptr;
  bool deleted = false;
  predecessors().erase(
      std::remove_if(predecessors().begin(), predecessors().end(),
                     [id, &pred, &deleted, this](BasicBlock *bb) {
                       if (bb->id() == id) {
                         pred = bb;
                         LOG("Remove pred:" << id << " from bb:" << this->id());
                         deleted = true;
                         return true;
                       }
                       return false;
                     }),
      predecessors().end());
  if (pred != nullptr) {
    BasicBlockID pred_id = pred->id();
    pred->successors().erase(
        std::remove_if(pred->successors().begin(), pred->successors().end(),
                       [pred_id, &deleted, this](BasicBlock *bb) {
                         if (bb->id() == this->id()) {
                           LOG("Remove succ:" << this->id()
                                              << " from bb:" << pred_id);
                           deleted = true;
                           return true;
                         }
                         return false;
                       }),
        pred->successors().end());
  }

  return deleted;
}

bool BasicBlock::removeSuccessor(const BasicBlockID id) {
  BasicBlock *succ;
  bool deleted = false;
  successors().erase(
      std::remove_if(successors().begin(), successors().end(),
                     [id, &succ, &deleted, this](BasicBlock *bb) {
                       if (bb->id() == id) {
                         succ = bb;
                         LOG("Remove succ:" << id << " from bb:" << this->id());
                         deleted = true;
                         return true;
                       }
                       return false;
                     }),
      successors().end());
  if (succ != nullptr) {
    BasicBlockID succ_id = succ->id();
    succ->predecessors().erase(
        std::remove_if(succ->predecessors().begin(), succ->predecessors().end(),
                       [succ_id, &deleted, this](BasicBlock *bb) {
                         if (bb->id() == this->id()) {
                           LOG("Remove pred:" << this->id()
                                              << " from bb:" << succ_id);
                           deleted = true;
                           return true;
                         }
                         return false;
                       }),
        succ->predecessors().end());
  }

  return deleted;
}

void BasicBlock::remove() {
  removeAllPredecessors();
  removeAllSuccessors();
}

void BasicBlock::removeAllPredecessors() {
  for (auto iter = predecessors().begin(); iter != predecessors().end();) {
    auto pred = (*iter);
    if (pred->removeSuccessor(id())) {
      iter = predecessors().begin();
    } else {
      iter++;
    }
  }
}

void BasicBlock::removeAllSuccessors() {
  for (auto iter = successors().begin(); iter != successors().end();) {
    auto succ = (*iter);

    if (succ->removePredecessor(id())) {
      iter = successors().begin();
    } else {
      iter++;
    }
  }
}

void BasicBlock::removeInaccessible() {
  LOG("Removing BB:" << id() << " since it's inaccessible");
  assert(isInaccessible());

  removeAllSuccessors();
}

void BasicBlock::removeEmpty() {
  LOG("Removing BB:" << id() << " since it's empty");
  assert(isEmpty());

  for (auto pred : predecessors()) {
    for (auto succ : successors()) {
      pred->addSuccessor(succ);
      succ->addPredecessor(pred);
    }
  }

  remove();
}

void BasicBlock::split(BasicBlock *bb_from, BasicBlock *bb_into, uint32_t from) {
  LOG("Split BB:" << bb_from->id() << " to:" << bb_into->id()
                  << " from:" << from);
  bool copied_anything = false;

  for (auto iter = bb_from->insns().begin(); iter != bb_from->insns().end();) {
    auto inst = (*iter);
    if (inst->offset() >= from) {
      bb_into->addIns(inst);
      inst->setBasicBlock(bb_into);
      iter = bb_from->insns().erase(iter);
      copied_anything = true;
    } else {
      iter++;
    }
  }

  if (copied_anything) {
    for (auto &succ : bb_from->successors()) {
      /* all bb_from succs -> bb_into succs */
      bb_into->addSuccessor(succ);
      succ->addPredecessor(bb_into);
    }

    bb_from->removeAllSuccessors();

    bb_into->addPredecessor(bb_from);
    bb_from->addSuccessor(bb_into); /* bb_from -> bb_into */
  }
}

} // namespace optimizer
