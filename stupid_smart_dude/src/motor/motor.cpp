#include "motor.h"

namespace Motor {
  static uint8_t  s_servoPin = 255;
  static uint8_t  s_enablePin = 255;
  static uint16_t s_minUs = 500, s_midUs = 1500, s_maxUs = 2500;
  static uint16_t s_freq = 50;
  static uint8_t  s_res = 14;
  static bool     s_attached = false;

  static uint32_t usToDuty(uint32_t us) {
    uint32_t maxDuty = (1UL << s_res) - 1;
    return (us * maxDuty) / 20000UL; // 20ms period for 50Hz
  }

  bool init(uint8_t servoPin, uint8_t enablePin,
            uint16_t minUs, uint16_t midUs, uint16_t maxUs,
            uint16_t freq, uint8_t resolutionBits) {
    s_servoPin = servoPin;
    s_enablePin = enablePin;
    s_minUs = minUs;
    s_midUs = midUs;
    s_maxUs = maxUs;
    s_freq = freq;
    s_res = resolutionBits;

    pinMode(s_enablePin, OUTPUT);
    digitalWrite(s_enablePin, LOW);   // disabled by default

    // don't attach yet; attach only on enable()
    s_attached = false;
    return true;
  }

  void enable() {
    digitalWrite(s_enablePin, HIGH);

    if (!s_attached) {
      s_attached = ledcAttach(s_servoPin, s_freq, s_res);
    }
    // stop by default
    if (s_attached) ledcWrite(s_servoPin, usToDuty(s_midUs));
  }

  void disable() {
    // stop first
    if (s_attached) {
      ledcWrite(s_servoPin, usToDuty(s_midUs));
      delay(20);
      ledcDetach(s_servoPin);
      s_attached = false;
    }
    digitalWrite(s_enablePin, LOW);
  }

  void writeUs(uint16_t us) {
    if (!s_attached) return;
    ledcWrite(s_servoPin, usToDuty(us));
  }

  void cw()   { writeUs(s_minUs); }
  void ccw()  { writeUs(s_maxUs); }
  void stop() { writeUs(s_midUs); }
}
