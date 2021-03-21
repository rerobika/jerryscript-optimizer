/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "dominator.h"
#include "basic-block.h"

namespace optimizer {

Dominator::Dominator() : Pass() {}

Dominator::~Dominator() {}

bool Dominator::run(Bytecode *byte_code) { return true; }

void Dominator::buildTree(Bytecode *byte_code) {
  // BasicBlockList &bbs = byte_code->basicBlockList();

  // for (auto &bb : bbs) {
  //   bb->dominated = nullptr;
  //   bb->dominators().clear();
  // }

  // ASSERT(bbs.size() >= 2);
  // ASSERT(bbs[0]->)
}

} // namespace optimizer
