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

  void addInst(InstRef inst);
  void addPredecessor(BasicBlockRef bb);
  BasicBlockRef addSuccessor();
  void addSuccessor(BasicBlockRef bb);

  static BasicBlockRef create(BasicBlockID id = 0) {
    return std::make_shared<BasicBlock>(id);
  }

  friend std::ostream &operator<<(std::ostream &os, const BasicBlock &bb) {
    os << "ID: " << bb.id() << std::endl;
    os << "predecessors: [";
    for (size_t i = 0; i < bb.predesessors_.size(); i++) {
      os << bb.predesessors_[i]->id()
         << (i + 1 == bb.predesessors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    os << "instructions:" << std::endl;
    for (auto &inst : bb.insts_) {
      os << *inst << std::endl;
    }

    os << "successors: [";
    for (size_t i = 0; i < bb.successors_.size(); i++) {
      os << bb.successors_[i]->id()
         << (i + 1 == bb.successors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    return os;
  }

private:
  BasicBlockList predesessors_;
  BasicBlockList successors_;
  BasicBlockList dominated_;
  BasicBlockRef dominator_;
  InstList insts_;
  BasicBlockID id_;
};

} // namespace optimizer
#endif // BASIC_BLOCK_H
