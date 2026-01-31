#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "src/data/data.h"
#include "src/timeManager/timeManager.h"
#include "src/motor/motor.h"

#define SERVO_PIN 17
#define SERVO_EN_PIN 20  // HIGH=enable, LOW=disable

#define SERVO_MIN_US 1350
// #define SERVO_MIN_US 500
#define SERVO_MID_US 1500
// #define SERVO_MAX_US 2500
#define SERVO_MAX_US 1650

#define DUDE_MOVE_MS 1300

using namespace Motor;
static bool dudeTimerActive = false;
static uint32_t dudeTimerEndMs = 0;

const char* WIFI_SSID = "Yadin";
const char* WIFI_PASS = "54005400";

IPAddress local_IP(10, 100, 102, 101);  // static IP you want for the ESP32
IPAddress gateway(10, 100, 102, 1);     // usually your router's IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 100, 102, 22);  // optional
IPAddress secondaryDNS(10, 100, 102, 1);

// const uint16_t PORT = 8000;
// WiFiServer server(PORT);

#define RGB_PIN 48    // as in OceanLabz example
#define NUM_PIXELS 1  // only one onboard LED
#define TIME_OK_BLINK_INTERVAL_MS 5000

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void initRgbLed() {
    pixel.begin();
    pixel.setBrightness(100);  // 0–255, tweak if it's too bright
    pixel.clear();
    pixel.show();
}

void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
    pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}

void rgbOff() {
    pixel.clear();
    pixel.show();
}

void blinkColor(uint8_t r, uint8_t g, uint8_t b, uint16_t onMs, uint16_t offMs) {
    setRgbColor(r, g, b);
    delay(onMs);
    rgbOff();
    delay(offMs);
}

void indicateMinuteTick() {
    // 1 small blue blink
    blinkColor(0, 0, 20, 80, 40);  // blue
}

void indicateTimeOk() {
    // 1 small green blink
    blinkColor(0, 5, 0, 80, 40);  // blue
}

void serviceTimeValidBlink() {
    static uint32_t lastBlinkMs = 0;

    if (!Time.isTimeValid()) return;

    uint32_t now = millis();
    if (now - lastBlinkMs < TIME_OK_BLINK_INTERVAL_MS) return;
    lastBlinkMs = now;

    // weak green blink
    indicateTimeOk();
}

void setDudeTimer(uint16_t duration) {
    if (duration > 120) {
        Serial.println("invalid duration");
        return;
    }

    // If a timer is already running, gracefully cancel it by returning home first.
    if (dudeTimerActive) {
        uint32_t now = millis();
        int32_t remainingMs = (int32_t)(dudeTimerEndMs - now);
        if (remainingMs < 0) remainingMs = 0;
        uint32_t requestedMs = (uint32_t)duration * 60UL * 1000UL;

        if (requestedMs > (uint32_t)remainingMs) {
            dudeTimerEndMs = now + requestedMs;
            Serial.printf(
                "Dude timer extended: remaining %lu ms, new %lu ms\n",
                (unsigned long)remainingMs,
                (unsigned long)requestedMs);
        } else {
            Serial.println("Dude timer already running with longer remaining time — ignoring");
        }
        return;
    }

    Motor::enable();
    delay(1000);
    Motor::ccw();
    delay(DUDE_MOVE_MS);
    Motor::stop();
    Motor::stop();

    // Duration is in minutes.
    const uint32_t durationMs = (uint32_t)duration * 60UL * 1000UL;
    dudeTimerEndMs = millis() + durationMs;
    dudeTimerActive = true;
    Serial.printf("Dude timer started: %u min (return at +%lu ms)\n",
                  duration, (unsigned long)durationMs);
}

void serviceDudeTimer() {
    if (!dudeTimerActive) return;

    // Motor::stop();
    // Handle millis() wrap by using signed subtraction.
    if ((int32_t)(millis() - dudeTimerEndMs) < 0) return;

    Serial.println("Dude timer finished — returning home");
    Motor::cw();
    delay(DUDE_MOVE_MS + 100);
    Motor::stop();
    Motor::stop();
    delay(1000);
    Motor::disable();
    dudeTimerActive = false;
}

void startWiFiOnce() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
        delay(250);
        Serial.print(".");
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi not found, connect timeout");
    }
}

void maintainWiFi() {
    static unsigned long lastAttempt = 0;
    const unsigned long RETRY_INTERVAL = 10000;

    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - lastAttempt < RETRY_INTERVAL) return;
    lastAttempt = now;

    Serial.println("WiFi not connected, retrying...");

    WiFi.disconnect(true, true);  // reset WiFi state
    WiFi.mode(WIFI_STA);
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void maintainTime() {
    static unsigned long lastNtpAttempt = 0;
    const unsigned long NTP_RETRY_INTERVAL = 10000;

    // If we already have valid time, we just let Data handle auto-resync
    if (Time.isTimeValid()) {
        Time.loop();
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return;  // Can't sync without WiFi; wait for maintainWiFi() to connect
    }

    unsigned long now = millis();
    if (now - lastNtpAttempt < NTP_RETRY_INTERVAL) return;
    lastNtpAttempt = now;

    Serial.println("Time invalid, trying NTP sync...");
    bool ok = Time.syncTimeFromNTP();
    Serial.printf("NTP sync: %s\n", ok ? "OK" : "FAILED");
}

