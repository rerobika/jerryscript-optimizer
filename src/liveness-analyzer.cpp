/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "liveness-analyzer.h"
#include "basic-block.h"
#include "dominator.h"
#include "optimizer.h"

namespace optimizer {

LivenessAnalyzer::LivenessAnalyzer() : Pass() {}

LivenessAnalyzer::~LivenessAnalyzer() {}

bool LivenessAnalyzer::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::IR_BUILDER));
  assert(optimizer->isSucceeded(PassKind::DOMINATOR));

  regs_number_ = byte_code->args().registerCount();

  LOG("TOTAL REG count :" << regs_number_);

  BasicBlockList &bbs = byte_code->basicBlockList();
  InstList &insns = byte_code->instructions();

  computeDefsUses(insns);
  computeLiveInOut(bbs);
  computeInOut(bbs);

  return true;
}

bool LivenessAnalyzer::isLive(uint32_t reg, BasicBlock *from, BasicBlock *to) {
  RegSet &uses = from->uses();
  RegSet &defs = to->defs();

  for (auto use : uses) {
    if (defs.find(use) != defs.end()) {
      return false;
    }
  }
  return true;
}

bool LivenessAnalyzer::isLiveInAt(uint32_t reg, BasicBlock *bb) {
  for (auto pred : bb->predecessors()) {
    if (pred->liveOut().find(reg) == pred->liveOut().end()) {
      return false;
    }
  }
  return true;
}

bool LivenessAnalyzer::isLiveOutAt(uint32_t reg, BasicBlock *bb) {
  if (bb->defs().find(reg) != bb->defs().end()) {
    return false;
  }

  return isLiveInAt(reg, bb);
}

void LivenessAnalyzer::computeDefsUses(InstList &insns) {
  for (auto ins : insns) {
    if (ins->hasFlag(InstFlags::WRITE_REG)) {
      ins->bb()->defs().insert(ins->writeReg());
      LOG("  "
          << "REG: " << ins->writeReg()
          << " defined by BB: " << ins->bb()->id());
    }

    if (ins->hasFlag(InstFlags::READ_REG)) {
      ins->bb()->uses().insert(ins->readReg());
      LOG("  "
          << "REG: " << ins->readReg() << " used by BB: " << ins->bb()->id());
    }
  }
}

void LivenessAnalyzer::computeLiveInOut(BasicBlockList &bbs) {
  for (uint32_t i = 0; i < regs_number_; i++) {
    for (auto bb : bbs) {
      for (auto succ : bb->successors()) {
        if (isLive(i, bb, succ)) {
          bb->liveOut().insert(i);
        }
      }

      for (auto pred : bb->predecessors()) {
        if (isLive(i, pred, bb)) {
          bb->liveIn().insert(i);
        }
      }
    }
  }

  for (auto bb : bbs) {
    LOG("LIVE-IN BB " << bb->id());
    for (auto li : bb->liveIn()) {
      LOG("  "
          << "REG :" << li);
    }
    LOG("LIVE-OUT BB " << bb->id());
    for (auto lo : bb->liveOut()) {
      LOG("  "
          << "REG :" << lo);
    }
  }
}

bool LivenessAnalyzer::setsEqual(RegSet &a, RegSet &b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (auto elem : a) {
    if (b.find(elem) == b.end()) {
      return false;
    }
  }

  return true;
}

void LivenessAnalyzer::computeInOut(BasicBlockList &bbs) {

  while (true) {
    bool changed = false;

    for (auto bb : bbs) {
      // in'[n]
      RegSet current_in = bb->in();
      // out'[n]
      RegSet current_out = bb->out();

      bb->in().clear();
      bb->out().clear();

      // in[n] <- use[n] U (out[n] - def[n])
      RegSet intersection;
      std::set_intersection(bb->out().begin(), bb->out().end(),
                            bb->defs().begin(), bb->defs().end(),
                            std::inserter(intersection, intersection.end()));

      std::set_union(intersection.begin(), intersection.end(),
                     bb->uses().begin(), bb->uses().end(),
                     std::inserter(bb->in(), bb->in().end()));

      // out[n] <- U(s in succ[n]) in[s]
      for (auto succ : bb->successors()) {
        for (uint32_t i = 0; i < regs_number_; i++) {
          if (isLiveInAt(i, succ)) {
            bb->out().insert(i);
          }
        }
      }

      if (!setsEqual(current_in, bb->in()) ||
          !setsEqual(current_out, bb->out())) {
        changed = true;
      }
    }

    if (!changed) {
      break;
    }
  }

  for (auto bb : bbs) {
    LOG("IN BB " << bb->id());
    for (auto in : bb->in()) {
      LOG("  "
          << "REG :" << in);
    }
    LOG("OUT BB " << bb->id());
    for (auto out : bb->out()) {
      LOG("  "
          << "REG :" << out);
    }
  }
}

} // namespace optimizer
