//===--- DebugUtils.cpp ---------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/SIL/DebugUtils.h"
#include "swift/Basic/STLExtras.h"
#include "swift/SIL/SILArgument.h"
#include "swift/SIL/SILInstruction.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallPtrSet.h"

using namespace swift;

bool swift::hasNonTrivialNonDebugTransitiveUsers(
    PointerUnion<SILInstruction *, SILArgument *> V) {
  llvm::SmallVector<SILInstruction *, 8> Worklist;
  llvm::SmallPtrSet<SILInstruction *, 8> SeenInsts;

  // Initialize our worklist.
  if (auto *A = V.dyn_cast<SILArgument *>()) {
    for (Operand *Op : getNonDebugUses(SILValue(A))) {
      auto *User = Op->getUser();
      if (!SeenInsts.insert(User).second)
        continue;
      Worklist.push_back(User);
    }
  } else {
    auto *I = V.get<SILInstruction *>();
    SeenInsts.insert(I);
    Worklist.push_back(I);
  }

  while (!Worklist.empty()) {
    SILInstruction *U = Worklist.pop_back_val();
    assert(SeenInsts.count(U) &&
           "Worklist should only contain seen instructions?!");

    // If U is a terminator inst, return false.
    if (isa<TermInst>(U))
      return true;

    // If U has side effects...
    if (U->mayHaveSideEffects())
      return true;

    // Otherwise add all non-debug uses of I that we have not seen yet to the
    // worklist.
    for (SILValue Result : U->getResults()) {
      for (Operand *I : getNonDebugUses(Result)) {
        auto *User = I->getUser();
        if (!SeenInsts.insert(User).second)
          continue;
        Worklist.push_back(User);
      }
    }
  }
  return false;
}
