// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "pins_arduino.h"
#include "utils.h"
#include <board.h>

namespace Board {

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
