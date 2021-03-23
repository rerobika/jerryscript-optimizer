/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "liveness-analyzer.h"
#include "basic-block.h"
#include "dominator.h"
#include "optimizer.h"

namespace optimizer {

LivenessAnalyzer::LivenessAnalyzer() : Pass() {}

LivenessAnalyzer::~LivenessAnalyzer() {
  for (auto it : edges_) {
    delete it;
  }
}

bool LivenessAnalyzer::run(Optimizer *optimizer, Bytecode *byte_code) {
  assert(optimizer->isSucceeded(PassKind::IR_BUILDER));
  assert(optimizer->isSucceeded(PassKind::DOMINATOR));

  regs_number_ = byte_code->args().registerCount();

  LOG("TOTAL REG count :" << regs_number_);

  if (regs_number_ == 0) {
    return true;
  }

  BasicBlockList &bbs = byte_code->basicBlockList();
  InstList &insns = byte_code->instructions();

  computeDefsUses(bbs, insns);
  // computeLiveInOut(bbs);
  computeInOut(bbs);

  return true;
}

// void LivenessAnalyzer::findDirectPath(BasicBlockOrderedSet &path,
//                                       BasicBlock *from, BasicBlock *to) {
//   if (from == to) {
//     to->addFlag(BasicBlockFlags::FOUND);
//     return;
//   }

//   from->iterateSuccessors([this, &path, to](BasicBlock *child) {
//     if (!path.insert(child).second || to->hasFlag(BasicBlockFlags::FOUND)) {
//       // found a circle
//       return;
//     }

//     if (child == to) {
//       BasicBlockSet &defs = defs_.find(current_reg_)->second;

//       // If the direct path's blocks passes through any definition of reg
//       // then reg is not live in bb
//       for (auto p : path) {
//         if (defs.find(p) != defs.end()) {
//           return;
//         }
//       }

//       to->addFlag(BasicBlockFlags::FOUND);
//       return;
//     }

//     findDirectPath(path, child, to);
//   });
// }

// bool LivenessAnalyzer::isLive(uint32_t reg, BasicBlock *from, BasicBlock *to)
// {
//   if (!from->isValid() || !to->isValid()) {
//     return false;
//   }

//   Edge *edge = new Edge(from, to);
//   auto edge_res = edges_.find(edge);

//   if (edge_res == edges_.end()) {
//     edges_.insert(edge);
//     edge->live().resize(regs_number_);
//     std::fill(edge->live().begin(), edge->live().end(), -1);
//   } else {
//     delete edge;
//     edge = *edge_res;

//     if (edge->live()[reg] != -1) {
//       return edge->live()[reg];
//     }
//   }

//   auto res = uses_.find(reg);

//   if (res == uses_.end()) {
//     return false;
//   }

//   for (auto &use : res->second) {
//     BasicBlockOrderedSet path;
//     findDirectPath(path, to, use);
//     // Find direct path from to -> use(reg)
//     if (use->hasFlag(BasicBlockFlags::FOUND)) {
//       use->clearFlag(BasicBlockFlags::FOUND);

//       LOG("REG " << reg << " LIVE ON EDGE: " << from->id() << " TO:" <<
//       to->id()
//                  << " USE: " << use->id());
//       edge->live()[reg] = 1;
//       return true;
//     }
//   }

//   edge->live()[reg] = 0;
//   return false;
// }

// bool LivenessAnalyzer::isLiveInAt(uint32_t reg, BasicBlock *bb) {
//   for (auto pred : bb->predecessors()) {
//     if (pred->liveOut().find(reg) == pred->liveOut().end()) {
//       return false;
//     }
//   }
//   return true;
// }

// bool LivenessAnalyzer::isLiveOutAt(uint32_t reg, BasicBlock *bb) {
//   if (bb->defs().find(reg) != bb->defs().end()) {
//     return false;
//   }

//   return isLiveInAt(reg, bb);
// }

void LivenessAnalyzer::computeDefsUses(BasicBlockList &bbs, InstList &insns) {

  // for (size_t i = 0; i < regs_number_; i++) {
  //   defs_.insert({i, {bbs[0]}});
  //   bbs[0]->defs().insert(i);
  // }

  for (auto ins : insns) {

    if (ins->hasFlag(InstFlags::READ_REG)) {
      for (auto reg : ins->readRegs()) {
        if (ins->bb()->kill().find(reg) == ins->bb()->kill().end()) {
          ins->bb()->ue().insert(reg);
        }
        // ins->bb()->uses().insert(reg);

        // auto res = uses_.find(reg);
        // if (res == uses_.end()) {
        //   uses_.insert({reg, {ins->bb()}});
        // } else {
        //   res->second.insert(ins->bb());
        // }

        LOG("  REG: " << reg << " used by BB: " << ins->bb()->id());
      }
    }

    if (ins->hasFlag(InstFlags::WRITE_REG)) {
      ins->bb()->kill().insert(ins->writeReg());
      // ins->bb()->defs().insert(ins->writeReg());

      // auto res = defs_.find(ins->writeReg());
      // if (res == defs_.end()) {
      //   defs_.insert({ins->writeReg(), {ins->bb()}});
      // } else {
      //   res->second.insert(ins->bb());
      // }

      LOG("  REG: " << ins->writeReg()
                    << " defined by BB: " << ins->bb()->id());
    }
  }
}

// void LivenessAnalyzer::computeLiveInOut(BasicBlockList &bbs) {
//   for (current_reg_ = 0; current_reg_ < regs_number_; current_reg_++) {
//     for (auto bb : bbs) {
//       for (auto succ : bb->successors()) {
//         if (isLive(current_reg_, bb, succ)) {
//           bb->liveOut().insert(current_reg_);
//         }
//       }

//       for (auto pred : bb->predecessors()) {
//         if (isLive(current_reg_, pred, bb)) {
//           bb->liveIn().insert(current_reg_);
//         }
//       }
//     }
//   }
//   LOG("------------------------------------------");

//   for (auto bb : bbs) {
//     LOG("BB " << bb->id() << ":");
//     std::stringstream ss;
//     for (auto li : bb->liveIn()) {
//       ss << li << ", ";
//     }
//     LOG(" LIVE-IN: " << ss.str());
//     ss.str("");

//     for (auto lo : bb->liveOut()) {
//       ss << lo << ", ";
//     }

//     LOG(" LIVE-OUT: " << ss.str());
//     ss.str("");
//   }

//   LOG("------------------------------------------");
// }

bool LivenessAnalyzer::setsEqual(RegSet &a, RegSet &b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (auto elem : a) {
    if (b.find(elem) == b.end()) {
      return false;
    }
  }

  return true;
}

void LivenessAnalyzer::computeInOut(BasicBlockList &bbs) {

  while (true) {
    bool changed = false;

    for (auto bb : bbs) {
      // in'[n]
      // RegSet current_in = bb->liveIn();
      // out'[n]
      RegSet current_out = bb->liveOut();

      // in[n] <- use[n] U (out[n] - def[n])
      RegSet intersection;
      std::set_intersection(bb->liveOut().begin(), bb->liveOut().end(),
                            bb->kill().begin(), bb->kill().end(),
                            std::inserter(intersection, intersection.end()));

      // bb->liveIn().clear();
      bb->liveOut().clear();
      std::set_union(intersection.begin(), intersection.end(), bb->ue().begin(),
                     bb->ue().end(),
                     std::inserter(bb->liveOut(), bb->liveOut().end()));

      // // out[n] <- U(s in succ[n]) in[s]
      // for (auto succ : bb->successors()) {
      //   // for (uint32_t i = 0; i < regs_number_; i++) {
      //   //   if (isLiveInAt(i, succ)) {
      //   //     bb->liveOut().insert(i);
      //   //   }
      //   // }
      //   // if (!in->isValid()) {
      //   //   continue;
      //   // }

      //   for (auto in : succ->liveIn()) {
      //     bb->liveOut().insert(in);
      //   }
      // }

      if ( //! setsEqual(current_in, bb->liveIn()) ||
          !setsEqual(current_out, bb->liveOut())) {
        changed = true;
      }
    }

    if (!changed) {
      break;
    }
    // }

    LOG("------------------------------------------");

    for (auto bb : bbs) {
      LOG("BB " << bb->id() << ":");
      std::stringstream ss;
      // for (auto i : bb->liveIn()) {
      //   ss << i << ", ";
      // }
      // LOG(" IN: " << ss.str());
      // ss.str("");

      for (auto o : bb->liveOut()) {
        ss << o << ", ";
      }

      LOG(" OUT: " << ss.str());
      ss.str("");
    }

    LOG("------------------------------------------");
  }
}

} // namespace optimizer
