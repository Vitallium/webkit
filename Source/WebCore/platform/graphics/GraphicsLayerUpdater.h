/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GraphicsLayerUpdater_h
#define GraphicsLayerUpdater_h

#include "DisplayRefreshMonitorClient.h"
#include "PlatformScreen.h"

namespace WebCore {

class GraphicsLayerUpdater;

class GraphicsLayerUpdaterClient {
public:
    virtual ~GraphicsLayerUpdaterClient() { }
    virtual void flushLayersSoon(GraphicsLayerUpdater*) = 0;
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual PassRefPtr<DisplayRefreshMonitor> createDisplayRefreshMonitor(PlatformDisplayID) const = 0;
#endif
};

class GraphicsLayerUpdater
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    : public DisplayRefreshMonitorClient
#endif
{
public:
    GraphicsLayerUpdater(GraphicsLayerUpdaterClient*, PlatformDisplayID);
    virtual ~GraphicsLayerUpdater();

    void scheduleUpdate();
    void screenDidChange(PlatformDisplayID);

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual PassRefPtr<DisplayRefreshMonitor> createDisplayRefreshMonitor(PlatformDisplayID) const override;
#endif

private:
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual void displayRefreshFired(double timestamp) override;
#endif

    GraphicsLayerUpdaterClient* m_client;
    bool m_scheduled;
};

} // namespace WebCore

#endif // GraphicsLayerUpdater_h
