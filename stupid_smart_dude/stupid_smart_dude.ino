#include <Arduino.h>

#define EN_PIN   10   // ENABLE (active LOW)
#define STEP_PIN 11   // STEP
#define DIR_PIN  12   // DIR

#define CLOCKWISE false
#define COUNTER_CLOCKWISE true

const unsigned long STEP_INTERVAL_US = 5000;  // 200 Hz step rate


void doStep() {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2);   // A4988 requires >1us
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_INTERVAL_US); 
}

void moveSteps(bool direction, int steps) {
    digitalWrite(DIR_PIN, direction);
    for (int i = 0; i < steps; i++) {
        doStep();
    }
}

void wiggle(bool direction) {
    bool opposite = !direction;

    for (int i = 0; i < 2; i++) {
        moveSteps(direction, 1);
        moveSteps(opposite, 1);
    }
}

// 3200 steps = 360 deg of motor
void moveMotor(bool direction, int steps) {
    digitalWrite(EN_PIN, LOW); // Enable
    wiggle(direction);
    moveSteps(direction, steps);
    digitalWrite(EN_PIN, HIGH); //disable
}

// ----------------------------------------------------------
// Setup
// ----------------------------------------------------------
void setup() {
    Serial.begin(115200);

    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);

    digitalWrite(STEP_PIN, LOW);
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(EN_PIN, HIGH); // keep disabled until used

    Serial.println("Stepper wiggle-motion test ready");
}

// ----------------------------------------------------------
// Loop
// ----------------------------------------------------------
void loop() {
    Serial.println("Calling moveMotor...");
    moveMotor(COUNTER_CLOCKWISE, 3200/4);
    delay(10000);
}
