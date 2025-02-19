/*
 * Copyright (C) 2008, 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(JIT)
#if USE(JSVALUE32_64)
#include "JIT.h"

#include "Arguments.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "JITInlines.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSCInlines.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include "SetupVarargsFrame.h"
#include "StackAlignment.h"
#include <wtf/StringPrintStream.h>


namespace JSC {

void JIT::emitPutCallResult(Instruction* instruction)
{
    int dst = instruction[1].u.operand;
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
}

void JIT::emit_op_ret(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    emitLoad(dst, regT1, regT0);

    checkStackPointerAlignment();
    emitFunctionEpilogue();
    ret();
}

void JIT::emitSlow_op_call(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emitSlow_op_call_eval(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_eval, currentInstruction, iter, m_callLinkInfoIndex);
}
 
void JIT::emitSlow_op_call_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_varargs, currentInstruction, iter, m_callLinkInfoIndex++);
}
    
void JIT::emitSlow_op_construct_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_construct_varargs, currentInstruction, iter, m_callLinkInfoIndex++);
}
    
void JIT::emitSlow_op_construct(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_construct, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emit_op_call(Instruction* currentInstruction)
{
    compileOpCall(op_call, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_call_eval(Instruction* currentInstruction)
{
    compileOpCall(op_call_eval, currentInstruction, m_callLinkInfoIndex);
}

void JIT::emit_op_call_varargs(Instruction* currentInstruction)
{
    compileOpCall(op_call_varargs, currentInstruction, m_callLinkInfoIndex++);
}
    
void JIT::emit_op_construct_varargs(Instruction* currentInstruction)
{
    compileOpCall(op_construct_varargs, currentInstruction, m_callLinkInfoIndex++);
}
    
void JIT::emit_op_construct(Instruction* currentInstruction)
{
    compileOpCall(op_construct, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::compileSetupVarargsFrame(Instruction* instruction, CallLinkInfo* info)
{
    int thisValue = instruction[3].u.operand;
    int arguments = instruction[4].u.operand;
    int firstFreeRegister = instruction[5].u.operand;
    int firstVarArgOffset = instruction[6].u.operand;

    JumpList slowCase;
    JumpList end;
    bool canOptimize = m_codeBlock->usesArguments()
        && VirtualRegister(arguments) == m_codeBlock->argumentsRegister()
        && !m_codeBlock->symbolTable()->slowArguments();

    if (canOptimize) {
        emitLoadTag(arguments, regT1);
        slowCase.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::EmptyValueTag)));
        
        move(TrustedImm32(-firstFreeRegister), regT1);
        emitSetupVarargsFrameFastCase(*this, regT1, regT0, regT1, regT2, firstVarArgOffset, slowCase);
        end.append(jump());
        slowCase.link(this);
    }

    emitLoad(arguments, regT1, regT0);
    callOperation(operationSizeFrameForVarargs, regT1, regT0, -firstFreeRegister, firstVarArgOffset);
    move(TrustedImm32(-firstFreeRegister), regT1);
    emitSetVarargsFrame(*this, returnValueGPR, false, regT1, regT1);
    addPtr(TrustedImm32(-(sizeof(CallerFrameAndPC) + WTF::roundUpToMultipleOf(stackAlignmentBytes(), 6 * sizeof(void*)))), regT1, stackPointerRegister);
    emitLoad(arguments, regT2, regT4);
    callOperation(operationSetupVarargsFrame, regT1, regT2, regT4, firstVarArgOffset, regT0);
    move(returnValueGPR, regT1);

    if (canOptimize)
        end.link(this);

    // Profile the argument count.
    load32(Address(regT1, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + PayloadOffset), regT2);
    load8(&info->maxNumArguments, regT0);
    Jump notBiggest = branch32(Above, regT0, regT2);
    Jump notSaturated = branch32(BelowOrEqual, regT2, TrustedImm32(255));
    move(TrustedImm32(255), regT2);
    notSaturated.link(this);
    store8(regT2, &info->maxNumArguments);
    notBiggest.link(this);
    
    // Initialize 'this'.
    emitLoad(thisValue, regT2, regT0);
    store32(regT0, Address(regT1, PayloadOffset + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
    store32(regT2, Address(regT1, TagOffset + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
    
    addPtr(TrustedImm32(sizeof(CallerFrameAndPC)), regT1, stackPointerRegister);
}

void JIT::compileCallEval(Instruction* instruction)
{
    addPtr(TrustedImm32(-static_cast<ptrdiff_t>(sizeof(CallerFrameAndPC))), stackPointerRegister, regT1);
    storePtr(callFrameRegister, Address(regT1, CallFrame::callerFrameOffset()));

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);

    callOperation(operationCallEval, regT1);

    addSlowCase(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));

    sampleCodeBlock(m_codeBlock);
    
    emitPutCallResult(instruction);
}

void JIT::compileCallEvalSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    int registerOffset = -instruction[4].u.operand;

    addPtr(TrustedImm32(registerOffset * sizeof(Register) + sizeof(CallerFrameAndPC)), callFrameRegister, stackPointerRegister);

    loadPtr(Address(stackPointerRegister, sizeof(Register) * JSStack::Callee - sizeof(CallerFrameAndPC)), regT0);
    loadPtr(Address(stackPointerRegister, sizeof(Register) * JSStack::Callee - sizeof(CallerFrameAndPC)), regT1);
    move(TrustedImmPtr(&CallLinkInfo::dummy()), regT2);

    emitLoad(JSStack::Callee, regT1, regT0);
    emitNakedCall(m_vm->getCTIStub(virtualCallThunkGenerator).code());
    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    
    emitPutCallResult(instruction);
}

void JIT::compileOpCall(OpcodeID opcodeID, Instruction* instruction, unsigned callLinkInfoIndex)
{
    CallLinkInfo* info = m_codeBlock->addCallLinkInfo();
    int callee = instruction[2].u.operand;

    /* Caller always:
        - Updates callFrameRegister to callee callFrame.
        - Initializes ArgumentCount; CallerFrame; Callee.

       For a JS call:
        - Callee initializes ReturnPC; CodeBlock.
        - Callee restores callFrameRegister before return.

       For a non-JS call:
        - Caller initializes ReturnPC; CodeBlock.
        - Caller restores callFrameRegister after return.
    */
    
    if (opcodeID == op_call_varargs || opcodeID == op_construct_varargs)
        compileSetupVarargsFrame(instruction, info);
    else {
        int argCount = instruction[3].u.operand;
        int registerOffset = -instruction[4].u.operand;
        
        if (opcodeID == op_call && shouldEmitProfiling()) {
            emitLoad(registerOffset + CallFrame::argumentOffsetIncludingThis(0), regT0, regT1);
            Jump done = branch32(NotEqual, regT0, TrustedImm32(JSValue::CellTag));
            loadPtr(Address(regT1, JSCell::structureIDOffset()), regT1);
            storePtr(regT1, instruction[OPCODE_LENGTH(op_call) - 2].u.arrayProfile->addressOfLastSeenStructureID());
            done.link(this);
        }
    
        addPtr(TrustedImm32(registerOffset * sizeof(Register) + sizeof(CallerFrameAndPC)), callFrameRegister, stackPointerRegister);

        store32(TrustedImm32(argCount), Address(stackPointerRegister, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + PayloadOffset - sizeof(CallerFrameAndPC)));
    } // SP holds newCallFrame + sizeof(CallerFrameAndPC), with ArgumentCount initialized.
    
    uint32_t locationBits = CallFrame::Location::encodeAsBytecodeInstruction(instruction);
    store32(TrustedImm32(locationBits), tagFor(JSStack::ArgumentCount, callFrameRegister));
    emitLoad(callee, regT1, regT0); // regT1, regT0 holds callee.

    store32(regT0, Address(stackPointerRegister, JSStack::Callee * static_cast<int>(sizeof(Register)) + PayloadOffset - sizeof(CallerFrameAndPC)));
    store32(regT1, Address(stackPointerRegister, JSStack::Callee * static_cast<int>(sizeof(Register)) + TagOffset - sizeof(CallerFrameAndPC)));

    if (opcodeID == op_call_eval) {
        compileCallEval(instruction);
        return;
    }

    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));

    DataLabelPtr addressOfLinkedFunctionCheck;
    Jump slowCase = branchPtrWithPatch(NotEqual, regT0, addressOfLinkedFunctionCheck, TrustedImmPtr(0));

    addSlowCase(slowCase);

    ASSERT(m_callCompilationInfo.size() == callLinkInfoIndex);
    info->callType = CallLinkInfo::callTypeFor(opcodeID);
    info->codeOrigin = CodeOrigin(m_bytecodeOffset);
    info->calleeGPR = regT0;
    m_callCompilationInfo.append(CallCompilationInfo());
    m_callCompilationInfo[callLinkInfoIndex].hotPathBegin = addressOfLinkedFunctionCheck;
    m_callCompilationInfo[callLinkInfoIndex].callLinkInfo = info;

    checkStackPointerAlignment();
    m_callCompilationInfo[callLinkInfoIndex].hotPathOther = emitNakedCall();

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    emitPutCallResult(instruction);
}

void JIT::compileOpCallSlowCase(OpcodeID opcodeID, Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter, unsigned callLinkInfoIndex)
{
    if (opcodeID == op_call_eval) {
        compileCallEvalSlowCase(instruction, iter);
        return;
    }

    linkSlowCase(iter);
    linkSlowCase(iter);

    ThunkGenerator generator = linkThunkGeneratorFor(
        (opcodeID == op_construct || opcodeID == op_construct_varargs) ? CodeForConstruct : CodeForCall,
        RegisterPreservationNotRequired);
    
    move(TrustedImmPtr(m_callCompilationInfo[callLinkInfoIndex].callLinkInfo), regT2);
    m_callCompilationInfo[callLinkInfoIndex].callReturnLocation = emitNakedCall(m_vm->getCTIStub(generator).code());

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    emitPutCallResult(instruction);
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
