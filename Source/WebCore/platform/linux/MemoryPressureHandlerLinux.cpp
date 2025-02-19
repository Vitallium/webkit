/*
 * Copyright (C) 2011, 2012 Apple Inc. All Rights Reserved.
 * Copyright (C) 2014 Raspberry Pi Foundation. All Rights Reserved.
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
#include "MemoryPressureHandler.h"

#if OS(LINUX)

#include "Logging.h"

#include <fcntl.h>
#include <malloc.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Disable memory event reception for a minimum of s_minimumHoldOffTime
// seconds after receiving an event. Don't let events fire any sooner than
// s_holdOffMultiplier times the last cleanup processing time. Effectively
// this is 1 / s_holdOffMultiplier percent of the time.
// These value seems reasonable and testing verifies that it throttles frequent
// low memory events, greatly reducing CPU usage.
static const unsigned s_minimumHoldOffTime = 5;
static const unsigned s_holdOffMultiplier = 20;

static const char* s_cgroupMemoryPressureLevel = "/sys/fs/cgroup/memory/memory.pressure_level";
static const char* s_cgroupEventControl = "/sys/fs/cgroup/memory/cgroup.event_control";
static const char* s_processStatus = "/proc/self/status";

static inline String nextToken(FILE* file)
{
    if (!file)
        return String();

    static const unsigned bufferSize = 128;
    char buffer[bufferSize] = {0, };
    unsigned index = 0;
    while (index < bufferSize) {
        char ch = fgetc(file);
        if (ch == EOF || (isASCIISpace(ch) && index)) // Break on non-initial ASCII space.
            break;
        if (!isASCIISpace(ch)) {
            buffer[index] = ch;
            index++;
        }
    }

    return String(buffer);
}

void MemoryPressureHandler::waitForMemoryPressureEvent(void*)
{
    ASSERT(!isMainThread());
    int eventFD = MemoryPressureHandler::singleton().m_eventFD;
    if (!eventFD) {
        LOG(MemoryPressure, "Invalidate eventfd.");
        return;
    }

    uint64_t buffer;
    if (read(eventFD, &buffer, sizeof(buffer)) <= 0) {
        LOG(MemoryPressure, "Failed to read eventfd.");
        return;
    }

    // FIXME: Current memcg does not provide any way for users to know how serious the memory pressure is.
    // So we assume all notifications from memcg are critical for now. If memcg had better inferfaces
    // to get a detailed memory pressure level in the future, we should update here accordingly.
    bool critical = true;
    if (ReliefLogger::loggingEnabled())
        LOG(MemoryPressure, "Got memory pressure notification (%s)", critical ? "critical" : "non-critical");

    MemoryPressureHandler::singleton().setUnderMemoryPressure(critical);
    callOnMainThread([critical] {
        MemoryPressureHandler::singleton().respondToMemoryPressure(critical);
    });
}

inline void MemoryPressureHandler::logErrorAndCloseFDs(const char* log)
{
    if (log)
        LOG(MemoryPressure, "%s, error : %m", log);

    if (m_eventFD) {
        close(m_eventFD);
        m_eventFD = 0;
    }
    if (m_pressureLevelFD) {
        close(m_pressureLevelFD);
        m_pressureLevelFD = 0;
    }
}

void MemoryPressureHandler::install()
{
    if (m_installed)
        return;

    m_eventFD = eventfd(0, EFD_CLOEXEC);
    if (m_eventFD == -1) {
        LOG(MemoryPressure, "eventfd() failed: %m");
        return;
    }

    m_pressureLevelFD = open(s_cgroupMemoryPressureLevel, O_CLOEXEC | O_RDONLY);
    if (m_pressureLevelFD == -1) {
        logErrorAndCloseFDs("Failed to open memory.pressure_level");
        return;
    }

    int fd = open(s_cgroupEventControl, O_CLOEXEC | O_WRONLY);
    if (fd == -1) {
        logErrorAndCloseFDs("Failed to open cgroup.event_control");
        return;
    }

    char line[128] = {0, };
    if (snprintf(line, sizeof(line), "%d %d low", m_eventFD, m_pressureLevelFD) < 0
        || write(fd, line, strlen(line) + 1) < 0) {
        logErrorAndCloseFDs("Failed to write cgroup.event_control");
        close(fd);
        return;
    }
    close(fd);

    m_threadID = createThread(waitForMemoryPressureEvent, this, "WebCore: MemoryPressureHandler");
    if (!m_threadID) {
        logErrorAndCloseFDs("Failed to create a thread for MemoryPressureHandler");
        return;
    }

    if (ReliefLogger::loggingEnabled() && isUnderMemoryPressure())
        LOG(MemoryPressure, "System is no longer under memory pressure.");

    setUnderMemoryPressure(false);
    m_installed = true;
}

void MemoryPressureHandler::uninstall()
{
    if (!m_installed)
        return;

    if (m_threadID) {
        detachThread(m_threadID);
        m_threadID = 0;
    }

    logErrorAndCloseFDs(nullptr);
    m_installed = false;
}

void MemoryPressureHandler::holdOffTimerFired()
{
    install();
}

void MemoryPressureHandler::holdOff(unsigned seconds)
{
    m_holdOffTimer.startOneShot(seconds);
}

void MemoryPressureHandler::respondToMemoryPressure(bool critical)
{
    uninstall();

    double startTime = monotonicallyIncreasingTime();
    m_lowMemoryHandler(critical);
    unsigned holdOffTime = (monotonicallyIncreasingTime() - startTime) * s_holdOffMultiplier;
    holdOff(std::max(holdOffTime, s_minimumHoldOffTime));
}

void MemoryPressureHandler::platformReleaseMemory(bool)
{
    ReliefLogger log("Run malloc_trim");
    malloc_trim(0);
}

void MemoryPressureHandler::ReliefLogger::platformLog()
{
    size_t currentMemory = platformMemoryUsage();
    if (currentMemory == static_cast<size_t>(-1) || m_initialMemory == static_cast<size_t>(-1)) {
        LOG(MemoryPressure, "%s (Unable to get dirty memory information for process)", m_logString);
        return;
    }

    ssize_t memoryDiff = currentMemory - m_initialMemory;
    if (memoryDiff < 0)
        LOG(MemoryPressure, "Pressure relief: %s: -dirty %lu bytes (from %lu to %lu)", m_logString, static_cast<unsigned long>(memoryDiff * -1), static_cast<unsigned long>(m_initialMemory), static_cast<unsigned long>(currentMemory));
    else if (memoryDiff > 0)
        LOG(MemoryPressure, "Pressure relief: %s: +dirty %lu bytes (from %lu to %lu)", m_logString, static_cast<unsigned long>(memoryDiff), static_cast<unsigned long>(m_initialMemory), static_cast<unsigned long>(currentMemory));
    else
        LOG(MemoryPressure, "Pressure relief: %s: =dirty (at %lu bytes)", m_logString, static_cast<unsigned long>(currentMemory));
}

size_t MemoryPressureHandler::ReliefLogger::platformMemoryUsage()
{
    FILE* file = fopen(s_processStatus, "r");
    if (!file)
        return static_cast<size_t>(-1);

    size_t vmSize = static_cast<size_t>(-1); // KB
    String token = nextToken(file);
    while (!token.isEmpty()) {
        if (token == "VmSize:") {
            vmSize = nextToken(file).toInt() * KB;
            break;
        }
        token = nextToken(file);
    }
    fclose(file);

    return vmSize;
}

} // namespace WebCore

#endif // OS(LINUX)
