/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "regalloc-linear-scan.h"
#include "basic-block.h"
#include "liveness-analyzer.h"
#include "optimizer.h"

namespace optimizer {

RegallocLinearScan::RegallocLinearScan() : Pass(), new_regs_count_(0) {}

RegallocLinearScan::~RegallocLinearScan() {}

bool RegallocLinearScan::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::LIVENESS_ANALYZER));

  /* arguments are also stored in register therefore they ar included as well */
  regs_count_ = byte_code->args().registerEnd();

  if (regs_count_ == 0) {
    return true;
  }

  sortIntervals(byte_code);
  computeRegisterMapping(byte_code);
  updateInstructions(byte_code);

  return true;
}

void RegallocLinearScan::sortIntervals(Bytecode *byte_code) {

  for (auto &iter : byte_code->liveRanges()) {
    for (auto res : iter.second) {
      intervals_.push_back({{iter.first, iter.first}, res});
    }
  }

  std::sort(intervals_.begin(), intervals_.end(),
            [](const RegLiveInterval &a, const RegLiveInterval &b) {
              if (a.second->start() < b.second->start()) {
                return true;
              }

              if (a.second->start() == b.second->start()) {
                return a.second->end() < b.second->end();
              }

              return false;
            });

  for (auto iter : intervals_) {
    LOG("REG: " << iter.first.first << " it: " << *iter.second);
  }
}

void RegallocLinearScan::computeRegisterMapping(Bytecode *byte_code) {
  RegLiveIntervalList active;
  RegList registers;

  for (int32_t i = regs_count_ - 1; i >= 0; i--) {
    registers.push_back(i);
  }

  for (auto &iter : intervals_) {
    expireOldIntervals(active, registers, iter);

    if (active.size() == regs_count_) {
      return;
    }

    if (iter.second->start() == iter.second->end()) {
      continue;
    }

    iter.first.second = registers.back();
    registers.pop_back();
    active.push_back(iter);

    if (active.size() > new_regs_count_) {
      new_regs_count_ = active.size();
    }
  }

  LOG("----------------------------------------------------");
  LOG("NEW TOTAL REGS: " << new_regs_count_);
  for (auto iter : intervals_) {
    LOG("REG: " << iter.first.second << " it: " << *iter.second);
  }
  LOG("----------------------------------------------------");
}

void RegallocLinearScan::expireOldIntervals(RegLiveIntervalList &active,
                                            RegList &registers,
                                            RegLiveInterval &interval) {

  std::sort(active.begin(), active.end(),
            [](const RegLiveInterval &a, const RegLiveInterval &b) {
              if (a.second->end() < b.second->end()) {
                return true;
              }

              if (a.second->end() == b.second->end()) {
                return a.second->start() < b.second->start();
              }

              return false;
            });

  for (auto iter = active.begin(); iter != active.end();) {
    if ((*iter).second->end() > interval.second->start()) {
      return;
    }

    registers.push_back((*iter).first.second);
    iter = active.erase(iter);
  }
}

void RegallocLinearScan::updateInstructions(Bytecode *byte_code) {
  if (new_regs_count_ == regs_count_) {
    return;
  }

  assert(regs_count_ > new_regs_count_);
  int32_t offset = new_regs_count_ - regs_count_;

  for (auto &iter : intervals_) {
    for (auto ins : byte_code->instructions()) {
      Argument &arg = ins->argument();
      if (ins->hasFlag(InstFlags::READ_REG)) {
        for (auto &reg : ins->readRegs()) {
          if (iter.first.first == reg) {
            reg = iter.first.second;
          }
        }

        for (auto &lit : arg.literals()) {
          if (lit.index() == iter.first.first) {
            lit.setIndex(iter.first.second);
          }
        }
      }

      if (ins->hasFlag(InstFlags::WRITE_REG)) {
        uint32_t &write_reg = ins->writeReg();

        if (iter.first.first == write_reg) {
          write_reg = iter.first.second;
        }
      }
    }
  }

  for (auto ins : byte_code->instructions()) {
    Argument &arg = ins->argument();
    for (auto &lit : arg.literals()) {
      if (lit.index() >= byte_code->args().registerEnd()) {
        lit.moveIndex(offset);
      }
    }
  }

  byte_code->args().moveRegIndex(offset);
  byte_code->literalPool().movePoolStart(offset -
                                         byte_code->args().argumentEnd());

  for (auto bb : byte_code->basicBlockList()) {
    LOG(*bb);
  }
}

} // namespace optimizer
