/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef WebsiteDataStore_h
#define WebsiteDataStore_h

#include "WebProcessLifetimeObserver.h"
#include "WebsiteDataTypes.h"
#include <WebCore/SessionID.h>
#include <functional>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class StorageManager;
class WebPageProxy;

class WebsiteDataStore : public RefCounted<WebsiteDataStore>, public WebProcessLifetimeObserver {
public:
    struct Configuration {
        String localStorageDirectory;
    };
    static RefPtr<WebsiteDataStore> createNonPersistent();
    static RefPtr<WebsiteDataStore> create(Configuration);
    virtual ~WebsiteDataStore();

    uint64_t identifier() const { return m_identifier; }

    bool isNonPersistent() const { return m_sessionID.isEphemeral(); }
    WebCore::SessionID sessionID() const { return m_sessionID; }

    static void cloneSessionData(WebPageProxy& sourcePage, WebPageProxy& newPage);

    void removeData(WebsiteDataTypes, std::chrono::system_clock::time_point modifiedSince, std::function<void ()> completionHandler);

    StorageManager* storageManager() { return m_storageManager.get(); }

private:
    explicit WebsiteDataStore(WebCore::SessionID);
    explicit WebsiteDataStore(Configuration);

    // WebProcessLifetimeObserver.
    virtual void webPageWasAdded(WebPageProxy&) override;
    virtual void webPageWasRemoved(WebPageProxy&) override;
    virtual void webProcessWillOpenConnection(WebProcessProxy&, IPC::Connection&) override;
    virtual void webPageWillOpenConnection(WebPageProxy&, IPC::Connection&) override;
    virtual void webPageDidCloseConnection(WebPageProxy&, IPC::Connection&) override;
    virtual void webProcessDidCloseConnection(WebProcessProxy&, IPC::Connection&) override;

    void platformInitialize();
    void platformDestroy();

    const uint64_t m_identifier;
    const WebCore::SessionID m_sessionID;

    const RefPtr<StorageManager> m_storageManager;
};

}

#endif // WebsiteDataStore_h
