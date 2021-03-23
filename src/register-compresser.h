/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef REGISTER_COMPRESSER_H
#define REGISTER_COMPRESSER_H

#include "bytecode.h"
#include "common.h"
#include "pass.h"

namespace optimizer {

class LiveInterval {
public:
  LiveInterval()
      : start_(std::numeric_limits<uint32_t>::max()),
        end_(std::numeric_limits<uint32_t>::max()) {}

  LiveInterval(uint32_t start, uint32_t end) : start_(start), end_(end) {}

  bool isStartSet() const {
    return start_ != std::numeric_limits<uint32_t>::max();
  }

  void update(uint32_t offset) {
    if (isStartSet()) {
      setEnd(offset);
    } else {
      setStart(offset);
    }
  }

  auto start() const { return start_; }
  auto end() const { return end_; }

  void setStart(uint32_t start) { start_ = start; }
  void setEnd(uint32_t end) { end_ = end; }

private:
  uint32_t start_;
  uint32_t end_;
};

using LiveIntervalMap = std::unordered_map<uint32_t, LiveInterval>;

class Optimizer;

class RegisterCompresser : public Pass {
public:
  RegisterCompresser();
  ~RegisterCompresser();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "RegisterCompresser"; }

  virtual PassKind kind() { return PassKind::REGISTER_COMPRESSER; }

private:
  void findLocals(BasicBlockList &bbs);
  void buildLiveIntervals();
  void adjustInstructions(InsList &insns);

  RegSet reusable_;
  uint32_t regs_count_;
  std::unordered_map<BasicBlock *, LiveIntervalMap> bb_li_map_;
};

} // namespace optimizer

#endif // REGISTER_COMPRESSER_H
