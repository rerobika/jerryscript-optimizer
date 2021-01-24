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

void Optimizer::connectSub(InstWeakRef w_last_inst, BasicBlockRef bb) {
  auto last_inst = w_last_inst.lock();
  auto last_inst_bb = last_inst->bb().lock();
  if (last_inst_bb->id() != bb->id()) {
    last_inst_bb->addSuccessor(bb);
    bb->addPredecessor(last_inst_bb);
  }
}

InstWeakRef Optimizer::buildBasicBlock(BytecodeRef byte_code, BasicBlockRef bb,
                                       Offset start, Offset end,
                                       BasicBlockOptions options) {
  std::cout << "ID: " << bb->id() << ", from: " << start << ", to: " << end
            << std::endl;
  bb_ranges_.push_back({{start, end}, bb});
  BasicBlockRef parent_bb = bb;

  InstWeakRef w_last_inst;

  for (Offset i = start; i <= end;) {
    auto &w_inst = byte_code->offsets()[i];
    auto inst = w_inst.lock();
    w_last_inst = inst;
    parent_bb->addInst(inst);
    std::cout << "add:" << *inst << ", to: " << parent_bb->id() << std::endl;
    inst->setBasicBlock(parent_bb);

    if (!inst->isJump()) {
      i += inst->size();
      continue;
    }

    int32_t jump_offset = inst->jumpOffset();

    if (jump_offset > 0) {
      if (inst->isConditionalJump()) {
        BasicBlockRef child_bb = BasicBlock::create(next());
        parent_bb->addSuccessor(child_bb);
        child_bb->addPredecessor(bb);

        InstWeakRef w_last_inst = buildBasicBlock(
            byte_code, child_bb, i + inst->size(), i + jump_offset - 1,
            BasicBlockOptions::CONDITIONAL);

        auto last_inst = w_last_inst.lock();
        if (!last_inst->isJump()) {
          i = last_inst->offset() + last_inst->size();
          auto last_inst_bb = last_inst->bb().lock();

          if (last_inst_bb == child_bb) {
            BasicBlockRef next_bb = BasicBlock::create(next());
            parent_bb->addSuccessor(next_bb);
            next_bb->addPredecessor(parent_bb);
            next_bb->addPredecessor(child_bb);
            child_bb->addSuccessor(next_bb);
            parent_bb = next_bb;
          } else {
            auto last_bb = bb_ranges_.back().second;
            bb_ranges_.pop_back();
            BasicBlockRef next_bb = last_bb;

            if (!last_bb->isEmpty()) {
              next_bb = BasicBlock::create(next());
              last_bb->addSuccessor(next_bb);
              next_bb->addPredecessor(last_bb);
            }

            parent_bb->addSuccessor(next_bb);
            next_bb->addPredecessor(parent_bb);
            parent_bb = next_bb;
          }
        } else {
          int32_t jump_inst_offset = last_inst->jumpOffset();
          assert(jump_inst_offset > 0);

          BasicBlockRef child2_bb = BasicBlock::create(next());
          parent_bb->addSuccessor(child2_bb);
          child2_bb->addPredecessor(parent_bb);

          Offset condition_end = last_inst->offset() + jump_inst_offset;
          w_last_inst = buildBasicBlock(byte_code, child2_bb, i + jump_offset,
                                        condition_end - 1);

          i = condition_end;
          parent_bb = BasicBlock::create(next());
          parent_bb->addPredecessor(child_bb);
          parent_bb->addPredecessor(child2_bb);
          child_bb->addSuccessor(parent_bb);
          child2_bb->addSuccessor(parent_bb);
        }

        bb_ranges_.push_back({{i, end}, parent_bb});
        if (options == BasicBlockOptions::CONDITIONAL) {
          return w_last_inst;
        }

        continue;
      }

      if (options == BasicBlockOptions::CONDITIONAL) {
        return w_last_inst;
      }

      int32_t incr_start = jump_offset;
      BasicBlockRef incr_bb = BasicBlock::create(next());
      InstWeakRef w_jump_inst =
          buildBasicBlock(byte_code, incr_bb, i + jump_offset, OffsetMax,
                          BasicBlockOptions::DIRECT);
      auto jump_inst = w_jump_inst.lock();
      assert(jump_inst->isJump());
      int32_t jump_inst_offset = jump_inst->jumpOffset();
      assert(jump_inst_offset < 0);

      BasicBlockRef body_bb = BasicBlock::create(next());
      InstWeakRef w_last_inst = buildBasicBlock(
          byte_code, body_bb, jump_inst->offset() + jump_inst_offset,
          i + incr_start - 1);

      connectSub(w_last_inst, parent_bb);

      body_bb->setType(BasicBlockType::LOOP_BODY);
      incr_bb->setType(BasicBlockType::LOOP_UPDATE);

      parent_bb->addSuccessor(incr_bb);
      body_bb->addPredecessor(incr_bb);
      body_bb->addSuccessor(incr_bb);
      incr_bb->addPredecessor(body_bb);
      incr_bb->addPredecessor(parent_bb);
      incr_bb->addSuccessor(body_bb);

      i = jump_inst->offset() + jump_inst->size();
      parent_bb = BasicBlock::create(next());
      parent_bb->addPredecessor(incr_bb);
      incr_bb->addSuccessor(parent_bb);
      bb_ranges_.push_back({{i, end}, parent_bb});
      continue;
    } else if (options == BasicBlockOptions::DIRECT) {
      return w_last_inst;
    }

    i += inst->size();
  }

  return w_last_inst;
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
