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
    bool timeValid;                       // Whether currentTime is valid
    time_t currentTime;                   // UNIX time, or 0 if invalid

    // Helper functions
    String serializeSchedule(const ScheduleItem &item);
    ScheduleItem deserializeSchedule(const String &data);

    void saveSchedules();
    void loadSchedules();

public:
    DataManager();

    // --- Time Handling ---
    void setTime(time_t t);
    time_t getTime();
    bool isTimeValid();
    void invalidateTime();

    // --- Schedule Handling ---
    void addSchedule(String timeOfDay, uint16_t duration, bool enabled);
    void updateSchedule(int index, String timeOfDay, uint16_t duration, bool enabled);
    void removeSchedule(int index);
    ScheduleItem getSchedule(int index);
    int getScheduleCount();

    // --- Persistence ---
    void begin();

    // --- Debugging ---
    void printAllSchedules();  
};

extern DataManager Data;  // Global instance

#endif 
