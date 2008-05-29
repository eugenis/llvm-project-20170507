//===- DCE.cpp - Code to perform dead code elimination --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Aggressive Dead Code Elimination pass.  This pass
// optimistically assumes that all instructions are dead until proven otherwise,
// allowing it to eliminate dead computations that other DCE passes do not 
// catch, particularly involving loop computations.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "adce"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallPtrSet.h"

using namespace llvm;

STATISTIC(NumRemoved, "Number of instructions removed");

namespace {
  struct VISIBILITY_HIDDEN ADCE : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    ADCE() : FunctionPass((intptr_t)&ID) {}
    
    virtual bool runOnFunction(Function& F);
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesCFG();
    }
    
  };
}

char ADCE::ID = 0;
static RegisterPass<ADCE> X("adce", "Aggressive Dead Code Elimination");

bool ADCE::runOnFunction(Function& F) {
  SmallPtrSet<Instruction*, 32> alive;
  std::vector<Instruction*> worklist;
  
  // Collect the set of "root" instructions that are known live.
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    if (isa<TerminatorInst>(I.getInstructionIterator()) ||
        I->mayWriteToMemory()) {
      alive.insert(I.getInstructionIterator());
      worklist.push_back(I.getInstructionIterator());
    }
  
  // Propagate liveness backwards to operands.
  while (!worklist.empty()) {
    Instruction* curr = worklist.back();
    worklist.pop_back();
    
    for (Instruction::op_iterator OI = curr->op_begin(), OE = curr->op_end();
         OI != OE; ++OI)
      if (Instruction* Inst = dyn_cast<Instruction>(OI))
        if (alive.insert(Inst))
          worklist.push_back(Inst);
  }
  
  // The inverse of the live set is the dead set.  These are those instructions
  // which have no side effects and do not influence the control flow or return
  // value of the function, and may therefore be deleted safely.
  SmallPtrSet<Instruction*, 32> dead;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
    if (!alive.count(I.getInstructionIterator())) {
      dead.insert(I.getInstructionIterator());
      I->dropAllReferences();
    }
  
  for (SmallPtrSet<Instruction*, 32>::iterator I = dead.begin(),
       E = dead.end(); I != E; ++I) {
    NumRemoved++;
    (*I)->eraseFromParent();
  }
    
  return !dead.empty();
}

FunctionPass *llvm::createAggressiveDCEPass() {
  return new ADCE();
}
