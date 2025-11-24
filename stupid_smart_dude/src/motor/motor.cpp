#include "Motor.h"

namespace Motor {

    // Internal state
    static uint8_t _enPin   = 255;
    static uint8_t _stepPin = 255;
    static uint8_t _dirPin  = 255;

    static unsigned long _stepIntervalUs = 5000UL;  // default 200 Hz


    static void doStep() {
        digitalWrite(_stepPin, HIGH);
        delayMicroseconds(2);                 // A4988 pulse width (>1us)
        digitalWrite(_stepPin, LOW);
        delayMicroseconds(_stepIntervalUs);   // interval between steps
    }

    static void moveSteps(Direction direction, int32_t steps) {
        if (steps <= 0) return;

        digitalWrite(_dirPin, (direction == COUNTER_CLOCKWISE) ? HIGH : LOW);

        for (int32_t i = 0; i < steps; i++) {
            doStep();
        }
    }

    static void wiggle(Direction direction) {
        Direction opposite = (direction == CLOCKWISE) ? COUNTER_CLOCKWISE : CLOCKWISE;

        for (uint8_t i = 0; i < 2; i++) {
            moveSteps(direction, 1);
            moveSteps(opposite, 1);
        }
    }

    void init(uint8_t enPin, uint8_t stepPin, uint8_t dirPin,
              unsigned long stepIntervalUs) {
        _enPin   = enPin;
        _stepPin = stepPin;
        _dirPin  = dirPin;

        _stepIntervalUs = stepIntervalUs;

        pinMode(_stepPin, OUTPUT);
        pinMode(_dirPin,  OUTPUT);
        pinMode(_enPin,   OUTPUT);

        digitalWrite(_stepPin, LOW);
        digitalWrite(_dirPin,  LOW);
        digitalWrite(_enPin,   HIGH);  // keep disabled until used
    }

    void setStepInterval(unsigned long stepIntervalUs) {
        _stepIntervalUs = stepIntervalUs;
    }

    void enable() {
        digitalWrite(_enPin, LOW);   // active LOW
    }

    void disable() {
        digitalWrite(_enPin, HIGH);
    }

    void moveMotor(Direction direction, int32_t steps) {
        if (steps <= 0) return;

        enable();
        wiggle(direction);
        moveSteps(direction, steps);
        disable();
    }

} // namespace Motor
