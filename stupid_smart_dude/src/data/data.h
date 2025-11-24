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

    // Helper functions
    String serializeSchedule(const ScheduleItem& item);
    ScheduleItem deserializeSchedule(const String& data);

    void saveSchedules();
    void loadSchedules();

   public:
    DataManager();

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
