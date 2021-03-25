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

RegallocLinearScan::RegallocLinearScan() : Pass() {}

RegallocLinearScan::~RegallocLinearScan() {}

bool RegallocLinearScan::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::LIVENESS_ANALYZER));

  regs_count_ = byte_code->args().registerCount();

  if (regs_count_ == 0) {
    return true;
  }

  sortIntervals(byte_code);
  computeRegisterMapping(byte_code);

  return true;
}

void RegallocLinearScan::sortIntervals(Bytecode *byte_code) {

  for (auto &iter : byte_code->liveRanges()) {
    for (auto res : iter.second) {
      intervals_.push_back({iter.first, res});
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
    LOG("REG: " << iter.first << " it: " << *iter.second);
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

    iter.first = registers.back();
    registers.pop_back();
    active.push_back(iter);
  }

  LOG("----------------------------------------------------");
  for (auto iter : intervals_) {
    LOG("REG: " << iter.first << " it: " << *iter.second);
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

    registers.push_back((*iter).first);
    iter = active.erase(iter);
  }
}

} // namespace optimizer
