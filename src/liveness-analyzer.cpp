/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "liveness-analyzer.h"
#include "basic-block.h"
#include "optimizer.h"

namespace optimizer {

LivenessAnalyzer::LivenessAnalyzer() : Pass() {}

LivenessAnalyzer::~LivenessAnalyzer() {}

bool LivenessAnalyzer::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::DOMINATOR));

  uint32_t regs_count = byte_code->args().registerCount();

  LOG("TOTAL REG count :" << regs_count);

  if (regs_count == 0) {
    return true;
  }

  BasicBlockList &bbs = byte_code->basicBlockList();
  InsList &insns = byte_code->instructions();

  computeDefsUses(bbs, insns);
  computeInOut(bbs);

  return true;
}

void LivenessAnalyzer::computeDefsUses(BasicBlockList &bbs, InsList &insns) {
  for (auto ins : insns) {

    if (ins->hasFlag(InstFlags::READ_REG)) {
      for (auto reg : ins->readRegs()) {
        if (ins->bb()->kill().find(reg) == ins->bb()->kill().end()) {
          ins->bb()->ue().insert(reg);
        }

        LOG("  REG: " << reg << " used by BB: " << ins->bb()->id());
      }
    }

    if (ins->hasFlag(InstFlags::WRITE_REG)) {
      ins->bb()->kill().insert(ins->writeReg());

      LOG("  REG: " << ins->writeReg()
                    << " defined by BB: " << ins->bb()->id());
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
      // out'[n]
      RegSet current_out = bb->liveOut();

      // out[n] <- ue[n] U (out[n] - kill[n])
      RegSet intersection;
      std::set_intersection(bb->liveOut().begin(), bb->liveOut().end(),
                            bb->kill().begin(), bb->kill().end(),
                            std::inserter(intersection, intersection.end()));

      // bb->liveIn().clear();
      bb->liveOut().clear();
      std::set_union(intersection.begin(), intersection.end(), bb->ue().begin(),
                     bb->ue().end(),
                     std::inserter(bb->liveOut(), bb->liveOut().end()));
      if (!setsEqual(current_out, bb->liveOut())) {
        changed = true;
      }
    }

    if (!changed) {
      break;
    }
  }

  LOG("------------------------------------------");

  for (auto bb : bbs) {
    std::stringstream ss;

    for (auto iter = bb->liveOut().begin(); iter != bb->liveOut().end();
         iter++) {
      ss << *iter;

      if (std::next(iter) != bb->liveOut().end()) {
        ss << ", ";
      }
    }

    LOG("BB " << bb->id() << " OUT: " << ss.str());
  }

  LOG("------------------------------------------");
}

} // namespace optimizer
