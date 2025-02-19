/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef InbandTextTrack_h
#define InbandTextTrack_h

#if ENABLE(VIDEO_TRACK)

#include "InbandTextTrackPrivateClient.h"
#include "TextTrack.h"
#include "TextTrackCueGeneric.h"
#include <wtf/RefPtr.h>
#include <wtf/TypeCasts.h>

namespace WebCore {

class InbandTextTrack : public TextTrack, public InbandTextTrackPrivateClient {
public:
    static PassRefPtr<InbandTextTrack> create(ScriptExecutionContext*, TextTrackClient*, PassRefPtr<InbandTextTrackPrivate>);
    virtual ~InbandTextTrack();

    virtual bool isClosedCaptions() const override;
    virtual bool isSDH() const override;
    virtual bool containsOnlyForcedSubtitles() const override;
    virtual bool isMainProgramContent() const override;
    virtual bool isEasyToRead() const override;
    virtual void setMode(const AtomicString&) override;
    size_t inbandTrackIndex();

    virtual AtomicString inBandMetadataTrackDispatchType() const;

    void setPrivate(PassRefPtr<InbandTextTrackPrivate>);

protected:
    InbandTextTrack(ScriptExecutionContext*, TextTrackClient*, PassRefPtr<InbandTextTrackPrivate>);

    void setModeInternal(const AtomicString&);
    void updateKindFromPrivate();

    RefPtr<InbandTextTrackPrivate> m_private;

private:
    virtual bool isInband() const override final { return true; }
    virtual void idChanged(TrackPrivateBase*, const AtomicString&) override;
    virtual void labelChanged(TrackPrivateBase*, const AtomicString&) override;
    virtual void languageChanged(TrackPrivateBase*, const AtomicString&) override;
    virtual void willRemove(TrackPrivateBase*) override;

    virtual void addDataCue(InbandTextTrackPrivate*, const MediaTime&, const MediaTime&, const void*, unsigned) override { ASSERT_NOT_REACHED(); }

#if ENABLE(DATACUE_VALUE)
    virtual void addDataCue(InbandTextTrackPrivate*, const MediaTime&, const MediaTime&, PassRefPtr<SerializedPlatformRepresentation>, const String&) override { ASSERT_NOT_REACHED(); }
    virtual void updateDataCue(InbandTextTrackPrivate*, const MediaTime&, const MediaTime&, PassRefPtr<SerializedPlatformRepresentation>) override  { ASSERT_NOT_REACHED(); }
    virtual void removeDataCue(InbandTextTrackPrivate*, const MediaTime&, const MediaTime&, PassRefPtr<SerializedPlatformRepresentation>) override  { ASSERT_NOT_REACHED(); }
#endif

    virtual void addGenericCue(InbandTextTrackPrivate*, PassRefPtr<GenericCueData>) override { ASSERT_NOT_REACHED(); }
    virtual void updateGenericCue(InbandTextTrackPrivate*, GenericCueData*) override { ASSERT_NOT_REACHED(); }
    virtual void removeGenericCue(InbandTextTrackPrivate*, GenericCueData*) override { ASSERT_NOT_REACHED(); }

    virtual void parseWebVTTFileHeader(InbandTextTrackPrivate*, String) override { ASSERT_NOT_REACHED(); }
    virtual void parseWebVTTCueData(InbandTextTrackPrivate*, const char*, unsigned) override { ASSERT_NOT_REACHED(); }
    virtual void parseWebVTTCueData(InbandTextTrackPrivate*, const ISOWebVTTCue&) override { ASSERT_NOT_REACHED(); }

    virtual MediaTime startTimeVariance() const;

#if USE(PLATFORM_TEXT_TRACK_MENU)
    virtual InbandTextTrackPrivate* privateTrack() override { return m_private.get(); }
#endif
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::InbandTextTrack)
    static bool isType(const WebCore::TextTrack& track) { return track.isInband(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ENABLE(VIDEO_TRACK)

#endif // InbandTextTrack_h
