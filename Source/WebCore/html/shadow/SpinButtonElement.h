/*
 * Copyright (C) 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 
#ifndef SpinButtonElement_h
#define SpinButtonElement_h

#include "HTMLDivElement.h"
#include "PopupOpeningObserver.h"
#include "Timer.h"

namespace WebCore {

class SpinButtonElement final : public HTMLDivElement, public PopupOpeningObserver {
public:
    enum UpDownState {
        Indeterminate, // Hovered, but the event is not handled.
        Down,
        Up,
    };

    class SpinButtonOwner {
    public:
        virtual ~SpinButtonOwner() { }
        virtual void focusAndSelectSpinButtonOwner() = 0;
        virtual bool shouldSpinButtonRespondToMouseEvents() = 0;
        virtual bool shouldSpinButtonRespondToWheelEvents() = 0;
        virtual void spinButtonStepDown() = 0;
        virtual void spinButtonStepUp() = 0;
    };

    // The owner of SpinButtonElement must call removeSpinButtonOwner
    // because SpinButtonElement can be outlive SpinButtonOwner
    // implementation, e.g. during event handling.
    static PassRefPtr<SpinButtonElement> create(Document&, SpinButtonOwner&);
    UpDownState upDownState() const { return m_upDownState; }
    void releaseCapture();
    void removeSpinButtonOwner() { m_spinButtonOwner = 0; }

    void step(int amount);
    
    virtual bool willRespondToMouseMoveEvents() override;
    virtual bool willRespondToMouseClickEvents() override;

    void forwardEvent(Event*);

private:
    SpinButtonElement(Document&, SpinButtonOwner&);

    virtual void willDetachRenderers() override;
    virtual bool isSpinButtonElement() const override { return true; }
    virtual bool isDisabledFormControl() const override { return shadowHost() && shadowHost()->isDisabledFormControl(); }
    virtual bool matchesReadWritePseudoClass() const override;
    virtual void defaultEventHandler(Event*) override;
    virtual void willOpenPopup() override;
    void doStepAction(int);
    void startRepeatingTimer();
    void stopRepeatingTimer();
    void repeatingTimerFired();
    virtual void setHovered(bool = true) override;
    bool shouldRespondToMouseEvents();
    virtual bool isMouseFocusable() const override { return false; }

    SpinButtonOwner* m_spinButtonOwner;
    bool m_capturing;
    UpDownState m_upDownState;
    UpDownState m_pressStartingState;
    Timer m_repeatingTimer;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::SpinButtonElement)
    static bool isType(const WebCore::Element& element) { return element.isSpinButtonElement(); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::Element>(node) && isType(downcast<WebCore::Element>(node)); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // SpinButtonElement_h
