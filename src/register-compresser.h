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

class RegisterCompresser : public Pass {
public:
  RegisterCompresser();
  ~RegisterCompresser();

  virtual bool run(Optimizer *optimizer, Bytecode *byte_code);

  virtual const char *name() { return "RegisterCompresser"; }

  virtual PassKind kind() { return PassKind::REGISTER_COMPRESSER; }

private:
  void findReusable(BasicBlockList &bbs);
  void adjustRegs(BasicBlockList &bbs, InsList &insns);

  RegSet reusable_;
  uint32_t regs_count_;
};

} // namespace optimizer

#endif // REGISTER_COMPRESSER_H
