//
// Created by machiry on 12/5/16.
//

#include <set>
#include <llvm/IR/CFG.h>
#include "InstructionUtils.h"

using namespace llvm;

namespace DRCHECKER {


    bool InstructionUtils::isPointerInstruction(Instruction *I) {
        bool retVal = false;
        LoadInst *LI = dyn_cast<LoadInst>(I);
        if(LI) {
            retVal = true;
        }
        StoreInst *SI = dyn_cast<StoreInst>(I);
        if(SI) {
            retVal = true;
        }
        VAArgInst *VAAI = dyn_cast<VAArgInst>(I);
        if(VAAI) {
            retVal = true;
        }
        return retVal;
    }

    unsigned InstructionUtils::getLineNumber(Instruction &I)
    {

        const DebugLoc &currDC = I.getDebugLoc();
        if(currDC) {
            return currDC.getLine();
        }
        return -1;
    }

    std::string InstructionUtils::getInstructionName(Instruction *I) {
        if(I->hasName()) {
            return I->getName().str();
        } else {
            return "No Name";
        }
    }

    std::string InstructionUtils::getValueName(Value *v) {
        if(v->hasName()) {
            return v->getName().str();
        } else {
            return "No Name";
        }
    }

    std::string InstructionUtils::escapeJsonString(const std::string& input) {
        std::ostringstream ss;
        for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
            //C++98/03:
            for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
                switch (*iter) {
                    case '\\':
                        ss << "\\\\";
                        break;
                    case '"':
                        ss << "\\\"";
                        break;
                    case '/':
                        ss << "\\/";
                        break;
                    case '\b':
                        ss << "\\b";
                        break;
                    case '\f':
                        ss << "\\f";
                        break;
                    case '\n':
                        ss << "\\n";
                        break;
                    case '\r':
                        ss << "\\r";
                        break;
                    case '\t':
                        ss << "\\t";
                        break;
                    default:
                        ss << *iter;
                        break;
                }
            }
            return ss.str();
        }
    }

    std::string InstructionUtils::escapeValueString(Value *currInstr) {
        std::string str;
        llvm::raw_string_ostream rso(str);
        currInstr->print(rso);
        return InstructionUtils::escapeJsonString(rso.str());
    }

    DILocation* getRecursiveDILoc(Instruction *currInst, DILocation *corrL, std::set<BasicBlock*> &visitedBBs) {
        DILocation *currIL = currInst->getDebugLoc().get();
        if(corrL == nullptr) {
            return currIL;
        }
        if(currIL != nullptr && currIL->getFilename().equals(corrL->getFilename())) {
            return currIL;
        }

        BasicBlock *currBB = currInst->getParent();
        if(visitedBBs.find(currBB) != visitedBBs.end()) {
            return nullptr;
        }
        for(auto &iu :currBB->getInstList()) {
            Instruction *currIterI = &iu;
            DILocation *currIteDL = currIterI->getDebugLoc();
            if(currIteDL != nullptr && currIteDL->getFilename().equals(corrL->getFilename())) {
                return currIteDL;
            }
            if(currIterI == currInst) {
                break;
            }
        }

        visitedBBs.insert(currBB);

        for (auto it = pred_begin(currBB), et = pred_end(currBB); it != et; ++it) {
            BasicBlock* predecessor = *it;
            DILocation *currBBLoc = getRecursiveDILoc(predecessor->getFirstNonPHIOrDbg(), corrL, visitedBBs);
            if(currBBLoc != nullptr) {
                return currBBLoc;
            }
        }
        return nullptr;
    }


    DILocation* InstructionUtils::getCorrectInstrLocation(Instruction *I) {
        DILocation *instrLoc = I->getDebugLoc().get();
        BasicBlock *firstBB = &(I->getFunction()->getEntryBlock());
        Instruction *firstInstr = firstBB->getFirstNonPHIOrDbg();

        DILocation *firstIL = firstInstr->getDebugLoc().get();
        std::set<BasicBlock*> visitedBBs;

        if(instrLoc != nullptr && instrLoc->getFilename().endswith(".c")) {
            return instrLoc;
        }

        if(instrLoc == nullptr || (firstIL != nullptr && !instrLoc->getFilename().equals(firstIL->getFilename()))) {
            // OK, the instruction is from the inlined function.
            visitedBBs.clear();
            DILocation *actualLoc = getRecursiveDILoc(I, firstIL, visitedBBs);
            if(actualLoc != nullptr) {
                return actualLoc;
            }
        }

        return instrLoc;
    }

    int InstructionUtils::getInstrLineNumber(Instruction *I) {
        DILocation *targetLoc = InstructionUtils::getCorrectInstrLocation(I);
        if(targetLoc != nullptr) {
            return targetLoc->getLine();
        }
        return -1;
    }


}
