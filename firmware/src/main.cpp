// =============================================================================
// Stepper Controller Firmware for the RoverMaster Project
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "agent.h"
#include "board.h"
#include "debug.h"
#include "global.h"
#include "motor.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

inline HardwareSerial &DebugSerialPort() {
  auto &serial = Serial2;
  const auto rx = Board::Port::I2C_ColorSensor.SDA.pin;
  const auto tx = Board::Port::I2C_ColorSensor.SCL.pin;
  serial.begin(115200, SERIAL_8N1, rx, tx);
  return serial;
}

void blink(void *) {
  while (true) {
    static uint8_t brightness = 0;
    if (Serial) {
      // Blink
      Board::LED::BUILTIN.analogWrite(brightness & 0b00100000 ? 255 : 0);
    } else {
      // Breathe
      Board::LED::BUILTIN.analogWrite(
          2 * (brightness > 128 ? 255 - brightness : brightness));
    }
    brightness += 1;
    delay(4);
  }
}

void timer(void *) {
  constexpr auto cpu_freq = 80; // MHz
  constexpr auto interval = 10; // microseconds
  constexpr auto divider = cpu_freq * interval;
  auto timer = timerBegin(0, divider, true);
  timerAttachInterrupt(timer, &motorTick, true);
  timerAlarmWrite(timer, 1, true);
  timerAlarmEnable(timer);
  vTaskDelete(nullptr);
}

void pool(void *) {
  while (true) {
    // agentTick();
    motorTick();
  }
}

void rescue(const char *reason) {
  Board::LED::RED.init();
  const auto len = strlen(reason);
  while (true) {
    Global::tx.send(0, Protocol::Method::LOG, Protocol::Property::NA, reason,
                    len);
    debug_write((void *)reason, len);
    DEBUG("Core 0 Trace: %s:%d (%s) %s\n", __trace_core0__.file,
          __trace_core0__.line, __trace_core0__.func,
          __trace_core0__.msg ? __trace_core0__.msg : "");
    DEBUG("Core 1 Trace: %s:%d (%s) %s\n", __trace_core1__.file,
          __trace_core1__.line, __trace_core1__.func,
          __trace_core1__.msg ? __trace_core1__.msg : "");
    Board::LED::RED.write(HIGH);
    delay(200);
    Board::LED::RED.write(LOW);
    delay(200);
    Board::LED::RED.write(HIGH);
    delay(200);
    Board::LED::RED.write(LOW);
    delay(200);
  }
}

void setup() {
  Serial.begin(Global::Config::SERIAL_BAUD_RATE);
  DebugSerialPort();
  switch (esp_reset_reason()) {
  case ESP_RST_UNKNOWN:
    DEBUG("Reset reason can not be determined\n");
    break;
  case ESP_RST_POWERON:
    DEBUG("Reset due to power-on event\n");
    break;
  case ESP_RST_EXT:
    DEBUG("Reset by external pin (not applicable for ESP32)\n");
    break;
  case ESP_RST_SW:
    DEBUG("Software reset via esp_restart\n");
    break;
  case ESP_RST_PANIC:
    return rescue("Software reset due to exception/panic\n");
  case ESP_RST_INT_WDT:
    return rescue("Reset (software or hardware) due to interrupt watchdog\n");
  case ESP_RST_TASK_WDT:
    return rescue("Reset due to task watchdog\n");
  case ESP_RST_WDT:
    return rescue("Reset due to other watchdogs\n");
  case ESP_RST_DEEPSLEEP:
    return rescue("Reset after exiting deep sleep mode\n");
  case ESP_RST_BROWNOUT:
    return rescue("Brownout reset (software or hardware)\n");
  case ESP_RST_SDIO:
    DEBUG("Reset over SDIO\n");
    break;
  }
  Board::init();
  Motor::init();
  // Configure Task Watchdog Timer for agentTick
  // 1 second timeout - will reset to rescue mode if agentTick() freezes
  esp_task_wdt_init(1, true); // 1 second timeout, panic on timeout
  xTaskCreatePinnedToCore(blink, "blink", 1024, nullptr, 1, nullptr, 0);
  TaskHandle_t agentTaskHandle = nullptr;
  xTaskCreatePinnedToCore(agent, "agent", 8192, nullptr, 1, &agentTaskHandle,
                          0);
  // Subscribe agent task to watchdog
  if (agentTaskHandle != nullptr) {
    esp_task_wdt_add(agentTaskHandle);
    DEBUG("Agent task subscribed to watchdog (1s timeout)\n");
  } else {
    PANIC("Failed to create agent task");
  }
  xTaskCreatePinnedToCore(timer, "timer", 8192, nullptr,
                          configMAX_PRIORITIES - 1, nullptr, 1);
  // xTaskCreatePinnedToCore(pool, "pool", 16384, nullptr,
  //                         configMAX_PRIORITIES - 1, nullptr, 1);
}

void loop() {}
