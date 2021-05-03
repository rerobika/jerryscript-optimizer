/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "cfg-analysis.h"
#include "optimizer.h"

namespace optimizer {

CFGAnalysis::CFGAnalysis() : Pass() {}

CFGAnalysis::~CFGAnalysis() {}

bool CFGAnalysis::run(Optimizer *optimizer, Bytecode *byte_code) {
  byte_code_ = byte_code;
  bb_id_ = 0;
  bbs_.clear();
  leaders_.clear();

  findLeaders();
  buildBlocks();
  connectBlocks();

  for (auto &bb : bbs_) {
    LOG(*bb);
  }

  byte_code->basicBlockList() = std::move(bbs_);
  return true;
}

BasicBlock *CFGAnalysis::newBB() {
  BasicBlock *bb = BasicBlock::create(bb_id_++);
  bbs_.push_back(bb);

  return bb;
}

void CFGAnalysis:: findLeaders() {

  auto &insns = byte_code_->instructions();
  auto iter = insns.begin();

  std::unordered_set<Ins *> leader_set;

  // The first instruction is always a leader
  leader_set.insert(*iter);

  for (iter++; iter != insns.end(); iter++) {
    auto ins = *iter;

    if (!ins->isJump()) {
      continue;
    }

    Ins *target_ins = byte_code_->insAt(ins->jumpTarget());

    if (ins->isConditionalJump()) {
      // The first instruction of a condition is always a leader
      leader_set.insert(*std::next(iter));
    } else if (ins->isTryContext()) {
      leader_set.insert(ins);
    }

    // The jump target is always a leader
    leader_set.insert(target_ins);
  }

  for (auto &it : leader_set) {
    leaders_.push_back(it);
  }

  std::sort(leaders_.begin(), leaders_.end(),
            [](Ins *a, Ins *b) { return a->offset() < b->offset(); });

  for (auto it : leaders_) {
    LOG("Leader:" << *it);
  }
}

void CFGAnalysis::buildBlocks() {

  auto &insns = byte_code_->instructions();
  auto iter = leaders_.begin();

  BasicBlock *bb_start = newBB();
  BasicBlock *bb = newBB();
  bb_start->addSuccessor(bb);
  bb_start->addFlag(BasicBlockFlags::INVALID);

  Ins *next_leader = std::next(iter) == leaders_.end() ? nullptr : *(++iter);

  bool found_jump = false;

  for (auto ins : insns) {
    if (ins == next_leader) {
      bb = newBB();
      next_leader = std::next(iter) == leaders_.end() ? nullptr : *(++iter);
      found_jump = false;
    }

    bb->addIns(ins);

    if (found_jump) {
      ins->addFlag(InstFlags::DEAD);
    }

    if (ins->isJump() && !ins->isTryContext()) {
      found_jump = true;
    }
  }

  BasicBlock *bb_end = BasicBlock::create(bb_id_++);
  bb_end->addFlag(BasicBlockFlags::INVALID);
  bb->addSuccessor(bb_end);
  bbs_.push_back(bb_end);
}

void CFGAnalysis::connectBlocks() {
  for (auto bb : bbs_) {
    if (!bb->isValid()) {
      continue;
    }

    bool found_jump = false;
    for (auto ins : bb->insns()) {
      if (ins->isJump()) {
        Ins *jump_target = byte_code_->insAt(ins->jumpTarget());

        if (ins->isConditionalJump()) {
          Ins *next_inst = ins->nextInst(byte_code_);
          bb->addSuccessor(next_inst->bb());
        } else if (ins->isTryStart()) {
          assert(jump_target->isTryCatch());

          Ins *try_last_inst = jump_target->prevInst(byte_code_);
          Ins *catch_next_inst = byte_code_->insAt(jump_target->jumpTarget());

          try_last_inst->bb()->addSuccessor(catch_next_inst->bb());
        }

        bb->addSuccessor(jump_target->bb());
        found_jump = true;
        break;
      }
    }

    if (!found_jump) {
      Ins *ins = bb->insns().back();

      if (ins->hasNext(byte_code_)) {
        Ins *next_inst = ins->nextInst(byte_code_);
        bb->addSuccessor(next_inst->bb());
      }
      continue;
    }
  }
}

} // namespace optimizer
