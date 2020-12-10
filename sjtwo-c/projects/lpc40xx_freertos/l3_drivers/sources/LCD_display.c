#include "LCD_display.h"
#include <stdio.h>

gpio_s register_select;
gpio_s lcd_enable;
gpio_s db[4];

static void send_data(uint8_t data) {
  if (data & (1 << 0)) {
    gpio__set(db[0]);
    printf("set gpio 0\n");
  } else {
    gpio__reset(db[0]);
  }

  if (data & (1 << 1)) {
    gpio__set(db[1]);
    printf("set gpio 1\n");
  } else {
    gpio__reset(db[1]);
  }

  if (data & (1 << 2)) {
    gpio__set(db[2]);
    printf("set gpio 2\n");
  } else {
    gpio__reset(db[2]);
  }

  if (data & (1 << 3)) {
    gpio__set(db[3]);
    printf("set gpio 3\n");
  } else {
    gpio__reset(db[3]);
  }
  gpio__set(lcd_enable);
  gpio__reset(lcd_enable);
}

static void data_or_instruction(uint8_t data) {
  gpio__reset(register_select);
  uint8_t high = data >> 4;
  send_data(high);
  send_data(data);
  delay__ms(1);
}

static void write_char(uint8_t data_instr) {
  gpio__set(register_select);
  uint8_t high = data_instr >> 4;
  send_data(high);
  send_data(data_instr);
  delay__ms(1);
}

static void preset() {
  for (int i = 0; i < 3; i++) {
    delay__ms(1);
    send_data(0b0011);
  }
  delay__ms(1);
  send_data(0b0010);
}

static void pin_init() {
  register_select = gpio__construct_as_output(GPIO__PORT_1, 28);
  lcd_enable = gpio__construct_as_output(GPIO__PORT_0, 1);
  db[3] = gpio__construct_as_output(GPIO__PORT_2, 0);
  db[2] = gpio__construct_as_output(GPIO__PORT_2, 2);
  db[1] = gpio__construct_as_output(GPIO__PORT_0, 7);
  db[0] = gpio__construct_as_output(GPIO__PORT_2, 7);
  gpio__set_as_output(lcd_enable);
  gpio__set_as_output(register_select);
  for (uint8_t INDEX = 0; INDEX < 4; INDEX++) {
    gpio__set_as_output(db[INDEX]);
  }
}

void LCD_init() {
  pin_init();

  gpio__reset(lcd_enable);
  gpio__reset(register_select);
  preset();

  data_or_instruction(0b00101000);
  data_or_instruction(0b00001000);
  data_or_instruction(0b00000001);
  data_or_instruction(0b0000110);
  data_or_instruction(0b00001111);
}

void write_str(char str[17], int line, int delay) {
  uint8_t inst;
  if (line == 1) {
    inst = (1 << 7) | 0x40;
  } else {
    inst = (1 << 7) | 0x00;
  }

  data_or_instruction(inst);
  int i = 0;
  while (str[i] != '\0') {
    delay__ms(delay);
    write_char(str[i]);
    i++;
  }
}
