#include "timeManager.h"
#include <WiFi.h>  // for WiFi.status()
#include <time.h>  // for time(), configTime, configTzTime

TimeManager Time;

TimeManager::TimeManager()
  : timeValid(false),
    lastSyncEpoch(0),
    resyncInterval(0),
    tzString("") {
}

void TimeManager::begin() {
    prefs.begin("time_store", false);
    loadTZ();

    // Note: we don't call configTzTime here because we only want
    // to start SNTP when we actually sync. TZ will be applied then.
    timeValid = false;
    lastSyncEpoch = 0;
}

// ========== TZ persistence ==========
void TimeManager::saveTZ() {
    prefs.putString("tz", tzString);
}

void TimeManager::loadTZ() {
    tzString = prefs.getString("tz", "");
}

void TimeManager::configureTimezone(const String& tz) {
    tzString = tz;
    saveTZ();
    // configTzTime will apply TZ on the next sync
}

const String& TimeManager::getTimezone() const {
    return tzString;
}

// ========== Time validity ==========
bool TimeManager::isTimeValid() const {
    return timeValid;
}

void TimeManager::invalidateTime() {
    timeValid = false;
}

// ========== Get current time ==========
time_t TimeManager::getTime() const {
    if (!timeValid) {
        return 0;
    }
    time_t nowEpoch = 0;
    time(&nowEpoch);  // read from ESP32 RTC (set by SNTP)
    return nowEpoch;
}

// ========== NTP sync ==========
bool TimeManager::syncTimeFromNTP(const char* server1,
                                  const char* server2,
                                  uint32_t timeoutMs) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    // Start SNTP with TZ if we have one
    if (tzString.length() > 0) {
        configTzTime(tzString.c_str(), server1, server2);
    } else {
        configTime(0, 0, server1, server2); // UTC
    }

    uint32_t start = millis();
    time_t nowEpoch = 0;
    const time_t saneThreshold = 978307200; // 2001-01-01

    // Wait for SNTP to set a sane time
    do {
        delay(200);
        time(&nowEpoch);
        if (nowEpoch >= saneThreshold) break;
    } while (millis() - start < timeoutMs);

    if (nowEpoch >= saneThreshold) {
        timeValid = true;
        lastSyncEpoch = nowEpoch;
        return true;
    }

    // No sane time received
    return false;
}

// ========== Auto-resync ==========
void TimeManager::enableAutoResync(uint32_t intervalSeconds) {
    resyncInterval = intervalSeconds; // 0 disables auto-resync
}

time_t TimeManager::getLastSyncEpoch() const {
    return lastSyncEpoch;
}

void TimeManager::loop() {
    if (!timeValid) {
        return;  // no point resyncing "automatically" if we never had valid time
    }
    if (resyncInterval == 0) {
        return;  // auto-resync disabled
    }
    if (WiFi.status() != WL_CONNECTED) {
        return;  // can't resync without WiFi
    }

    time_t nowEpoch = 0;
    time(&nowEpoch);
    if (nowEpoch == 0) {
        return;
    }

    if (nowEpoch >= lastSyncEpoch + (time_t)resyncInterval) {
        // Try a new sync. If it fails, we keep old timeValid = true, old lastSyncEpoch
        if (syncTimeFromNTP()) {
            // syncTimeFromNTP will set lastSyncEpoch on success
        }
    }
}
