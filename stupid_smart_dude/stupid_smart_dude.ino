#include "src/data/data.h"

void setup() {
    delay(5000);
    Serial.begin(115200);
    Data.begin();

    Serial.println("\nDevice Booted");
    Serial.println("Loaded data from flash:");
    Data.printAllSchedules();

    // Example: only add schedules if none exist
    // if (Data.getScheduleCount() == 0) {
    //     Serial.println("No schedules found — creating test schedules...");
    //     Data.addSchedule("06:00", 45, true);
    //     Data.addSchedule("19:00", 30, false);
    // }
    Data.removeSchedule(0);

    Serial.println("Current schedules after setup:");
    Data.printAllSchedules();
}

void loop() {}
