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

void Optimizer::setLoopJumps(BasicBlockRef next_bb, BasicBlockRef body_end_bb,
                             size_t body_end) {
  for (auto loop_break : loop_breaks_) {
    LOG("Set loop break for BB:" << loop_break->id()
                                 << " to:" << loop_break->id());
    loop_break->addSuccessor(next_bb);
    next_bb->addPredecessor(loop_break);
  }

  if (loop_continues_.size() > 0) {
    BasicBlockRef update_bb;

    if (body_end == SIZE_MAX) {
      update_bb = body_end_bb;
    } else {
      update_bb = BasicBlock::create(next());
      update_bb->setType(BasicBlockType::LOOP_UPDATE_PENDING);
    }

    for (auto &loop_continue : loop_continues_) {
      if (update_bb->type() == BasicBlockType::LOOP_UPDATE_PENDING) {
        for (auto &bb_range : bb_ranges_) {
          size_t range_start = bb_range.first.first;
          size_t range_end = bb_range.first.second;
          size_t update_start = loop_continue.second;

          if (update_start >= range_start && update_start <= range_end) {
            BasicBlock::split(body_end_bb, update_bb, update_start);
            bb_ranges_.push_back({{update_start, body_end}, update_bb});
            update_bb->setType(BasicBlockType::LOOP_UPDATE);
            break;
          }
        }
      }

      LOG("Set loop continue for BB:" << loop_continue.first->id()
                                      << " to:" << update_bb->id());

      loop_continue.first->addSuccessor(update_bb);
      update_bb->addPredecessor(loop_continue.first);
    }
  }

  loop_breaks_.clear();
  loop_continues_.clear();
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
    auto inst = byte_code->offsets()[i].lock();
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
      if (inst->isForContextInit()) {
        auto context_end = i + inst->jumpOffset();
        i += inst->size();
        auto get_next_inst = byte_code->offsets()[i].lock();

        auto loop_init_bb = parent_bb;
        auto loop_init_last_inst_bb = loop_init_bb;

        BasicBlockRef loop_update_bb = BasicBlock::create(next());
        loop_update_bb->setType(BasicBlockType::LOOP_UPDATE);

        if (!get_next_inst->isForContextGetNext()) {
          if (!get_next_inst->isJump()) {
            assert(get_next_inst->opcode().is(CBC_EXT_CLONE_CONTEXT) ||
                   get_next_inst->opcode().is(CBC_EXT_CLONE_FULL_CONTEXT));
            loop_update_bb->addInst(get_next_inst);
            get_next_inst->setBasicBlock(loop_update_bb);
            LOG("add:" << *get_next_inst << ", to: " << loop_update_bb->id());

            i += get_next_inst->size();
            get_next_inst = byte_code->offsets()[i].lock();
          }

          if (get_next_inst->isJump()) {
            assert(get_next_inst->jumpOffset() > 0);

            auto init_offset = get_next_inst->jumpOffset();
            loop_init_bb = BasicBlock::create(next());
            loop_init_bb->setType(BasicBlockType::LOOP_INIT);

            auto loop_init_start = i + get_next_inst->size();
            auto loop_init_end = i + init_offset;

            BBResult w_loop_init = buildBasicBlock(
                byte_code, loop_init_bb, loop_init_start, loop_init_end - 1);

            loop_init_last_inst_bb = w_last_bb.lock();

            i = loop_init_end;
            get_next_inst = byte_code->offsets()[i].lock();
          }
        }

        assert(get_next_inst->isForContextGetNext());

        loop_update_bb->addInst(get_next_inst);
        get_next_inst->setBasicBlock(loop_update_bb);
        LOG("add:" << *get_next_inst << ", to: " << loop_update_bb->id());
        bb_ranges_.push_back({{i, i + get_next_inst->size()}, loop_update_bb});

        BasicBlockRef loop_body_bb = BasicBlock::create(next());
        loop_body_bb->setType(BasicBlockType::LOOP_CONTEXT_BODY_PENDING);

        auto loop_body_start = i + get_next_inst->size();
        auto loop_body_end = context_end;

        BBResult w_loop_body = buildBasicBlock(
            byte_code, loop_body_bb, loop_body_start, loop_body_end - 1);

        w_last_inst = w_loop_body.first;
        w_last_bb = w_loop_body.second;

        auto loop_body_last_inst = w_last_inst.lock();
        auto loop_body_last_inst_bb = w_last_bb.lock();

        loop_body_bb->setType(BasicBlockType::LOOP_BODY);

        assert(loop_body_last_inst->isForContextHasNext());

        BasicBlockRef loop_test_bb = BasicBlock::create(next());
        loop_test_bb->setType(BasicBlockType::LOOP_TEST);

        LOG("remove:" << *loop_body_last_inst
                      << ", from: " << loop_body_last_inst_bb->id());
        loop_body_last_inst_bb->insts().pop_back();
        loop_test_bb->addInst(loop_body_last_inst);
        loop_body_last_inst->setBasicBlock(loop_test_bb);
        LOG("add:" << *loop_body_last_inst << ", to: " << loop_test_bb->id());
        bb_ranges_.push_back(
            {{loop_body_last_inst->offset(),
              loop_body_last_inst->offset() + loop_body_last_inst->size()},
             loop_test_bb});

        BasicBlockRef next_bb = BasicBlock::create(next());

        /* loop_init_last_inst_bb -> loop_test_bb */
        loop_init_last_inst_bb->addSuccessor(loop_test_bb);
        loop_test_bb->addPredecessor(loop_init_last_inst_bb);

        /* loop_body_last_inst_bb -> loop_test_bb */
        loop_body_last_inst_bb->addSuccessor(loop_test_bb);
        loop_test_bb->addPredecessor(loop_body_last_inst_bb);

        /* loop_test_bb -> loop_update_bb */
        loop_test_bb->addSuccessor(loop_update_bb);
        loop_update_bb->addPredecessor(loop_test_bb);

        /* loop_test_bb -> next_bb */
        loop_test_bb->addSuccessor(next_bb);
        next_bb->addPredecessor(loop_test_bb);

        /* loop_update_bb -> loop_body_bb */
        loop_update_bb->addSuccessor(loop_body_bb);
        loop_body_bb->addPredecessor(loop_update_bb);

        setLoopJumps(next_bb, loop_test_bb, SIZE_MAX);

        i = context_end;
        bb_ranges_.push_back({{i, end}, next_bb});
        w_last_bb = next_bb;
        parent_bb = next_bb;
        continue;
      }

      if (inst->isForContextHasNext()) {
        break;
      }

      if (inst->isTryBlock()) {
        parent_bb->insts().pop_back();
        BasicBlockRef try_bb = BasicBlock::create(next());
        try_bb->setType(BasicBlockType::TRY_BLOCK);
        parent_bb->addSuccessor(try_bb);
        try_bb->addPredecessor(parent_bb); /* parent -> try_bb */

        auto try_start = i + inst->size();
        auto try_end = i + jump_offset;

        BBResult w_try = buildBasicBlock(byte_code, try_bb, try_start, try_end);

        w_last_inst = w_try.first;
        w_last_bb = w_try.second;

        auto try_last_inst = w_last_inst.lock();
        auto try_last_inst_bb = w_last_bb.lock();

        assert(try_last_inst->isCatchBlock());

        auto catch_start = try_end + try_last_inst->size();
        auto catch_end = try_end + try_last_inst->jumpOffset();

        BasicBlockRef catch_bb = BasicBlock::create(next());
        catch_bb->setType(BasicBlockType::CATCH_BLOCK);

        BBResult w_catch =
            buildBasicBlock(byte_code, catch_bb, catch_start, catch_end - 1);

        w_last_inst = w_catch.first;
        w_last_bb = w_catch.second;

        auto catch_last_inst = w_last_inst.lock();
        auto catch_last_inst_bb = w_last_bb.lock();

        i = catch_end;
        BasicBlockRef next_bb = BasicBlock::create(next());

        /* try_last_inst_bb -> next_bb */
        try_last_inst_bb->addSuccessor(next_bb);
        next_bb->addPredecessor(try_last_inst_bb);

        /* try_last_inst_bb -> catch_last_inst_bb */
        try_last_inst_bb->addSuccessor(catch_last_inst_bb);
        catch_last_inst_bb->addPredecessor(try_last_inst_bb);

        /* catch_last_inst_bb -> next_bb */
        catch_last_inst_bb->addSuccessor(next_bb);
        next_bb->addPredecessor(catch_last_inst_bb);

        auto finally_inst = byte_code->offsets()[i].lock();
        w_last_inst = finally_inst;

        if (finally_inst->isFinallyBlock()) {
          BasicBlockRef finally_bb = next_bb;
          finally_bb->setType(BasicBlockType::FINALLY_BLOCK);

          auto finally_start = i + finally_inst->size();
          auto finally_end = i + finally_inst->jumpOffset();
          BBResult w_finally = buildBasicBlock(byte_code, finally_bb,
                                               finally_start, finally_end - 1);

          w_last_inst = w_finally.first;
          w_last_bb = w_finally.second;

          auto finally_last_inst = w_last_inst.lock();
          auto finally_last_inst_bb = w_last_bb.lock();

          i = finally_end;
          next_bb = BasicBlock::create(next());
          /* finally_last_inst_bb -> next_bb */
          finally_last_inst_bb->addSuccessor(next_bb);
          next_bb->addPredecessor(finally_last_inst_bb);
        }

        bb_ranges_.push_back({{i, end}, next_bb});
        w_last_bb = next_bb;
        parent_bb = next_bb;
        continue;
      }

      if (inst->isConditionalJump()) {
        BasicBlockRef case_1_bb = BasicBlock::create(next());
        case_1_bb->setType(BasicBlockType::CONDTITION_CASE_1);
        parent_bb->addSuccessor(case_1_bb);
        case_1_bb->addPredecessor(parent_bb); /* cond -> case1 */

        BBResult w_case_1 = buildBasicBlock(
            byte_code, case_1_bb, i + inst->size(), i + jump_offset - 1);

        w_last_inst = w_case_1.first;
        w_last_bb = w_case_1.second;

        auto case_1_last_inst = w_last_inst.lock();
        auto case_1_last_inst_bb = w_last_bb.lock();

        if (!case_1_last_inst->isJump() || case_1_last_inst->isContextBreak()) {
          BasicBlockRef next_bb = BasicBlock::create(next());

          parent_bb->addSuccessor(next_bb); /* cond -> next_bb */
          next_bb->addPredecessor(parent_bb);

          if (!case_1_last_inst->isContextBreak()) {
            next_bb->addPredecessor(case_1_last_inst_bb);
            case_1_last_inst_bb->addSuccessor(next_bb); /* case1 -> next_bb */
          }

          i = case_1_last_inst->offset() + case_1_last_inst->size();
          w_last_bb = next_bb;
          parent_bb = next_bb;
        } else {
          int32_t case_2_offset = case_1_last_inst->jumpOffset();
          assert(case_2_offset > 0);
          Offset condition_end = case_1_last_inst->offset() + case_2_offset;

          BasicBlockRef case_2_bb = BasicBlock::create(next());
          case_2_bb->setType(BasicBlockType::CONDTITION_CASE_2);
          parent_bb->addSuccessor(case_2_bb);
          case_2_bb->addPredecessor(parent_bb); /* cond -> case2 */

          BBResult w_case_2 = buildBasicBlock(
              byte_code, case_2_bb, i + jump_offset, condition_end - 1);

          w_last_inst = w_case_2.first;
          w_last_bb = w_case_2.second;

          auto case_2_last_inst = w_last_inst.lock();
          auto case_2_last_inst_bb = w_last_bb.lock();

          BasicBlockRef next_bb = BasicBlock::create(next());
          if (!case_1_last_inst_bb->hasFlag(BasicBlockFlags::CONTEXT_BREAK)) {
            next_bb->addPredecessor(case_1_last_inst_bb); /* case1 -> new_bb */
            case_1_last_inst_bb->addSuccessor(next_bb);
          }

          if (!case_2_last_inst_bb->hasFlag(BasicBlockFlags::CONTEXT_BREAK)) {
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
        size_t last_loop_test_end = SIZE_MAX;
        for (auto riter = bb_ranges_.rbegin(); riter != bb_ranges_.rend();
             riter++) {
          auto &bb_range = *riter;

          if (bb_range.second->type() == BasicBlockType::LOOP_TEST_PENDING ||
              bb_range.second->type() ==
                  BasicBlockType::LOOP_CONTEXT_BODY_PENDING) {
            last_loop_test_end = bb_range.first.second;
            break;
          }
        }

        if (last_loop_test_end == SIZE_MAX) {
          /* Must be the jump part of the conditional jump 1st case */
          break;
        }

        bool is_break = jump_target > last_loop_test_end;
        if (is_break) {
          /* This must be a break */
          loop_breaks_.push_back(parent_bb);
        } else if (bb_ranges_.back().second->type() ==
                       BasicBlockType::CONDTITION_CASE_1 &&
                   is_last_inst) {
          /* Must be the jump part of the conditional jump 1st case */
          break;
        } else {
          /* This must be a continue */
          loop_continues_.push_back({parent_bb, jump_target});
        }

        w_last_bb.lock()->addFlag(BasicBlockFlags::CONTEXT_BREAK);
        inst->setFlag(InstFlags::CONTEXT_BREAK);

        /* all upcoming instructions are dead until the next jump instruction */
        i += inst->size();
        while (i < end) {
          inst = byte_code->offsets()[i].lock();

          if (inst->isJump() || inst->isForContextHasNext()) {
            break;
          }
          i += inst->size();
          LOG("skip: " << *inst << " due to it's dead after "
                       << (is_break ? "break " : "continue"));
        };

        if (!inst->isForContextHasNext()) {
          append_inst = false;
        }
        continue;
      } else if (bb_ranges_.back().second->type() ==
                     BasicBlockType::LOOP_BODY_PENDING ||
                 bb_ranges_.back().second->type() ==
                     BasicBlockType::LOOP_CONTEXT_BODY_PENDING) {
        /* found a loop body level continue */
        loop_continues_.push_back({parent_bb, jump_target});
        while (i < end) {
          i += inst->size();
          inst = byte_code->offsets()[i].lock();

          if (i == jump_target) {
            /* found for update in the same level */
            if (!inst->isForContextHasNext()) {
              BasicBlockRef update_bb = BasicBlock::create(next());
              update_bb->setType(BasicBlockType::LOOP_UPDATE);

              update_bb->addPredecessor(parent_bb);
              parent_bb->addSuccessor(update_bb); /* parent -> update */

              w_last_bb = update_bb;
              parent_bb = update_bb;
              w_last_inst = inst;
            }

            /* Invalidate update creation */
            loop_continues_.pop_back();
            bb_ranges_.push_back({{i, end}, parent_bb});
            break;
          }
          LOG("skip: " << *inst << " due to it's dead after continue");
        };
        continue;
      }

      /* found for statement */
      BasicBlockRef last_loop_body = current_loop_body_;

      int32_t test_start = jump_offset;
      BasicBlockRef test_bb = BasicBlock::create(next());
      test_bb->setType(BasicBlockType::LOOP_TEST_PENDING);
      BBResult w_test =
          buildBasicBlock(byte_code, test_bb, i + jump_offset, OffsetMax,
                          BasicBlockOptions::RETURN_ON_BACKWARD);

      w_last_inst = w_test.first;
      w_last_bb = w_test.second;

      auto test_last_inst = w_last_inst.lock();
      auto test_last_inst_bb = w_last_bb.lock();

      assert(test_last_inst->isJump());
      int32_t test_inst_offset = test_last_inst->jumpOffset();
      assert(test_inst_offset < 0);

      BasicBlockRef body_bb = BasicBlock::create(next());
      body_bb->setType(BasicBlockType::LOOP_BODY_PENDING);

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

      setLoopJumps(next_bb, body_last_inst_bb, body_end);

      test_bb->setType(BasicBlockType::LOOP_TEST);
      body_bb->setType(BasicBlockType::LOOP_BODY);

      parent_bb = next_bb;
      w_last_bb = next_bb;
      i = test_last_inst->offset() + test_last_inst->size();
      bb_ranges_.push_back({{i, end}, parent_bb});
      continue;
    } else if (options == BasicBlockOptions::RETURN_ON_BACKWARD) {
      bb_ranges_.back().first.second = i;
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

  for (auto &bb_range : bb_ranges_) {
    if (bb_range.second->isEmpty()) {
      bb_range.second->removeEmpty();
    } else if (bb_range.second->isInaccessible()) {
      bb_range.second->removeInaccessible();
    } else {
      byte_code->basicBlockList().push_back(bb_range.second);
    }
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
