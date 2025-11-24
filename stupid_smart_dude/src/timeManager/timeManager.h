#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>  // for saving timezone
#include <time.h>         // for time_t, time(), localtime_r

class TimeManager {
  private:
    Preferences prefs;       // NVS storage (only for TZ)
    bool timeValid;          // Do we trust the clock?
    time_t lastSyncEpoch;    // When we last successfully synced (epoch seconds)
    uint32_t resyncInterval; // Auto-resync period in seconds (0 = disabled)
    String tzString;         // POSIX TZ string, e.g. "IST-2IDT,M3.5.0/2,M10.5.0/2"

    void saveTZ();           // Save tzString to NVS
    void loadTZ();           // Load tzString from NVS

  public:
    TimeManager();

    // Call once in setup()
    void begin();

    // Time validity
    bool isTimeValid() const;
    void invalidateTime();

    // Get current epoch (UTC-based). Returns 0 if time is invalid.
    time_t getTime() const;

    // NTP sync via SNTP. Requires WiFi to be connected.
    // Returns true if we got a sane epoch value.
    bool syncTimeFromNTP(const char* server1 = "pool.ntp.org",
                         const char* server2 = "time.nist.gov",
                         uint32_t timeoutMs = 12000);

    // Timezone management
    void configureTimezone(const String& tz);
    const String& getTimezone() const;

    // Auto-resync
    void enableAutoResync(uint32_t intervalSeconds);  // 0 disables
    time_t getLastSyncEpoch() const;
    void loop();  // call frequently to perform auto-resync if enabled
};

extern TimeManager Time;  // Global instance

#endif
