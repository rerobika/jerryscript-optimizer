/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "register-compresser.h"
#include "basic-block.h"
#include "optimizer.h"

namespace optimizer {

RegisterCompresser::RegisterCompresser() : Pass() {}

RegisterCompresser::~RegisterCompresser() {}

bool RegisterCompresser::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::LIVENESS_ANALYZER));

  regs_count_ = byte_code->args().registerCount();

  if (regs_count_ == 0) {
    return true;
  }

  BasicBlockList &bbs = byte_code->basicBlockList();
  InsList &insns = byte_code->instructions();

  findLocals(bbs);
  buildLiveIntervals();
  adjustLocals(insns);

  return true;
}

void RegisterCompresser::findLocals(BasicBlockList &bbs) {
  for (auto &bb : bbs) {
    for (auto def : bb->kill()) {
      if (bb->liveOut().find(def) == bb->liveOut().end()) {
        bb->locals().insert(def);
      }
    }

    if (!bb->locals().empty()) {
      bb_li_map_.insert({bb, {}});
    }
  }

  for (auto &bb : bbs) {
    std::stringstream ss;

    for (auto iter = bb->locals().begin(); iter != bb->locals().end(); iter++) {
      ss << *iter;

      if (std::next(iter) != bb->locals().end()) {
        ss << ", ";
      }
    }

    LOG("BB " << bb->id() << " LOCALS: " << ss.str());
  }
}

void RegisterCompresser::buildLiveIntervals() {
  for (auto &elem : bb_li_map_) {
    BasicBlock *bb = elem.first;
    LiveIntervalMap &li_map = elem.second;

    for (auto local : bb->locals()) {
      li_map.insert({local, {}});
    }

    for (auto ins : bb->insns()) {
      if (ins->hasFlag(InstFlags::READ_REG)) {
        for (auto reg : ins->readRegs()) {
          if (li_map.find(reg) == li_map.end()) {
            continue;
          }
          li_map.find(reg)->second.update(ins->offset());
        }
      }

      if (ins->hasFlag(InstFlags::WRITE_REG)) {
        uint32_t write_reg = ins->writeReg();
        if (li_map.find(write_reg) == li_map.end()) {
          continue;
        }
        li_map.find(write_reg)->second.update(ins->offset());
      }
    }
  }

  for (auto &elem : bb_li_map_) {
    BasicBlock *bb = elem.first;
    LiveIntervalMap &li_map = elem.second;

    LOG("BB " << bb->id() << ":");

    for (auto res : li_map) {
      LOG(" REG: " << res.first << " li: " << res.second);
    }
  }
}

void RegisterCompresser::adjustLocals(InsList &insns) {
  RegLiveIntervalList intervals;

  for (auto &elem : bb_li_map_) {
    LiveIntervalMap &li_map = elem.second;

    for (auto res : li_map) {
      intervals.push_back({res.first, res.second});
    }
  }

  if (intervals.size() <= 1) {
    return;
  }

  std::sort(intervals.begin(), intervals.end(),
            [](const RegLiveInterval &a, const RegLiveInterval &b) {
              if (a.second.start() < b.second.start()) {
                return true;
              }

              if (a.second.start() == b.second.start()) {
                return a.second.end() <= b.second.end();
              }

              return false;
            });

  for (auto &li : intervals) {
    LOG(li.second);
  }

  for (size_t i = 1; i < intervals.size(); i++) {
    RegLiveInterval &current = intervals[i];
    RegLiveInterval &prev = intervals[i - 1];

    if (current.second.start() >= prev.second.end()) {
      LOG("FREE reg:" << current.first);
    }
  }
}

} // namespace optimizer
