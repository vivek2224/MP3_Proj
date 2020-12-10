
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "ssp2.h"
#include <stdio.h>

bool pause_music(bool pause);

void reset();

uint16_t sci_read(uint8_t address_to_read_from);

void pin_config();

void sci_write(uint8_t address_to_write_to, uint16_t data_to_write);

void set_volume(uint8_t left, uint8_t right);

void set_bass(uint8_t bass_level);

void set_treble(uint8_t treble_level);

void play_data(uint8_t *buffer, int buffer_size);

bool dreq_level();
