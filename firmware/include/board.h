// =============================================================================
// Arduino Nano Stepper Controller Physical Connections
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once
#include <Arduino.h>
#include <pins_arduino.h>

namespace Board {

class Pin {
  inline int maybeInverted(int value) const { return invert ? !value : value; }

public:
  const uint8_t pin;  // GPIO pin number
  const uint8_t mode; // GPIO pin mode
  const bool invert;
  Pin(uint8_t pin, uint8_t mode, bool invert = false)
      : pin(pin), mode(mode), invert(invert) {};
  inline void init() const { pinMode(pin, mode); }
  inline bool read() const { return maybeInverted(digitalRead(pin)); }
  inline void write(bool value) const {
    if (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN) {
      digitalWrite(pin, maybeInverted(value));
    }
  }
  inline void toggle() const {
    if (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN) {
      digitalWrite(pin, !digitalRead(pin));
    }
  }
  inline void analogWrite(int value) const {
    if (invert)
      value = 255 - value;
    if (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN) {
      ::analogWrite(pin, value);
    }
  }
};

namespace Port {
extern struct SPI {
  Pin MISO, MOSI, CS, SCLK;
  inline void init() {
    MISO.init();
    MOSI.init();
    CS.init();
    SCLK.init();
  }
} SPI;
extern struct I2C_ColorSensor {
  Pin LED, INT, SDA, SCL;
  inline void init() {
    LED.init();
    INT.init();
    SDA.init();
    SCL.init();
  }
} I2C_ColorSensor;
extern struct WS2812 {
  Pin DATA;
  inline void init() { DATA.init(); }
} WS2812;
inline void init() {
  SPI.init();
  I2C_ColorSensor.init();
  WS2812.init();
}
} // namespace Port

namespace LED {
extern Pin BUILTIN, RED, GREEN, BLUE; // Onboard RGB LED
} // namespace LED

// External Switches
class Switch : public Pin {
public:
  typedef struct Subscription {
    bool rise : 1;
    bool fall : 1;
    bool change : 1;
    void clear() { rise = fall = change = false; }
  } Subscription;

  // Only one process can subscribe at a time.
  Subscription *sub = nullptr;

  void (*const trigger_helper)();

  inline void trigger() {};

  Switch(Pin &&pin, void (*trigger_helper)())
      : Pin(pin), trigger_helper(trigger_helper) {};

  void init() const {
    Pin::init();
    attachInterrupt(pin, trigger_helper, CHANGE);
  };
};

extern Switch SW[3];

// TMC2209 Stepper Driver
class Drv {
public:
  static constexpr uint8_t EN = A0;
  static constexpr uint8_t TX = D1;
  static constexpr uint8_t RX = D0;
  static constexpr long BAUD_RATE = 115200;
  static constexpr HardwareSerial &serial = Serial1;
  static constexpr uint32_t baud = 115200;
  static inline void init() {
    pinMode(EN, OUTPUT);
    serial.begin(baud, SERIAL_8N1, RX, TX);
  }
  static inline bool is_enabled() { return digitalRead(EN) == LOW; }
  static inline void enable() { digitalWrite(EN, LOW); }
  static inline void disable() { digitalWrite(EN, HIGH); }

public:
  const Pin dir, step, diag;
  Drv(uint8_t dir, uint8_t step, uint8_t diag)
      : dir(dir, OUTPUT), step(step, OUTPUT), diag(diag, INPUT) {};
};

extern Drv DRV[3];

inline void init() {
  LED::BUILTIN.init();
  LED::RED.init();
  LED::GREEN.init();
  LED::BLUE.init();
  for (auto &sw : SW)
    sw.init();
}

} // namespace Board
