#include "data.h"

DataManager Data;  // Global instance

// Constructor
DataManager::DataManager() {
    timeValid = false;
    currentTime = 0;
}

// Initialize Preferences and load existing schedules
void DataManager::begin() {
    prefs.begin("data_store", false);
    loadSchedules();
}

// --- Time handling ---
void DataManager::setTime(time_t t) {
    currentTime = t;
    timeValid = true;
}

time_t DataManager::getTime() {
    return currentTime;
}

bool DataManager::isTimeValid() {
    return timeValid;
}

void DataManager::invalidateTime() {
    timeValid = false;
    currentTime = 0;
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
        String key = "s" + String(i);
        prefs.putString(key.c_str(), serializeSchedule(schedules[i]));
    }
}

void DataManager::loadSchedules() {
    schedules.clear();
    int count = prefs.getInt("count", 0);
    for (int i = 0; i < count; i++) {
        String key = "s" + String(i);
        String saved = prefs.getString(key.c_str(), "");
        if (saved.length() > 0)
            schedules.push_back(deserializeSchedule(saved));
    }
}

// --- Schedule management ---
void DataManager::addSchedule(String timeOfDay, uint16_t duration, bool enabled) {
    ScheduleItem item = {timeOfDay, duration, enabled};
    schedules.push_back(item);
    saveSchedules();
}

void DataManager::updateSchedule(int index, String timeOfDay, uint16_t duration, bool enabled) {
    if (index < 0 || index >= schedules.size()) return;
    schedules[index] = {timeOfDay, duration, enabled};
    saveSchedules();
}

void DataManager::removeSchedule(int index) {
    if (index < 0 || index >= schedules.size()) return;
    schedules.erase(schedules.begin() + index);
    saveSchedules();
}

ScheduleItem DataManager::getSchedule(int index) {
    if (index < 0 || index >= schedules.size()) {
        return {"00:00", 0, false};
    }
    return schedules[index];
}

int DataManager::getScheduleCount() {
    return schedules.size();
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
