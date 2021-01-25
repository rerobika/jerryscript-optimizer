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

BBResult Optimizer::buildBasicBlock(BytecodeRef byte_code, BasicBlockRef bb,
                                    Offset start, Offset end,
                                    BasicBlockOptions options) {
  LOG("Build ID: " << bb->id() << ", from: " << start << ", to: " << end);
  bb_ranges_.push_back({{start, end}, bb});
  BasicBlockRef parent_bb = bb;

  InstWeakRef w_last_inst;
  BasicBlockWeakRef w_last_bb = parent_bb;
  bool append_inst = true;

  for (Offset i = start; i <= end;) {
    auto &w_inst = byte_code->offsets()[i];
    auto inst = w_inst.lock();
    w_last_inst = inst;
    bool is_last_inst = (i + inst->size()) > end;

    if (append_inst) {
      parent_bb->addInst(inst);
      inst->setBasicBlock(parent_bb);
      LOG("add:" << *inst << ", to: " << parent_bb->id());
    } else {
      append_inst = true;
    }

    if (!inst->isJump()) {
      i += inst->size();
      continue;
    }

    int32_t jump_offset = inst->jumpOffset();

    if (jump_offset > 0) {
      if (inst->isConditionalJump()) {
        BasicBlockRef child_bb = BasicBlock::create(next());
        parent_bb->addSuccessor(child_bb);
        child_bb->addPredecessor(parent_bb); /* cond -> case1 */

        BBResult w_case_1 = buildBasicBlock(
            byte_code, child_bb, i + inst->size(), i + jump_offset - 1);

        w_last_inst = w_case_1.first;
        w_last_bb = w_case_1.second;

        auto case_1_last_inst = w_last_inst.lock();
        auto case_1_last_inst_bb = w_last_bb.lock();

        if (!case_1_last_inst->isJump()) {
          BasicBlockRef next_bb = BasicBlock::create(next());

          parent_bb->addSuccessor(next_bb); /* cond -> next_bb */
          next_bb->addPredecessor(parent_bb);

          next_bb->addPredecessor(case_1_last_inst_bb);
          case_1_last_inst_bb->addSuccessor(next_bb); /* case1 -> next_bb */

          i = case_1_last_inst->offset() + case_1_last_inst->size();
          w_last_bb = next_bb;
          parent_bb = next_bb;
        } else {
          int32_t jump_inst_offset = case_1_last_inst->jumpOffset();
          assert(jump_inst_offset > 0);
          Offset condition_end = case_1_last_inst->offset() + jump_inst_offset;

          BasicBlockRef child2_bb = BasicBlock::create(next());
          parent_bb->addSuccessor(child2_bb);
          child2_bb->addPredecessor(parent_bb); /* cond -> case2 */

          BBResult w_case_2 = buildBasicBlock(
              byte_code, child2_bb, i + jump_offset, condition_end - 1);

          w_last_inst = w_case_2.first;
          w_last_bb = w_case_2.second;

          auto case_2_last_inst = w_last_inst.lock();
          auto case_2_last_inst_bb = w_last_bb.lock();

          BasicBlockRef next_bb = BasicBlock::create(next());
          if (!case_1_last_inst_bb->hasFlag(BasicBlockFlags::BREAK_SUCC)) {
            next_bb->addPredecessor(case_1_last_inst_bb); /* case1 -> new_bb */
            case_1_last_inst_bb->addSuccessor(next_bb);
          }

          if (!case_2_last_inst_bb->hasFlag(BasicBlockFlags::BREAK_SUCC)) {
            next_bb->addPredecessor(case_2_last_inst_bb); /* case2 -> new_bb */
            case_2_last_inst_bb->addSuccessor(next_bb);
          }
          i = condition_end;
          w_last_bb = next_bb;
          parent_bb = next_bb;
        }

        bb_ranges_.push_back({{i, end}, parent_bb});
        continue;
      }

      /* Simple jump forward which could be a
         - for loop start - if the jump remains in range
         - end jump of an if's case - if the jump leaves the current range
         - loop break
      */
      size_t jump_target = i + jump_offset;
      if (jump_target > end) {
        size_t jump_distance = 0;
        for (auto riter = ++bb_ranges_.rbegin(); riter != bb_ranges_.rend();
             riter++) {
          auto &bb_range = *riter;
          size_t range_start = bb_range.first.first;
          size_t range_end = bb_range.first.second;

          if (jump_target >= range_start && jump_target <= range_end) {
            break;
          }

          jump_distance++;
          if (jump_target >= range_start &&
              bb_range.second->type() == BasicBlockType::LOOP_BODY) {
            loop_breaks_.push_back(parent_bb);
          }
        }

        if (jump_distance == 0) {
          /* Found the jump part of the conditional jump 1st case */
          if (is_last_inst) {
            break;
          }
          /* Found continue statement */
          else {
            loop_continues_.push_back({parent_bb, jump_target});
            while (i < end) {
              i += inst->size();
              inst = byte_code->offsets()[i].lock();

              if (inst->isJump()) {
                append_inst = false;
                break;
              }
              LOG("skip: " << *inst << " due to it's dead after continue");
            };
            continue;
          }
        } else {
          w_last_bb.lock()->addFlag(BasicBlockFlags::BREAK_SUCC);
          /* found loop break */
          while (i < end) {
            i += inst->size();
            inst = byte_code->offsets()[i].lock();

            if (inst->isJump()) {
              break;
            }

            LOG("skip: " << *inst << " due to it's dead after break");
          };
          append_inst = false;
          continue;
        }
      } else {
        bool covered = false;
        for (auto &bb_range : bb_ranges_) {
          size_t range_start = bb_range.first.first;
          if (range_start > jump_target) {
            covered = true;
            break;
          }
        }

        if (covered) {
          loop_continues_.push_back({parent_bb, jump_target});
          /* found top level loop continue */
          while (i < end) {
            i += inst->size();
            inst = byte_code->offsets()[i].lock();

            if (i == jump_target) {
              /* found for update in the same level */
              BasicBlockRef update_bb = BasicBlock::create(next());
              update_bb->setType(BasicBlockType::LOOP_UPDATE);

              update_bb->addPredecessor(parent_bb);
              parent_bb->addSuccessor(update_bb); /* parent -> update */

              w_last_bb = update_bb;
              parent_bb = update_bb;
              w_last_inst = inst;
              /* Invalidate update creation */
              loop_continues_.back().second = 0;
              bb_ranges_.push_back({{i, end}, parent_bb});
              break;
            }
            LOG("skip: " << *inst << " due to it's dead after continue");
          };
          continue;
        }
      }

      /* found for statement */
      int32_t test_start = jump_offset;
      BasicBlockRef test_bb = BasicBlock::create(next());
      test_bb->setType(BasicBlockType::LOOP_TEST);
      BBResult w_test = buildBasicBlock(byte_code, test_bb, i + jump_offset,
                                        OffsetMax, BasicBlockOptions::DIRECT);

      w_last_inst = w_test.first;
      w_last_bb = w_test.second;

      auto test_last_inst = w_last_inst.lock();
      auto test_last_inst_bb = w_last_bb.lock();

      assert(test_last_inst->isJump());
      int32_t test_inst_offset = test_last_inst->jumpOffset();
      assert(test_inst_offset < 0);

      BasicBlockRef body_bb = BasicBlock::create(next());
      body_bb->setType(BasicBlockType::LOOP_BODY);

      size_t body_start = test_last_inst->offset() + test_inst_offset;
      size_t body_end = i + test_start - 1;
      BBResult w_body =
          buildBasicBlock(byte_code, body_bb, body_start, body_end);

      w_last_inst = w_body.first;
      w_last_bb = w_body.second;

      auto body_last_inst = w_last_inst.lock();
      auto body_last_inst_bb = w_last_bb.lock();

      parent_bb->addSuccessor(test_bb);
      test_bb->addPredecessor(parent_bb); /*  parent -> test */

      test_last_inst_bb->addSuccessor(body_bb);
      body_bb->addPredecessor(test_last_inst_bb); /* test_end -> b_start */

      BasicBlockRef next_bb = BasicBlock::create(next());
      test_last_inst_bb->addSuccessor(next_bb);
      next_bb->addPredecessor(test_last_inst_bb); /* test_end -> next_bb */

      body_last_inst_bb->addSuccessor(test_bb);
      test_bb->addPredecessor(body_last_inst_bb); /*  b_last -> test */

      for (auto loop_break : loop_breaks_) {
        loop_break->addSuccessor(next_bb);
        next_bb->addPredecessor(loop_break);
      }

      for (auto &loop_continue : loop_continues_) {
        if (loop_continue.second != 0) {
          for (auto &bb_range : bb_ranges_) {
            size_t range_start = bb_range.first.first;
            size_t range_end = bb_range.first.second;
            size_t update_start = loop_continue.second;

            if (update_start >= range_start && update_start <= range_end) {
              BasicBlockRef update_bb =
                  BasicBlock::split(body_last_inst_bb, next(), update_start);

              loop_continue.first->addSuccessor(update_bb);
              update_bb->addPredecessor(loop_continue.first);

              bb_ranges_.push_back({{update_start, body_end}, update_bb});
              break;
            }
          }
        }
      }

      loop_breaks_.clear();
      loop_continues_.clear();

      parent_bb = next_bb;
      w_last_bb = next_bb;
      i = test_last_inst->offset() + test_last_inst->size();
      bb_ranges_.push_back({{i, end}, parent_bb});
      continue;
    } else if (options == BasicBlockOptions::DIRECT) {
      break;
    }

    i += inst->size();
  }

  return {w_last_inst, w_last_bb};
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
    if (bb_ranges.second->isEmpty()) {
      bb_ranges.second->removeEmpty();
      continue;
    }
    if (bb_ranges.second->isInaccessible()) {
      bb_ranges.second->removeInaccessible();
      continue;
    }
    byte_code->basicBlockList().push_back(bb_ranges.second);
  }

  bb_ranges_.clear();
  bb_id_ = 0;

  auto last_bb = byte_code->basicBlockList().back();
  byte_code->basicBlockList().push_back(bb_end);
  bb_end->addPredecessor(last_bb);
  last_bb->addSuccessor(bb_end);

  LOG("--------- function basicblocks start --------");

  for (auto &bb : byte_code->basicBlockList()) {
    LOG(*bb);
  }

  LOG("--------- function basicblocks end --------");
}

Optimizer::~Optimizer() {}
} // namespace optimizer
