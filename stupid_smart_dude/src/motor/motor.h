#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

namespace Motor {

    enum Direction : uint8_t {
        CLOCKWISE = 0,
        COUNTER_CLOCKWISE = 1
    };

    // Initialize pins and timing
    void init(uint8_t enPin, uint8_t stepPin, uint8_t dirPin,
              unsigned long stepIntervalUs = 5000UL);

    void setStepInterval(unsigned long stepIntervalUs);

    // Move motor:
    // 1) enable
    // 2) wiggle (2x: 1 step forward/back)
    // 3) move 'steps' in 'direction'
    // 4) disable
    void moveMotor(Direction direction, int32_t steps);

    void enable();
    void disable();
}

#endif // MOTOR_H
