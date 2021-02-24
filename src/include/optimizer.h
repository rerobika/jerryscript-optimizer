/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "basic-block.h"
#include "common.h"
#include "inst.h"
#include "snapshot-readwriter.h"

namespace optimizer {

using BBResult = std::pair<InstWeakRef, BasicBlockWeakRef>;
using BBRange = std::pair<std::pair<int32_t, int32_t>, BasicBlockRef>;

class Optimizer {
public:
  Optimizer(BytecodeRefList &list);
  ~Optimizer();

  void buildBasicBlocks(BytecodeRef byte_code);

  auto &list() { return list_; }

private:
  BasicBlockRef findBB(size_t index);
  void setLoopJumps(BasicBlockRef next_bb, BasicBlockRef body_end_bb,
                    int32_t body_end, int32_t loop_end);

  BBResult buildBasicBlock(BytecodeRef byte_code, BasicBlockRef parent_bb,
                           int32_t start, int32_t end,
                           BasicBlockOptions options = BasicBlockOptions::NONE);

  BasicBlockID next() { return bb_id_++; }

private:
  BytecodeRefList list_;
  std::vector<BBRange> bb_ranges_;
  std::vector<std::pair<InstRef, int32_t>> unknown_jumps_;
  BasicBlockRef current_loop_body_;
  BasicBlockID bb_id_;
};

} // namespace optimizer

#endif // OPTIMIZER_H
