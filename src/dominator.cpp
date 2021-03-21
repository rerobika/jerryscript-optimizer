/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "dominator.h"
#include "basic-block.h"
#include "optimizer.h"

namespace optimizer {

Dominator::Dominator() : Pass() {}

Dominator::~Dominator() {}

bool Dominator::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::IR_BUILDER));

  BasicBlockList &bbs = byte_code->basicBlockList();

  for (auto &bb : bbs) {
    bb->dominated() = nullptr;
    bb->dominators().clear();
  }

  computeDominators(bbs);
  computeDominated(bbs);
  return true;
}

bool Dominator::dominates(BasicBlock *who, BasicBlock *whom, BasicBlock *root) {
  BasicBlockList stack{root};
  bool dominates = false;
  checkDominates(stack, dominates, who, whom, root);
  return dominates;
}

void Dominator::checkDominates(BasicBlockList &stack, bool &dominates,
                               BasicBlock *who, BasicBlock *whom,
                               BasicBlock *current) {

  current->iterateSuccessors(
      [this, &stack, &dominates, whom, who](BasicBlock *child) -> void {
        if (std::find(stack.begin(), stack.end(), child) != stack.end()) {
          return;
        }

        if (child == who) {
          for (auto &bb : stack) {
            if (bb->id() == whom->id()) {
              dominates = true;
              return;
            }
          }
        }

        if (!dominates) {
          stack.push_back(child);
          this->checkDominates(stack, dominates, who, whom, child);
          stack.pop_back();
        }
      });
}

void Dominator::computeDominated(BasicBlockList &bbs) {
  auto iter = bbs.begin();
  assert((*iter)->dominated() == nullptr); // the first node has no idom

  for (iter++; iter != bbs.end(); iter++) {
    BasicBlock *bb = (*iter);

    BasicBlockList set = bb->dominators();

    // Calculate dom(n) - n
    set.erase(std::remove(set.begin(), set.end(), bb), set.end());

    while (set.size() != 1) {
      for (auto iter = set.begin(); iter != set.end(); iter++) {
        assert(std::next(iter) != set.end());
        BasicBlock *next = *std::next(iter);
        if (dominates(*iter, next, bbs[0])) {
          set.erase(std::remove(set.begin(), set.end(), next), set.end());
          break;
        }
      }
    }

    bb->dominated() = set.back();
    // for (auto iter : bbs) {
    //   if (iter->id() == set.back()->id()) {
    //     bb->dominated() = set.back();
    //     break;
    //   }
    // }
  }

  for (auto &bb : bbs) {
    if (bb->dominated()) {
      LOG("BB: " << bb->id() << " idom: " << bb->dominated()->id());
    } else {
      LOG("BB: " << bb->id() << " idom: -");
    }
  }
}

void Dominator::computeDominators(BasicBlockList &bbs) {
  // dominator of the start node is the start itself
  bbs[0]->dominators().push_back(bbs[0]);

  // // for all other nodes, set all nodes as the dominators
  auto iter_start = ++bbs.begin();

  for (auto iter = iter_start; iter != bbs.end(); iter++) {
    for (auto &bb : bbs) {
      (*iter)->dominators().push_back(bb);
    }
  }

  // iteratively eliminate nodes that are not dominators

  while (true) {
    bool changed = false;

    for (auto iter = iter_start; iter != bbs.end(); iter++) {
      BasicBlock *bb = *iter;
      BasicBlockList &preds = bb->predecessors();

      auto iter_start = preds.begin();
      BasicBlock *pred = *iter_start;
      std::unordered_set<BasicBlock *> intersect{pred->dominators().begin(),
                                                 pred->dominators().end()};

      std::unordered_set<BasicBlock *> matches;

      for (auto iter = ++iter_start; iter != preds.end(); iter++) {
        pred = *iter;

        for (auto &dom : pred->dominators()) {
          if (intersect.find(dom) != intersect.end()) {
            matches.insert(dom);
          }
        }

        for (auto &elem : intersect) {
          if (matches.find(elem) == matches.end()) {
            intersect.erase(elem);
          }
        }

        matches.clear();
      }

      intersect.insert({bb});

      BasicBlockList &current_dominators = bb->dominators();

      for (auto elem : current_dominators) {
        if (intersect.find(elem) == intersect.end()) {
          changed = true;
          current_dominators.clear();

          for (auto elem : intersect) {
            current_dominators.push_back(elem);
          }
          break;
        }
      }
    }

    if (!changed) {
      break;
    }
  }

  for (auto bb : bbs) {
    std::stringstream ss;
    ss << "BB: " << bb->id() << " dominates: <";

    for (auto dom : bb->dominators()) {
      ss << dom->id();

      if (dom != bb->dominators().back()) {
        ss << ", ";
      }
    }
    ss << ">" << std::endl;
    LOG(ss.str());
  }
}

} // namespace optimizer
