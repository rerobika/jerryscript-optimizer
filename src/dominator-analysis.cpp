/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "dominator-analysis.h"
#include "basic-block.h"
#include "optimizer.h"

namespace optimizer {

DominatorAnalysis::DominatorAnalysis() : Pass() {}

DominatorAnalysis::~DominatorAnalysis() {}

bool DominatorAnalysis::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::CFG_BUILDER));

  BasicBlockList &bbs = byte_code->basicBlockList();

  for (auto &bb : bbs) {
    bb->dominated() = nullptr;
    bb->dominators().clear();
  }

  computeDominators(bbs);
  computeDominated(bbs);

  return true;
}

bool DominatorAnalysis::dominatedBy(BasicBlock *who, BasicBlock *by) {
  return std::find(who->dominators().begin(), who->dominators().end(), by) !=
         who->dominators().end();
}

void DominatorAnalysis::computeDominated(BasicBlockList &bbs) {
  auto iter = bbs.begin();
  assert((*iter)->dominated() == nullptr); // the first node has no idom

  for (iter++; iter != bbs.end(); iter++) {
    BasicBlock *bb = (*iter);

    BasicBlockList doms = bb->dominators();

    // Remove n from dom(n)
    doms.erase(std::remove(doms.begin(), doms.end(), bb), doms.end());

    while (doms.size() != 1) {
    restart:
      for (size_t i = 0; i < doms.size(); i++) {
        for (size_t j = i + 1; j < doms.size(); j++) {
          if (dominatedBy(doms[i], doms[j])) {
            doms.erase(std::remove(doms.begin(), doms.end(), doms[j]),
                       doms.end());
            goto restart;
          }
          if (dominatedBy(doms[j], doms[i])) {
            doms.erase(std::remove(doms.begin(), doms.end(), doms[i]),
                       doms.end());
            goto restart;
          }
        }
      }
    }

    // The remaining element in the doms will be de immidiate dominator
    bb->dominated() = doms.back();
  }

  for (auto &bb : bbs) {
    if (bb->dominated()) {
      LOG("BB: " << bb->id() << "'s idom: " << bb->dominated()->id());
    } else {
      LOG("BB: " << bb->id() << "'s idom: -");
    }
  }
}

void DominatorAnalysis::computeDominators(BasicBlockList &bbs) {
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
    ss << "BB: " << bb->id() << "'s dominators: <";

    for (auto dom : bb->dominators()) {
      ss << dom->id();

      if (dom != bb->dominators().back()) {
        ss << ", ";
      }
    }
    ss << ">";
    LOG(ss.str());
  }
}

} // namespace optimizer
