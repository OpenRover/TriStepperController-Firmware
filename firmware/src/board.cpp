// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "pins_arduino.h"
#include "utils.h"
#include <board.h>

namespace Board {

namespace Port {
struct SPI SPI {
  .MISO = {D12, INPUT}, .MOSI = {D11, OUTPUT}, .CS = {D10, OUTPUT, true},
  .SCLK = {D13, OUTPUT},
};
struct I2C_ColorSensor I2C_ColorSensor{
    .LED = {A2, OUTPUT},
    .INT = {A3, INPUT},
    .SDA = {A4, INPUT_PULLUP},
    .SCL = {A5, INPUT_PULLUP},
};
struct WS2812 WS2812 {
  .DATA = {B1, OUTPUT},
};
} // namespace Port

namespace LED {
Pin BUILTIN(LED_BUILTIN, OUTPUT);
Pin RED(LED_RED, OUTPUT);
Pin GREEN(LED_GREEN, OUTPUT);
Pin BLUE(LED_BLUE, OUTPUT);
} // namespace LED

void sw0_trigger_helper();
void sw1_trigger_helper();
void sw2_trigger_helper();

Switch SW[] = {
    {Pin(A7, INPUT_PULLDOWN), sw0_trigger_helper},
    {Pin(A6, INPUT_PULLDOWN), sw1_trigger_helper},
    {Pin(A1, INPUT_PULLDOWN), sw2_trigger_helper},
};

void sw0_trigger_helper() { SW[0].trigger(); }
void sw1_trigger_helper() { SW[1].trigger(); }
void sw2_trigger_helper() { SW[2].trigger(); }

Drv DRV[] = {
    {D8, D9, B0},
    {D5, D6, D7},
    {D2, D3, D4},
};

} // namespace Board

void __panic_enter__() {
  Board::Drv::disable();
  Board::LED::RED.init();
}

void __panic_signal__() {
  Board::LED::RED.write(HIGH);
  delay(100);
  Board::LED::RED.write(LOW);
  delay(100);
  Board::LED::RED.write(HIGH);
  delay(100);
  Board::LED::RED.write(LOW);
  delay(100);
}
