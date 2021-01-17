/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "common.h"
#include "inst.h"

namespace optimizer {

#define INVALID_BASIC_BLOCK_ID UINT32_MAX

enum class BasicBlockOptions { NONE, CONDITIONAL };

class BasicBlock {
public:
  BasicBlock() : BasicBlock(INVALID_BASIC_BLOCK_ID) {}
  BasicBlock(BasicBlockID id) : id_(id) {}

  auto &predesessors() { return predesessors_; }
  auto &successors() { return successors_; }
  auto &dominated() { return dominated_; }
  auto &dominator() { return dominator_; }
  auto &insts() { return insts_; }
  auto id() const { return id_; }

  void addInst(InstWeakRef inst);
  void addPredecessor(BasicBlockWeakRef bb);
  void addSuccessor(BasicBlockWeakRef bb);

  static BasicBlockRef create(BasicBlockID id = 0) {
    return std::make_shared<BasicBlock>(id);
  }

  friend std::ostream &operator<<(std::ostream &os, const BasicBlock &bb) {
    os << "ID: " << bb.id() << std::endl;
    os << "predecessors: [";
    for (size_t i = 0; i < bb.predesessors_.size(); i++) {
      auto pred = bb.predesessors_[i].lock();
      os << pred->id() << (i + 1 == bb.predesessors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    os << "instructions:" << std::endl;

    for (auto &w_inst : bb.insts_) {
      auto inst = w_inst.lock();
      os << *inst << std::endl;
    }

    os << "successors: [";
    for (size_t i = 0; i < bb.successors_.size(); i++) {
      auto succ = bb.successors_[i].lock();
      os << succ->id() << (i + 1 == bb.successors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    return os;
  }

private:
  BasicBlockWeakList predesessors_;
  BasicBlockWeakList successors_;
  BasicBlockWeakList dominated_;
  BasicBlockWeakList dominator_;
  InstWeakList insts_;
  BasicBlockID id_;
};

} // namespace optimizer
#endif // BASIC_BLOCK_H
