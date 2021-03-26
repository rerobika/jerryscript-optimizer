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

class Optimizer;

class RegallocLinearScan : public Pass {
public:
  RegallocLinearScan();
  ~RegallocLinearScan();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "RegAllocLinearScan"; }

  virtual PassKind kind() { return PassKind::REGALLOC_LINEAR_SCAN; }

private:
  void sortIntervals(Bytecode *byte_code);
  void computeRegisterMapping(Bytecode *byte_code);
  void expireOldIntervals(RegLiveIntervalList &active, RegList &registers,
                          RegLiveInterval &interval);
  void updateInstructions(Bytecode *byte_code);

  uint32_t regs_count_;
  RegLiveIntervalList intervals_;
};

} // namespace optimizer

#endif // REGISTER_COMPRESSER_H
