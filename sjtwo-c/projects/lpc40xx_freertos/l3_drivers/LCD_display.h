#pragma once

#include "gpio.h"
#include <stdint.h>

void LCD_init();

void write_str(char str[17], int line, int delay);