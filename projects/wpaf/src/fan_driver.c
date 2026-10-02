#include <stdio.h>
#include <stdint.h>
#include "fan_driver.h"

void fan_func_set_pwm_unimplemented(__attribute__((unused)) fan_handler*, __attribute__((unused)) uint8_t t) {
}

uint32_t fan_func_status_unimplemented(__attribute__((unused)) fan_handler*) {
  return 0;
}
