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

  findReusable(bbs);
  adjustRegs(bbs, insns);

  return true;
}

void RegisterCompresser::findReusable(BasicBlockList &bbs) {
  RegSet used;

  for (auto &bb : bbs) {
    for (auto out : bb->liveOut()) {
      used.insert(out);
    }
  }

  for (size_t i = 0; i < regs_count_; i++) {
    if (used.find(i) == used.end()) {
      LOG("Found reusable reg " << i);
      reusable_.insert(i);
    }
  }
}

void RegisterCompresser::adjustRegs(BasicBlockList &bbs, InsList &insns) {}

} // namespace optimizer
