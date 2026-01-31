#pragma once
#include <Arduino.h>

namespace Motor {
  bool init(uint8_t servoPin, uint8_t enablePin,
            uint16_t minUs = 1000, uint16_t midUs = 1500, uint16_t maxUs = 2000,
            uint16_t freq = 50, uint8_t resolutionBits = 14);

  void enable();     // EN=HIGH + attach PWM + stop
  void disable();    // stop + detach PWM + EN=LOW

  void cw();         // write MIN pulse
  void ccw();        // write MAX pulse
  void stop();       // write MID pulse

  void writeUs(uint16_t us); // direct pulse write (requires enabled)
}
