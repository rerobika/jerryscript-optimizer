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

enum class BasicBlockFlags {
  NONE = 0,
  INVALID = 1 << 0,
  FOUND = 1 << 1,
};

using SuccessorTraverser = std::function<void(BasicBlock *child)>;
using PredecessorTraverser = SuccessorTraverser;

class BasicBlock {
public:
  BasicBlock() : BasicBlock(INVALID_BASIC_BLOCK_ID) {}
  BasicBlock(BasicBlockID id) : flags_(0), id_(id) {}

  auto &predecessors() { return predecessors_; }
  auto &successors() { return successors_; }
  auto &dominated() { return dominated_; }
  auto &dominators() { return dominators_; }

  auto &insns() { return insts_; }
  auto id() const { return id_; }

  // auto &defs() { return defs_; }
  // auto &uses() { return uses_; }
  // auto &liveIn() { return live_in_; }
  // auto &liveOut() { return live_out_; }
  // auto &in() { return in_; }
  // auto &out() { return out_; }

  auto &ue() { return ue_; }
  auto &kill() { return kill_; }
  auto &liveOut() { return live_out_; }

  bool isValid() const {
    return (flags_ & static_cast<uint32_t>(BasicBlockFlags::INVALID)) == 0;
  };

  bool isEmpty() const { return insts_.empty(); }
  bool isInaccessible() const { return predecessors_.empty(); }

  void addIns(Ins *inst);
  void addPredecessor(BasicBlock *bb);
  void addSuccessor(BasicBlock *bb);

  void removeEmpty();
  void removeInaccessible();
  bool removePredecessor(const BasicBlockID id);
  bool removeSuccessor(const BasicBlockID id);
  void remove();
  void removeAllPredecessors();
  void removeAllSuccessors();

  void iterateSuccessors(SuccessorTraverser cb) {
    for (auto bb : successors_) {
      cb(bb);
    }
  }

  void iteratePredecessors(PredecessorTraverser cb) {
    for (auto bb : predecessors_) {
      cb(bb);
    }
  }

  static void split(BasicBlock *bb_from, BasicBlock *bb_into, int32_t from);

  void addFlag(BasicBlockFlags flags) {
    flags_ |= static_cast<uint32_t>(flags);
  }

  bool hasFlag(BasicBlockFlags flags) {
    return (flags_ & static_cast<uint32_t>(flags)) != 0;
  }

  void clearFlag(BasicBlockFlags flags) {
    flags_ &= ~static_cast<uint32_t>(flags);
  }

  static BasicBlock *create(BasicBlockID id = 0) {
    LOG("Create BB: " << id);
    return new BasicBlock(id);
  }

  friend std::ostream &operator<<(std::ostream &os, const BasicBlock &bb) {
    os << "ID: " << bb.id()
       << (bb.isValid() ? "" : bb.id() == 0 ? " <start> " : " <end>")
       << std::endl;
    ;

    os << "predecessors: [";
    for (size_t i = 0; i < bb.predecessors_.size(); i++) {
      auto pred = bb.predecessors_[i];
      os << pred->id() << (i + 1 == bb.predecessors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    os << "instructions:" << std::endl;

    for (auto &w_inst : bb.insts_) {
      auto inst = w_inst;
      os << *inst << std::endl;
    }

    os << "successors: [";
    for (size_t i = 0; i < bb.successors_.size(); i++) {
      auto succ = bb.successors_[i];
      os << succ->id() << (i + 1 == bb.successors_.size() ? "" : ", ");
    }
    os << "]" << std::endl;

    return os;
  }

private:
  // IR-builder
  InstList insts_;

  // Dominator
  BasicBlockList predecessors_;
  BasicBlockList successors_;
  BasicBlock *dominated_;
  BasicBlockList dominators_;

  // Liveness
  // RegSet defs_;
  // RegSet uses_;
  // RegSet live_in_;
  // RegSet live_out_;
  // RegSet in_;
  // RegSet out_;

  RegSet ue_;
  RegSet kill_;
  RegSet live_out_;

  uint32_t flags_;
  BasicBlockID id_;
};

} // namespace optimizer
#endif // BASIC_BLOCK_H
