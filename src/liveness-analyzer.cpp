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

  regs_count_ = byte_code->args().registerCount();

  LOG("TOTAL REG count :" << regs_count_);

  if (regs_count_ == 0) {
    return true;
  }

  BasicBlockList &bbs = byte_code->basicBlockList();
  InsList &insns = byte_code->instructions();

  computeDefsUses(bbs, insns);
  computeLiveOuts(bbs);
  // findLocals(bbs);
  buildLiveRanges(byte_code, bbs);

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

void LivenessAnalyzer::computeLiveOut(BasicBlock *bb) {
  RegSet new_liveout;

  for (auto succ : bb->successors()) {
    // out[n] <- ue[n] U (out[n] - comp(kill[n]))
    // Note: (out[n] - comp(kill[n])) == out[n] - (out[n] intersection kill[n])
    RegSet difference;

    for (auto reg : succ->liveOut()) {
      if (succ->kill().find(reg) == succ->kill().end()) {
        difference.insert(reg);
      }
    }

    new_liveout.insert(difference.begin(), difference.end());
    new_liveout.insert(bb->ue().begin(), bb->ue().end());
  }

  bb->liveOut() = std::move(new_liveout);
}

void LivenessAnalyzer::computeLiveOuts(BasicBlockList &bbs) {

  while (true) {
    bool changed = false;
    for (auto bb : bbs) {
      RegSet current_out = bb->liveOut();
      computeLiveOut(bb);

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

// void LivenessAnalyzer::findLocals(BasicBlockList &bbs) {
//   for (auto &bb : bbs) {
//     for (auto def : bb->kill()) {
//       if (bb->liveOut().find(def) == bb->liveOut().end()) {
//         bb->locals().insert(def);
//       }
//     }
//   }

//   for (auto &bb : bbs) {
//     std::stringstream ss;

//     for (auto iter = bb->locals().begin(); iter != bb->locals().end();
//     iter++) {
//       ss << *iter;

//       if (std::next(iter) != bb->locals().end()) {
//         ss << ", ";
//       }
//     }

//     LOG("BB " << bb->id() << " LOCALS: " << ss.str());
//   }
// }

void LivenessAnalyzer::buildLiveRanges(Bytecode *byte_code,
                                       BasicBlockList &bbs) {
  for (auto bb : bbs) {
    for (auto ins : bb->insns()) {
      if (ins->hasFlag(InstFlags::READ_REG)) {
        for (auto reg : ins->readRegs()) {
          auto res = byte_code->liveRanges().find(reg);
          if (res == byte_code->liveRanges().end()) {
            byte_code->liveRanges().insert(
                {reg,
                 {new LiveInterval(bb->insns()[0]->offset(), ins->offset())}});
            continue;
          }

          res->second.back()->setEnd(ins->offset());
        }
      }

      if (ins->hasFlag(InstFlags::WRITE_REG)) {
        uint32_t write_reg = ins->writeReg();

        auto res = byte_code->liveRanges().find(write_reg);
        if (res == byte_code->liveRanges().end()) {
          byte_code->liveRanges().insert(
              {write_reg, {new LiveInterval(ins->offset())}});
          continue;
        }

        res->second.back()->setEnd(ins->offset());
        res->second.push_back(new LiveInterval(ins->offset()));
      }
    }
  }

  for (auto &li_range : byte_code->liveRanges()) {
    LiveIntervalList &ranges = li_range.second;
    for (auto iter = ranges.begin(); iter != ranges.end();) {
      if ((*iter)->end() == 0) {
        iter = ranges.erase(iter);
        continue;
      }
      iter++;
    }
  }

  for (auto &iter : byte_code->liveRanges()) {
    LOG(" REG: " << iter.first);
    for (auto res : iter.second) {
      LOG(" li: " << *res);
    }
  }
}

} // namespace optimizer
