#include "audio_driver.h"

static gpio_s xdcs_pin_h;
static gpio_s dreq_pin_h;
static gpio_s adesto_cs_h;
static gpio_s rst_pin_h;
static gpio_s xdcs_data_select_h;
static bool pause_music_var = false;

void pin_config() {
  xdcs_pin_h = gpio__construct_as_output(GPIO__PORT_0, 25);
  dreq_pin_h = gpio__construct_as_input(GPIO__PORT_1, 20);
  rst_pin_h = gpio__construct_as_output(GPIO__PORT_2, 1);
  xdcs_data_select_h = gpio__construct_as_output(GPIO__PORT_0, 26);
}

void reset() {
  gpio__reset(rst_pin_h);
  delay__ms(50);
  gpio__set(rst_pin_h);
  delay__ms(50);
  gpio__set(xdcs_pin_h);
  delay__ms(50);
  gpio__set(xdcs_data_select_h);
  delay__ms(50);
  sci_write(0x00, 0x0800 | 0x0004);
  delay__ms(50);
  delay__ms(50);
  sci_write(0x03, 0x6000);
  delay__ms(50);
}

uint8_t begin() {
  pin_config();
  delay__ms(10);
  gpio__reset(rst_pin_h); // set rst as low
  delay__ms(10);
  gpio__set(xdcs_pin_h); // set cs as high
  delay__ms(10);
  gpio__set(xdcs_data_select_h); // set cs as high
  delay__ms(10);
  reset();
}

void play_data(uint8_t *buffer, int buffer_size) {
  gpio__reset(xdcs_data_select_h);
  for (int i = 0; i < buffer_size; i = i++) {
    while (!dreq_level()) {
      ;
    }
    ssp2__exchange_byte(*buffer++);
  }
  gpio__set(xdcs_data_select_h);
}

bool pause_music(bool pause) {
  if (pause) {
    return true;
  } else {
    return false;
  }
}

static void decoder_xcs_h() {
  // LPC_GPIO1->CLR |= (0x1 << 10);
  // gpio__reset(adesto_cs);
  gpio__reset(xdcs_pin_h);
}

static void decoder_xds_h() {
  // LPC_GPIO1->SET |= (0x1 << 10);
  // gpio__set(adesto_cs);
  gpio__set(xdcs_pin_h);
}

bool dreq_level() {
  if (gpio__get(dreq_pin_h)) {
    return true;
  } else {
    return false;
  }
}

uint16_t sci_read(uint8_t address_to_read_from) {
  uint16_t ret;
  uint8_t val1;
  uint8_t val2;
  decoder_xcs_h();
  {
    uint8_t byte_FF = ssp2__exchange_byte(0x03);
    ssp2__exchange_byte(address_to_read_from);
    val1 = ssp2__exchange_byte(0x0A);
    val2 = ssp2__exchange_byte(0x0A);
  }
  decoder_xds_h();
  ret = val1;
  ret = ret << 8;
  ret |= val2;
  return ret;
}

void sci_write(uint8_t address_to_write_to, uint16_t data_to_write) {
  uint8_t lower = data_to_write & 0xFF;
  uint8_t higher = data_to_write >> 8;
  decoder_xcs_h();
  {
    ssp2__exchange_byte(0x02);
    ssp2__exchange_byte(address_to_write_to);
    ssp2__exchange_byte(higher);
    ssp2__exchange_byte(lower);
  }
  decoder_xds_h();
}

void set_volume(uint8_t left, uint8_t right) {
  uint16_t vol = right | (left << 8);
  sci_write(0x0B, vol);
  // sci_write(0x)
}

void set_bass(uint8_t bass_level) {
  uint16_t current_bass = sci_read(0x02);
  current_bass &= 0xFF0F;
  current_bass |= (bass_level << 4);
  sci_write(0x02, current_bass);
}

void set_treble(uint8_t treble_level) {
  uint16_t current_treble = sci_read(0x02);
  current_treble &= 0x0FFF;
  current_treble |= (treble_level << 12);
  sci_write(0x02, current_treble);
}