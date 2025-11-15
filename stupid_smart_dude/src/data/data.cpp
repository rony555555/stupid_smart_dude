#include "data.h"
#include <WiFi.h>
#include <time.h>

DataManager Data;  // Global instance

// Constructor
DataManager::DataManager() {
    timeValid = false;
    currentTime = 0;
    lastMillis = 0;
    lastSyncEpoch = 0;
    resyncInterval = 0;  // disabled by default
    tzString = "";       // will be loaded from NVS
}

// Initialize Preferences and load existing schedules
void DataManager::begin() {
    prefs.begin("data_store", false);
    loadSchedules();
    loadTZ();

    // Start invalid. After Wi-Fi connects, call syncTimeFromNTP().
    timeValid = false;
    currentTime = 0;
    lastMillis = millis();
}

// ====== Core time getters/setters you already had ======
void DataManager::setTime(time_t t) {
    currentTime = t;
    timeValid = true;
    lastMillis = millis();  // reset delta baseline
}
time_t DataManager::getTime() { return currentTime; }

bool DataManager::isTimeValid() { return timeValid; }

void DataManager::invalidateTime() {
    timeValid = false;
    currentTime = 0;
}

// ====== Timezone persistence ======
void DataManager::saveTZ() {
    prefs.putString("tz", tzString);
}
void DataManager::loadTZ() {
    tzString = prefs.getString("tz", "");
}
void DataManager::configureTimezone(const String& tz) {
    tzString = tz;
    saveTZ();
}
const String& DataManager::getTimezone() const {
    return tzString;
}

// ====== NTP Sync ======
// Uses SNTP via configTime(). Requires Wi-Fi to be connected.
// Returns true if a valid epoch was obtained before timeout.
bool DataManager::syncTimeFromNTP(const char* server1, const char* server2, uint32_t timeoutMs) {
    if (WiFi.status() != WL_CONNECTED) {
        // We can’t sync without Wi-Fi. Keep previous state.
        return false;
    }

    // If TZ string is present, use configTzTime (sets TZ + starts SNTP).
    // Otherwise, fall back to UTC with configTime.
    if (tzString.length() > 0) {
        // Example TZ for Israel (you can set this once via configureTimezone):
        // "IST-2IDT,M3.5.0/2,M10.5.0/2"
        configTzTime(tzString.c_str(), server1, server2);
    } else {
        configTime(0, 0, server1, server2);  // UTC if no TZ configured
    }

    uint32_t start = millis();
    time_t nowEpoch = 0;
    const time_t saneThreshold = 978307200;  // 2001-01-01

    do {
        delay(200);
        time(&nowEpoch);
        if (nowEpoch >= saneThreshold) break;
    } while (millis() - start < timeoutMs);

    if (nowEpoch >= saneThreshold) {
        setTime(nowEpoch);
        lastSyncEpoch = nowEpoch;
        return true;
    }
    return false;
}

void DataManager::enableAutoResync(uint32_t intervalSeconds) {
    resyncInterval = intervalSeconds;  // 0 disables
}

time_t DataManager::getLastSyncEpoch() const {
    return lastSyncEpoch;
}

// ====== Keep time moving + auto re-sync ======
// Call this frequently (e.g., each loop()). It:
// 1) Advances currentTime by millis() delta if time is valid.
// 2) Tries an NTP resync if enabled and interval elapsed AND Wi-Fi is connected.
void DataManager::loopTime() {
    uint32_t nowMs = millis();

    if (timeValid) {
        // Accumulate elapsed milliseconds into our epoch counter.
        uint32_t deltaMs = nowMs - lastMillis;
        if (deltaMs > 0) {
            // Convert ms to seconds (lossy but fine) and carry small remainders via lastMillis baseline.
            currentTime += (deltaMs / 1000);
            // push baseline forward by the exact amount we consumed in seconds
            lastMillis += (deltaMs / 1000) * 1000;
        }

        // Auto re-sync check
        if (resyncInterval > 0) {
            if (currentTime >= lastSyncEpoch + (time_t)resyncInterval) {
                // Only attempt if Wi-Fi is up; if not, we’ll try next loop.
                if (WiFi.status() == WL_CONNECTED) {
                    // Non-blocking-ish: same routine, small timeout is OK because we run in loop.
                    syncTimeFromNTP();
                }
            }
        }
    } else {
        // Time is invalid; nothing to accumulate. We’ll become valid after a successful NTP sync.
        lastMillis = nowMs;  // keep baseline sane
    }
}

