#ifndef DATA_H
#define DATA_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

// Structure for a single schedule entry
struct ScheduleItem {
    String timeOfDay;   // e.g. "06:00"
    uint16_t duration;  // in minutes
    bool enabled;       // true or false
};

class DataManager {
   private:
    Preferences prefs;                    // For saving to flash
    std::vector<ScheduleItem> schedules;  // In-memory list of schedules

    // --- Time state ---
    bool timeValid;           // true once we’ve synced at least once
    time_t currentTime;       // tracked Unix epoch (seconds, UTC)
    uint32_t lastMillis;      // for accumulating elapsed time
    time_t lastSyncEpoch;     // when we last successfully synced (epoch)
    uint32_t resyncInterval;  // seconds between automatic re-syncs (0=disabled)

    // --- Timezone ---
    String tzString;  // POSIX TZ string (e.g., "IST-2IDT,M3.5.0/2,M10.5.0/2")

    // Helper functions
    String serializeSchedule(const ScheduleItem& item);
    ScheduleItem deserializeSchedule(const String& data);

    void saveSchedules();
    void loadSchedules();

   public:
    DataManager();

    // --- Time Handling ---
    void setTime(time_t t);
    time_t getTime();
    bool isTimeValid();
    void invalidateTime();

    // Time management API ---
    void saveTZ();
    void loadTZ();
    void configureTimezone(const String& tz);  // set TZ and persist
    const String& getTimezone() const;         // read TZ string
    bool syncTimeFromNTP(const char* server1 = "pool.ntp.org",
                         const char* server2 = "time.nist.gov",
                         uint32_t timeoutMs = 12000);  // fetch epoch via SNTP
    void loopTime();                                   // call often; keeps time & auto-resync
    void enableAutoResync(uint32_t intervalSeconds);   // 0 disables
    time_t getLastSyncEpoch() const;                   // debug/telemetry

    // --- Schedule Handling ---
    void addSchedule(String timeOfDay, uint16_t duration, bool enabled);
    void updateSchedule(int index, String timeOfDay, uint16_t duration, bool enabled);
    void removeSchedule(int index);
    ScheduleItem getSchedule(int index);
    int getScheduleCount();
    bool isValidScheduleIndex(int index) const;
    String makeScheduleKey(int index) const;

    // --- Persistence ---
    void begin();

    // --- Debugging ---
    void printAllSchedules();
};

extern DataManager Data;  // Global instance

#endif
