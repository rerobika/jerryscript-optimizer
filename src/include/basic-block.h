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

static const char *basic_block_type_names[] = {
    "None",
    "loop-body",
    "loop-test",
    "loop-update",
};

enum class BasicBlockFlags { NONE, BREAK_SUCC };
enum class BasicBlockOptions { NONE, DIRECT, CONDITIONAL, LOOP };
enum class BasicBlockType {
  NONE,
  LOOP_BODY,
  LOOP_TEST,
  LOOP_UPDATE,
};

class BasicBlock {
public:
  BasicBlock() : BasicBlock(INVALID_BASIC_BLOCK_ID) {}
  BasicBlock(BasicBlockID id) : id_(id), flags_(0) {}

  auto &predecessors() { return predecessors_; }
  auto &successors() { return successors_; }
  auto &dominated() { return dominated_; }
  auto &dominator() { return dominator_; }
  auto &insts() { return insts_; }
  auto id() const { return id_; }
  auto type() const { return type_; }

  bool isEmpty() const { return insts_.empty(); }
  bool isInaccessible() const { return predecessors_.empty(); }

  void setType(BasicBlockType type) { type_ = type; }

  void addInst(InstWeakRef inst);
  void addPredecessor(BasicBlockWeakRef bb);
  void addSuccessor(BasicBlockWeakRef bb);

  void removeEmpty();
  void removeInaccessible();
  bool removePredecessor(const BasicBlockID id);
  bool removeSuccessor(const BasicBlockID id);
  void remove();
  void removeAllPredecessors();
  void removeAllSuccessors();

  static BasicBlockRef split(BasicBlockRef bb, BasicBlockID id, size_t from);

  void addFlag(BasicBlockFlags flags) {
    flags_ |= static_cast<uint32_t>(flags);
  }

  bool hasFlag(BasicBlockFlags flags) {
    return (flags_ & static_cast<uint32_t>(flags)) != 0;
  }

  static BasicBlockRef create(BasicBlockID id = 0) {
    LOG("Create BB: " << id);
    return std::make_shared<BasicBlock>(id);
  }

  friend std::ostream &operator<<(std::ostream &os, const BasicBlock &bb) {
    os << "ID: " << bb.id()
       << (bb.id() == 0 ? " <start> " : bb.id() == 1 ? " <end>" : "")
       << std::endl;
    ;

    if (bb.type() != BasicBlockType::NONE) {
      os << "Type: " << basic_block_type_names[static_cast<uint32_t>(bb.type())]
         << std::endl;
    }

    os << "predecessors: [";
    for (size_t i = 0; i < bb.predecessors_.size(); i++) {
      auto pred = bb.predecessors_[i].lock();
      os << pred->id() << (i + 1 == bb.predecessors_.size() ? "" : ", ");
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
  BasicBlockWeakList predecessors_;
  BasicBlockWeakList successors_;
  BasicBlockWeakList dominated_;
  BasicBlockWeakList dominator_;
  InstWeakList insts_;
  BasicBlockID id_;
  BasicBlockType type_;
  uint32_t flags_;
};

} // namespace optimizer
#endif // BASIC_BLOCK_H
