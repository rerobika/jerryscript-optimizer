/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef DOMINATOR_H
#define DOMINATOR_H

#include "bytecode.h"
#include "common.h"
#include "pass.h"

namespace optimizer {

class Dominator : public Pass {
public:
  Dominator();
  ~Dominator();

  virtual bool run(Bytecode *byte_code);
  void buildTree(Bytecode *byte_code);

private:
  uint32_t depth_;
  BasicBlockList anc_;
  std::vector<BasicBlockList> buckets_;
  BasicBlockList idoms_;
  BasicBlockList labels_;
  BasicBlockList parents_;
  std::vector<uint32_t> semi_;
  BasicBlockList vert_;
};

} // namespace optimizer

#endif // DOMINATOR_H
