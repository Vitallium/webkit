/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderMathMLUnderOver_h
#define RenderMathMLUnderOver_h

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

namespace WebCore {
    
class RenderMathMLUnderOver final : public RenderMathMLBlock {
public:
    RenderMathMLUnderOver(Element&, Ref<RenderStyle>&&);
    
    virtual RenderMathMLOperator* unembellishedOperator() override;

    virtual int firstLineBaseline() const override;
    
protected:
    virtual void layout() override;

private:
    virtual bool isRenderMathMLUnderOver() const override { return true; }
    virtual const char* renderName() const override { return "RenderMathMLUnderOver"; }

    enum UnderOverType { Under, Over, UnderOver };
    UnderOverType m_kind;
};
    
}

#endif // ENABLE(MATHML)

#endif // RenderMathMLUnderOver_h