void checkAndRunSchedules() {
    if (!Time.isTimeValid()) return;  // no global time yet, skip schedule logic

    time_t nowEpoch = Time.getTime();
    if (nowEpoch == 0) return;  // safety check

    struct tm localTm;
    localtime_r(&nowEpoch, &localTm);

    // Only run logic once per minute
    static int lastMinute = -1;
    if (localTm.tm_min == lastMinute) return;  // still same minute as last check
    lastMinute = localTm.tm_min;

    // Debug print of the current time
    Serial.printf("Now %02d:%02d (epoch %ld)\n",
                  localTm.tm_hour,
                  localTm.tm_min,
                  (long)nowEpoch);
    indicateMinuteTick();

    int nowMinutes = localTm.tm_hour * 60 + localTm.tm_min;  // Current time in "minutes since midnight"

    // Go over all schedules
    for (int i = 0; i < Data.getScheduleCount(); i++) {
        ScheduleItem s = Data.getSchedule(i);
        if (!s.enabled) continue;

        // Parse timeOfDay like "9:03" or "09:03"
        int sh = 0, sm = 0;
        if (sscanf(s.timeOfDay.c_str(), "%d:%d", &sh, &sm) != 2) {
            Serial.printf("Warning: bad time format in schedule [%d]: '%s'\n",
                          i, s.timeOfDay.c_str());
            continue;
        }

        int schedMinutes = sh * 60 + sm;

        if (schedMinutes == nowMinutes) {
            // This schedule should trigger now
            Serial.printf("Trigger schedule [%d]: %s for %u minutes\n",
                          i, s.timeOfDay.c_str(), s.duration);

            setDudeTimer(s.duration);
        }
    }
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////

void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("\nDevice Booted");

    initRgbLed();

    Data.begin();
    Serial.println("Loaded data from flash:");
    Time.begin();
    Motor::init(SERVO_PIN, SERVO_EN_PIN, SERVO_MIN_US, SERVO_MID_US, SERVO_MAX_US);

    // set TZ once (it will be saved & reused next boots)
    if (Time.getTimezone().length() == 0) {
        Time.configureTimezone("IST-2IDT,M3.5.0/2,M10.5.0/2");
        Serial.println("Timezone configured: IST/IDT");
    }

    // Bring up Wi-Fi and attempt time sync
    startWiFiOnce();
    if (WiFi.status() == WL_CONNECTED) {
        bool ok = Time.syncTimeFromNTP();
        Serial.printf("Initial NTP sync: %s\n", ok ? "OK" : "FAILED");
    } else {
        Serial.println("Skipping initial NTP sync (no WiFi yet).");
    }

    // Optional: re-sync every 6 hours
    Time.enableAutoResync(6UL * 3600UL);

    // Data.removeSchedule(1);
    // Data.removeSchedule(0);
    // add schedules manually
    if (Data.getScheduleCount() == 0) {
        Serial.println("No schedules found — creating test schedules...");
        Data.addSchedule("05:40", 70, true);
        Data.addSchedule("17:45", 75, true);
    }
    // Data.removeSchedule(6);
    // Data.removeSchedule(5);
    // Data.removeSchedule(4);
    // Data.removeSchedule(3);
    // Data.removeSchedule(2);
    // Data.removeSchedule(1);
    // Data.removeSchedule(0);
    // Data.addSchedule("14:20", 5, true);
    // Data.addSchedule("14:20", 3, false);
    // Data.addSchedule("14:30", 5, true);
    // Data.addSchedule("14:40", 5, true);
    // Data.addSchedule("14:43", 5, true);

    Serial.println("Current schedules after setup:");
    Data.printAllSchedules();
}

void loop() {
    maintainWiFi();
    maintainTime();
    checkAndRunSchedules();
    serviceDudeTimer();
    serviceTimeValidBlink();

    // Motor::enable();
    // Serial.println("enable");
    // delay(1000);
    // Serial.println("ccw");
    // Motor::ccw();
    // delay(1300);
    // // delay(170);
    // Serial.println("stop");
    // Motor::stop();
    // Motor::stop();
    // Motor::stop();
    // delay(20000);
    // Serial.println("cw");
    // Motor::cw();
    // delay(1400);
    // // delay(150);
    // Serial.println("stop");
    // Motor::stop();
    // Motor::stop();
    // Motor::stop();
    // delay(3000);
    // Serial.println("disable");
    // Motor::disable();
    // delay(3000);

    delay(5);
}
