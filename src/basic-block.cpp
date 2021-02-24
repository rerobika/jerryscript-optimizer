/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "basic-block.h"
namespace optimizer {
void BasicBlock::addInst(InstWeakRef inst) {
  LOG("Add:" << *(inst.lock().get()) << ", to: " << this->id());
  insts().push_back(inst);
}

void BasicBlock::addPredecessor(BasicBlockWeakRef bb) {
  LOG("Add " << bb.lock()->id() << " to " << this->id() << " as pred");
  predecessors().push_back(bb);
}

void BasicBlock::addSuccessor(BasicBlockWeakRef bb) {
  LOG("Add " << bb.lock()->id() << " to " << this->id() << " as succ");
  successors().push_back(bb);
}

bool BasicBlock::removePredecessor(const BasicBlockID id) {
  BasicBlockWeakRef pred;
  bool deleted = false;
  predecessors().erase(
      std::remove_if(predecessors().begin(), predecessors().end(),
                     [id, &pred, &deleted, this](BasicBlockWeakRef bb) {
                       if (bb.lock()->id() == id) {
                         pred = bb;
                         LOG("Remove pred:" << id << " from bb:" << this->id());
                         deleted = true;
                         return true;
                       }
                       return false;
                     }),
      predecessors().end());
  if (!pred.expired()) {
    BasicBlockID pred_id = pred.lock()->id();
    pred.lock()->successors().erase(
        std::remove_if(
            pred.lock()->successors().begin(), pred.lock()->successors().end(),
            [pred_id, &deleted, this](BasicBlockWeakRef bb) {
              if (bb.lock()->id() == this->id()) {
                LOG("Remove succ:" << this->id() << " from bb:" << pred_id);
                deleted = true;
                return true;
              }
              return false;
            }),
        pred.lock()->successors().end());
  }

  return deleted;
}

bool BasicBlock::removeSuccessor(const BasicBlockID id) {
  BasicBlockWeakRef succ;
  bool deleted = false;
  successors().erase(
      std::remove_if(successors().begin(), successors().end(),
                     [id, &succ, &deleted, this](BasicBlockWeakRef bb) {
                       if (bb.lock()->id() == id) {
                         succ = bb;
                         LOG("Remove succ:" << id << " from bb:" << this->id());
                         deleted = true;
                         return true;
                       }
                       return false;
                     }),
      successors().end());
  if (!succ.expired()) {
    BasicBlockID succ_id = succ.lock()->id();
    succ.lock()->predecessors().erase(
        std::remove_if(succ.lock()->predecessors().begin(),
                       succ.lock()->predecessors().end(),
                       [succ_id, &deleted, this](BasicBlockWeakRef bb) {
                         if (bb.lock()->id() == this->id()) {
                           LOG("Remove pred:" << this->id()
                                              << " from bb:" << succ_id);
                           deleted = true;
                           return true;
                         }
                         return false;
                       }),
        succ.lock()->predecessors().end());
  }

  return deleted;
}

void BasicBlock::remove() {
  removeAllPredecessors();
  removeAllSuccessors();
}

void BasicBlock::removeAllPredecessors() {
  for (auto iter = predecessors().begin(); iter != predecessors().end();) {
    auto pred = (*iter).lock();
    if (pred->removeSuccessor(id())) {
      iter = predecessors().begin();
    } else {
      iter++;
    }
  }
}

void BasicBlock::removeAllSuccessors() {
  for (auto iter = successors().begin(); iter != successors().end();) {
    auto succ = (*iter).lock();

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
      pred.lock()->addSuccessor(succ);
      succ.lock()->addPredecessor(pred);
    }
  }

  remove();
}

void BasicBlock::split(BasicBlockRef bb_from, BasicBlockRef bb_into,
                       int32_t from) {
  LOG("Split BB:" << bb_from->id() << " to:" << bb_into->id()
                  << " from:" << from);
  bool copied_anything = false;
  for (auto iter = bb_from->insts().begin(); iter != bb_from->insts().end();) {
    auto inst = (*iter).lock();
    if (inst->offset() >= from) {
      bb_into->addInst(inst);
      inst->setBasicBlock(bb_into);
      iter = bb_from->insts().erase(iter);
      copied_anything = true;
    } else {
      iter++;
    }
  }

  if (copied_anything) {
    for (auto &succ : bb_from->successors()) {
      /* all bb_from succs -> bb_into succs */
      bb_into->addSuccessor(succ);
      succ.lock()->addPredecessor(bb_into);
    }

    bb_from->removeAllSuccessors();

    bb_into->addPredecessor(bb_from);
    bb_from->addSuccessor(bb_into); /* bb_from -> bb_into */
  }
}

} // namespace optimizer
