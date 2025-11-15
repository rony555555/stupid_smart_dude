#include <WiFi.h>
#include "src/data/data.h"

const char* WIFI_SSID = "Yadin";
const char* WIFI_PASS = "54005400";

IPAddress local_IP(10, 100, 102, 101);  // static IP you want for the ESP32
IPAddress gateway(10, 100, 102, 1);     // usually your router's IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 100, 102, 22);  // optional
IPAddress secondaryDNS(10, 100, 102, 1);

// const uint16_t PORT = 8000;
// WiFiServer server(PORT);

void waitForWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connect timeout (continuing offline).");
    }
}

void setup() {
    delay(5000);
    Serial.begin(115200);
    Data.begin();

    Serial.println("\nDevice Booted");
    Serial.println("Loaded data from flash:");

    // set TZ once (it will be saved & reused next boots)
    if (Data.getTimezone().length() == 0) {
        Data.configureTimezone("IST-2IDT,M3.5.0/2,M10.5.0/2");
        Serial.println("Timezone configured: IST/IDT");
    }

    // Bring up Wi-Fi and attempt time sync
    waitForWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        bool ok = Data.syncTimeFromNTP();
        Serial.printf("Initial NTP sync: %s\n", ok ? "OK" : "FAILED");
    }

    // Optional: re-sync every 6 hours
    Data.enableAutoResync(6UL * 3600UL);

    // add schedules manually
    if (Data.getScheduleCount() == 0) {
        Serial.println("No schedules found — creating test schedules...");
        Data.addSchedule("05:45", 45, true);
        Data.addSchedule("18:00", 60, true);
    }

    Serial.println("Current schedules after setup:");
    Data.printAllSchedules();
}

void loop() {
    // Keep internal time ticking and auto re-syncing when due
    Data.loopTime();

    // Example usage: only act on schedules if time is valid
    if (Data.isTimeValid()) {
        time_t nowEpoch = Data.getTime();
        struct tm localTm;
        localtime_r(&nowEpoch, &localTm);

        // Build HH:MM string for comparison with schedule items
        char hhmm[6];
        snprintf(hhmm, sizeof(hhmm), "%02d:%02d", localTm.tm_hour, localTm.tm_min);

        // Example: check each minute whether to trigger something
        static uint8_t lastMinute = 255;
        if (lastMinute != localTm.tm_min) {
            lastMinute = localTm.tm_min;
            Serial.printf("Now %02d:%02d (epoch %ld)\n", localTm.tm_hour, localTm.tm_min, (long)nowEpoch);

            for (int i = 0; i < Data.getScheduleCount(); i++) {
                ScheduleItem s = Data.getSchedule(i);
                if (s.enabled && s.timeOfDay == String(hhmm)) {
                    Serial.printf("Trigger schedule [%d]: %s for %u minutes\n", i, s.timeOfDay.c_str(), s.duration);
                    // TODO: Call your motor: move dial to match 'duration'
                    // e.g., Motor.setDialMinutes(s.duration);
                }
            }
        }
    } else {
        // Time invalid → do not run time-based logic
        // Optionally retry NTP if Wi-Fi came up later:
        static uint32_t lastRetry = 0;
        if (WiFi.status() == WL_CONNECTED && millis() - lastRetry > 10000) {
            lastRetry = millis();
            if (Data.syncTimeFromNTP()) {
                Serial.println("NTP sync (retry) OK");
            } else {
                Serial.println("NTP sync (retry) failed");
            }
        }
    }

    delay(50);
}
