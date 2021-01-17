/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "optimizer.h"

namespace optimizer {
Optimizer::Optimizer(BytecodeRefList &list)
    : list_(std::move(list)), bb_id_(0) {
  for (auto &it : list_) {
    buildBasicBlocks(it);
  }
}

InstWeakRef Optimizer::buildBasicBlock(BytecodeRef byte_code, BasicBlockRef bb,
                                       Offset start, Offset end,
                                       BasicBlockOptions options) {
  bb_ranges_.push_back({{start, end}, bb});
  BasicBlockRef parent_bb = bb;

  InstWeakRef last_inst;

  for (Offset i = start; i <= end;) {
    auto &w_inst = byte_code->offsets()[i];
    auto inst = w_inst.lock();
    last_inst = inst;
    parent_bb->addInst(inst);
    inst->setBasicBlock(parent_bb);

    if (inst->isJump()) {
      Offset jump_offset = inst->jumpOffset();

      if (jump_offset > 0) {
        if (options == BasicBlockOptions::CONDITIONAL) {
          return last_inst;
        }

        BasicBlockRef child_bb = BasicBlock::create(next());
        parent_bb->addSuccessor(child_bb);
        child_bb->addPredecessor(bb);

        if (inst->isConditionalJump()) {
          InstWeakRef w_jump_inst = buildBasicBlock(
              byte_code, child_bb, i + inst->size(), i + jump_offset - 1,
              BasicBlockOptions::CONDITIONAL);

          auto jump_inst = w_jump_inst.lock();
          assert(jump_inst->isJump());

          Offset jump_inst_offset = jump_inst->jumpOffset();
          assert(jump_inst_offset > 0);

          BasicBlockRef child2_bb = BasicBlock::create(next());
          parent_bb->addSuccessor(child2_bb);
          child2_bb->addPredecessor(parent_bb);

          Offset condition_end = jump_inst->offset() + jump_inst_offset;
          buildBasicBlock(byte_code, child2_bb, i + jump_offset,
                          condition_end - 1);
          i = condition_end;
          parent_bb = BasicBlock::create(next());
          parent_bb->addPredecessor(child_bb);
          parent_bb->addPredecessor(child2_bb);
          bb_ranges_.push_back({{i, end}, parent_bb});
          continue;
        }
        buildBasicBlock(byte_code, child_bb, i + inst->size(), i + jump_offset);

      } else {
        Offset jump_target = i + jump_offset;
        for (auto &bb_range : bb_ranges_) {
          auto &start = bb_range.first.first;
          auto &end = bb_range.first.second;

          if (jump_target >= start && jump_target <= end) {
            auto &target_bb = bb_range.second;
            target_bb->addPredecessor(bb);
            bb->addSuccessor(bb_range.second);
#ifndef NDEBUG
            jump_target = 0;
#endif /* !NDEBUG */
            break;
          }
        }

        assert(jump_target == 0);
      }
    }

    i += inst->size();
  }

  return last_inst;
}

void Optimizer::buildBasicBlocks(BytecodeRef byte_code) {
  BasicBlockRef bb_start = BasicBlock::create(next());
  BasicBlockRef bb_end = BasicBlock::create(next());
  BasicBlockRef bb = BasicBlock::create(next());
  bb->addPredecessor(bb_start);
  bb_start->addSuccessor(bb);
  byte_code->basicBlockList().push_back(std::move(bb_start));

  buildBasicBlock(byte_code, bb, 0, byte_code->instructions().back()->offset());

  for (auto &bb_ranges : bb_ranges_) {
    byte_code->basicBlockList().push_back(bb_ranges.second);
  }

  bb_ranges_.clear();
  bb_id_ = 0;

  auto last_bb = byte_code->basicBlockList().back();
  byte_code->basicBlockList().push_back(bb_end);
  bb_end->addPredecessor(last_bb);
  last_bb->addSuccessor(bb_end);

#if DUMP_INST
  std::cout << "--------- function basicblocks start --------" << std::endl;

  for (auto &bb : byte_code->basicBlockList()) {
    std::cout << *bb << std::endl;
  }

  std::cout << "--------- function basicblocks end --------" << std::endl;
#endif
}

Optimizer::~Optimizer() {}
} // namespace optimizer
