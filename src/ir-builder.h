/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef IR_BUILDER_H
#define IR_BUILDER_H

#include "basic-block.h"
#include "common.h"
#include "inst.h"
#include "pass.h"
#include "snapshot-readwriter.h"

namespace optimizer {

using BBResult = std::pair<InstWeakRef, BasicBlockWeakRef>;
using BBRange = std::pair<std::pair<int32_t, int32_t>, BasicBlockRef>;

class IRBuilder : public Pass {
public:
  IRBuilder();
  ~IRBuilder();

  virtual bool run(BytecodeRef byte_code);

private:
  void buildBasicBlocks(BytecodeRef byte_code);

  BasicBlockRef findBB(size_t index);
  void setLoopJumps(BasicBlockRef next_bb, BasicBlockRef body_end_bb,
                    int32_t body_end, int32_t loop_end);

  BBResult buildBasicBlock(BytecodeRef byte_code, BasicBlockRef parent_bb,
                           int32_t start, int32_t end,
                           BasicBlockOptions options = BasicBlockOptions::NONE);

  BasicBlockID next() { return bb_id_++; }

private:
  std::vector<BBRange> bb_ranges_;
  std::vector<std::pair<InstRef, int32_t>> unknown_jumps_;
  BasicBlockRef current_loop_body_;
  BasicBlockID bb_id_;
};

} // namespace optimizer

#endif // IR_BUILDER_H