// --- Schedule serialization ---
String DataManager::serializeSchedule(const ScheduleItem& item) {
    // Format: "06:00,30,1"
    return item.timeOfDay + "," + String(item.duration) + "," + String(item.enabled ? 1 : 0);
}

ScheduleItem DataManager::deserializeSchedule(const String& data) {
    ScheduleItem item;
    int first = data.indexOf(',');
    int second = data.indexOf(',', first + 1);

    item.timeOfDay = data.substring(0, first);
    item.duration = data.substring(first + 1, second).toInt();
    item.enabled = (data.substring(second + 1).toInt() == 1);
    return item;
}

// --- Save and Load ---
void DataManager::saveSchedules() {
    prefs.clear();  // Clear old data
    prefs.putInt("count", schedules.size());
    for (int i = 0; i < schedules.size(); i++) {
        String key = makeScheduleKey(i);
        prefs.putString(key.c_str(), serializeSchedule(schedules[i]));
    }
}

void DataManager::loadSchedules() {
    schedules.clear();
    int count = prefs.getInt("count", 0);
    for (int i = 0; i < count; i++) {
        String key = makeScheduleKey(i);
        String saved = prefs.getString(key.c_str(), "");
        if (saved.length() > 0)
            schedules.push_back(deserializeSchedule(saved));
    }
}

// --- Schedule management ---
static String normalizeTimeString(const String& t) {
    int h = 0, m = 0;
    if (sscanf(t.c_str(), "%d:%d", &h, &m) == 2) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
        return String(buf);
    }
    // fallback if parse fails
    return t;
}

void DataManager::addSchedule(String timeOfDay, uint16_t duration, bool enabled) {
    ScheduleItem item = {normalizeTimeString(timeOfDay), duration, enabled};
    schedules.push_back(item);
    saveSchedules();
}

void DataManager::updateSchedule(int index, String timeOfDay, uint16_t duration, bool enabled) {
    if (!isValidScheduleIndex(index)) return;
    schedules[index] = {normalizeTimeString(timeOfDay), duration, enabled};
    saveSchedules();
}

void DataManager::removeSchedule(int index) {
    if (!isValidScheduleIndex(index)) return;
    schedules.erase(schedules.begin() + index);
    saveSchedules();
}

ScheduleItem DataManager::getSchedule(int index) {
    if (!isValidScheduleIndex(index)) {
        return {"00:00", 0, false};
    }
    return schedules[index];
}

int DataManager::getScheduleCount() {
    return schedules.size();
}

bool DataManager::isValidScheduleIndex(int index) const {
    return index >= 0 && index < (int)schedules.size();
}

String DataManager::makeScheduleKey(int index) const {
    // All schedule entries use keys "s0", "s1", "s2", ...
    return "s" + String(index);
}

void DataManager::printAllSchedules() {
    Serial.println("=== Saved Schedules ===");
    if (schedules.empty()) {
        Serial.println("No schedules stored.");
        return;
    }

    for (int i = 0; i < schedules.size(); i++) {
        ScheduleItem s = schedules[i];
        Serial.printf("[%d] Time: %s | Duration: %d | Enabled: %s\n",
                      i,
                      s.timeOfDay.c_str(),
                      s.duration,
                      s.enabled ? "true" : "false");
    }
    Serial.println("========================\n");
}
