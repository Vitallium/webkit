/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebPageGroupData.h"

#include "WebCoreArgumentCoders.h"

namespace WebKit {

void WebPageGroupData::encode(IPC::ArgumentEncoder& encoder) const
{
    encoder << identifier;
    encoder << pageGroupID;
    encoder << visibleToInjectedBundle;
    encoder << visibleToHistoryClient;
    encoder << userStyleSheets;
    encoder << userScripts;
#if ENABLE(CONTENT_EXTENSIONS)
    encoder << userContentFilters;
#endif
}

bool WebPageGroupData::decode(IPC::ArgumentDecoder& decoder, WebPageGroupData& data)
{
    if (!decoder.decode(data.identifier))
        return false;
    if (!decoder.decode(data.pageGroupID))
        return false;
    if (!decoder.decode(data.visibleToInjectedBundle))
        return false;
    if (!decoder.decode(data.visibleToHistoryClient))
        return false;
    if (!decoder.decode(data.userStyleSheets))
        return false;
    if (!decoder.decode(data.userScripts))
        return false;
#if ENABLE(CONTENT_EXTENSIONS)
    if (!decoder.decode(data.userContentFilters))
        return false;
#endif
    return true;
}

} // namespace WebKit
