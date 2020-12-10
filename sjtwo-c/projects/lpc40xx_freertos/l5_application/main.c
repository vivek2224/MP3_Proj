#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"

#include "audio_driver.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
// 'static' to make these functions 'private' to this file
static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

#include "acceleration.h"
#include "event_groups.h"
#include "ff.h"
#include <string.h>
int file_count = 0;

#include "i2c_slave_init.h"
#include "uart_lab.h"

extern QueueHandle_t song_name_queue;
static QueueHandle_t mp3_file_queue;
static QueueHandle_t lcd_song_name;
const int NUM_OF_SONGS = 7;
typedef char songname_t[32];
typedef char song_data_t[512];
bool pause_music_interrupt = false;
char song_array[7][32];
int song_count = 0;
bool new_song_interrupt = false;
uint16_t adc_value;

static void read_file(const char *filename) {
  puts("Read and stored file name");
  FIL file;
  UINT bytes_written = 0;
  new_song_interrupt = false;
  FRESULT result = f_open(&file, filename, (FA_READ | FA_OPEN_EXISTING));
  if (FR_OK == result) {
    song_data_t buffer = {};
    UINT bytes_to_read = 512;
    UINT bytes_done_reading = 1;
    while (bytes_done_reading > 0) {
      if (new_song_interrupt) {
        break;
      }
      if (!pause_music_interrupt) {
        FRESULT rd = f_read(&file, buffer, bytes_to_read, &bytes_done_reading);
        xQueueSend(mp3_file_queue, buffer, portMAX_DELAY);
      } else {
        bytes_done_reading = 1;
      }
    }

    f_close(&file);
  } else {
    puts("Unavailable song");
  }
}
static void mp3_file_reader_task(void *p) {
  songname_t s_name = {};
  while (1) {
    if (xQueueReceive(song_name_queue, &s_name, 3000)) {
      read_file(s_name);
    } else {
      puts("Queue did not receive item");
    }
  }
}

static void mp3_decoder_send_block(song_data_t s_data) {
  for (size_t index = 0; index < sizeof(song_data_t); index++) {
    vTaskDelay(3);
    putchar(s_data[index]);
  }
}

static void print_hex(song_data_t s_data) {
  for (size_t index = 0; index < sizeof(song_data_t); index++) {
    vTaskDelay(1);
    printf("%02x", s_data[index]);
  }
}

static void mp3_data_player_task(void *p) {
  song_data_t s_data = {};
  while (1) {
    memset(&s_data[0], 0, sizeof(song_data_t));
    if (xQueueReceive(mp3_file_queue, &s_data[0], portMAX_DELAY)) {
      play_data(&s_data, 512);
    }
  }
}
TaskHandle_t read_handle;
TaskHandle_t play_handle;
void milestone_1_main() {
  song_name_queue = xQueueCreate(1, sizeof(songname_t));
  mp3_file_queue = xQueueCreate(2, sizeof(song_data_t));

  xTaskCreate(mp3_file_reader_task, "reader", 512, NULL, 2, &read_handle);
  xTaskCreate(mp3_data_player_task, "player", 512, NULL, 1, &play_handle);
}

#include "LCD_display.h"
#include "adc.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "ssp2.h"

void spi_task(void *p) {
  reset();
  ssp2__initialize(24);
  while (1) {
    sci_write(0x0B, 0x050E);
    uint16_t p = sci_read(0x0B);
    vTaskDelay(100);
    fprintf(stderr, "Version: %04x\n", p);
  }
}
void milestone_2_main() {
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCTION_4); // mosi
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCTION_4); // miso
  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCTION_4); // sck
  ssp2__initialize(24);
  begin();
}

int pause_or_play_count = 0;
int get_count(int count_p) {
  if (count_p >= 0 && count_p < NUM_OF_SONGS) {
    return count_p;
  } else if (count_p < 0) {
    count_p = NUM_OF_SONGS - 1;
    return count_p;
  } else if (count_p >= NUM_OF_SONGS) {
    count_p = 0;
    return count_p;
  }
}
void pin29_resume() {
  fprintf(stderr, "29\n");
  pause_music_interrupt = false;
}

void pin30_pause() {
  fprintf(stderr, "30\n");
  if (pause_or_play_count % 2 == 0) {
    pause_music_interrupt = true;
  } else {
    pause_music_interrupt = false;
  }
  pause_or_play_count++;
}

void pin29_up() {
  fprintf(stderr, "up");
  new_song_interrupt = true;
  song_count++;
  if (song_count >= NUM_OF_SONGS) {
    song_count = 0;
  }
  xQueueSendFromISR(song_name_queue, song_array[song_count], NULL);
}

void pin29_volume() {
  fprintf(stderr, "vol");
  if (adc_value >= 2000) {
    set_volume(0x70, 0x3F);
  } else {
    set_volume(0x18, 0x18);
  }
}

void pin30_down() {
  fprintf(stderr, "down");
  new_song_interrupt = true;
  song_count--;
  if (song_count < 0) {
    song_count = NUM_OF_SONGS - 1;
  }
  xQueueSendFromISR(song_name_queue, song_array[song_count], NULL);
  xQueueSendFromISR(lcd_song_name, song_array[song_count], NULL);
}

void lcd_down() {
  fprintf(stderr, "down");
  song_count--;
  if (song_count < 0) {
    song_count = NUM_OF_SONGS - 1;
  }
  xQueueSendFromISR(lcd_song_name, song_array[song_count], NULL);
}

void play_song_menu() {
  fprintf(stderr, "play");
  new_song_interrupt = true;
  if (song_count < 0) {
    song_count = NUM_OF_SONGS - 1;
  }
  xQueueSendFromISR(song_name_queue, song_array[song_count], NULL);
}

volatile int bass_treble_volume = 0;
void change_bass_treble_volume() {
  fprintf(stderr, "vol");
  bass_treble_volume++;
}

void gpio0_isr(void) { gpio0__interrupt_dispatcher(); }

void interrupt_setup() {
  strcpy(song_array[0], "song_one.mp3");
  strcpy(song_array[1], "song_two.mp3");
  strcpy(song_array[2], "Kokomo.mp3");
  strcpy(song_array[3], "AngelicRays.mp3");
  strcpy(song_array[4], "SorryBlameItOnMe.mp3");
  strcpy(song_array[5], "BeingKindToEachOther.mp3");
  strcpy(song_array[6], "Swing.mp3");
  gpio0__attach_interrupt(17, GPIO_INTR__RISING_EDGE, pin30_pause);
  gpio0__attach_interrupt(22, GPIO_INTR__RISING_EDGE, lcd_down);
  gpio0__attach_interrupt(11, GPIO_INTR__RISING_EDGE, play_song_menu);
  gpio0__attach_interrupt(16, GPIO_INTR__RISING_EDGE, change_bass_treble_volume);
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0_isr, "dispatcher");
  NVIC_EnableIRQ(GPIO_IRQn);
}

void adc_task(void *p) {
  adc__initialize();
  adc__enable_burst_mode(ADC__CHANNEL_5);
  LPC_IOCON->P1_31 &= ~(1 << 7);
  adc_value = 10;
  int currVal = 0;
  while (1) {
    adc_value = adc__get_channel_reading_with_burst_mode();
    // fprintf(stderr, "res:%d", adc_value);
    if (adc_value - currVal > 50 || adc_value - currVal < -50) {
      currVal = adc_value;

      write_str("                   ", 0, 1);
      if (bass_treble_volume % 3 == 0) { // volume
        if (adc_value < 500) {
          set_volume(0x00, 0x00);
          write_str("|--------|", 0, 1);
        } else if (adc_value < 1000) {
          set_volume(0x0F, 0x0F);
          write_str("|------- |", 0, 1);
        } else if (adc_value < 1500) {
          set_volume(0x30, 0x30);
          write_str("|------  |", 0, 1);
        } else if (adc_value < 2000) {
          set_volume(0x4F, 0x4F);
          write_str("|-----   |", 0, 1);
        } else if (adc_value < 2500) {
          set_volume(0x5F, 0x5F);
          write_str("|----    |", 0, 1);
        } else if (adc_value < 3000) {
          set_volume(0x6F, 0x6F);
          write_str("|---     |", 0, 1);
        } else if (adc_value < 3500) {
          set_volume(0xBF, 0xBF);
          write_str("|--      |", 0, 1);
        } else {
          set_volume(0xFE, 0xFE);
          write_str("|-       |", 0, 1);
        }
      } else if (bass_treble_volume % 3 == 1) {
        if (adc_value < 500) {
          set_bass(0x00);
        } else if (adc_value < 1000) {
          set_bass(0x02);
        } else if (adc_value < 1500) {
          set_bass(0x04);
        } else if (adc_value < 2000) {
          set_bass(0x06);
        } else if (adc_value < 2500) {
          set_bass(0x08);
        } else if (adc_value < 3000) {
          set_bass(0x0A);
        } else if (adc_value < 3500) {
          set_bass(0x0C);
        } else {
          set_bass(0x0E);
        }
      } else {
        if (adc_value < 500) {
          set_treble(0x00);
        } else if (adc_value < 1000) {
          set_treble(0x01);
        } else if (adc_value < 1500) {
          set_treble(0x02);
        } else if (adc_value < 2000) {
          set_treble(0x03);
        } else if (adc_value < 2500) {
          set_treble(0x04);
        } else if (adc_value < 3000) {
          set_treble(0x05);
        } else if (adc_value < 3500) {
          set_treble(0x06);
        } else {
          set_treble(0x07);
        }
      }
    }

    vTaskDelay(2000);
  }
}
static gpio_s test_gp;

void lcd_task(void *p) {
  songname_t s_name = {};
  while (1) {
    if (xQueueReceive(lcd_song_name, &s_name, 3000)) {
      write_str("                   ", 0, 100);
      write_str("                   ", 1, 100);
      write_str(s_name, 0, 100);
      int num = get_count(song_count + 1);
      (song_array[num], 1, 100);
    } else {
      write_str("                   ", 0, 100);
      write_str("                   ", 1, 100);
      int num_one = get_count(song_count);
      write_str(song_array[num_one], 0, 100);
      int num = get_count(song_count + 1);
      write_str(song_array[num], 1, 100);
    }
  }
}
void lcd_setup() {
  LCD_init();
  write_str("Hello, Please", 0, 100);
  write_str("Choose a song!", 1, 100);
  lcd_song_name = xQueueCreate(1, sizeof(songname_t));
  xTaskCreate(lcd_task, "lcd_task", (512U * 4) / sizeof(void *), NULL, 4, NULL);
}

void adc_setup() { xTaskCreate(adc_task, "adc_task", (512U * 4) / sizeof(void *), NULL, 3, NULL); }

int main(void) {
  create_blinky_tasks();
  create_uart_task();
  lcd_setup();
  pin_config();
  milestone_2_main();
  milestone_1_main();
  interrupt_setup();
  adc_setup();

  puts("Starting RTOS");

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

static void create_blinky_tasks(void) {
  /**
   * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
   * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
   */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes, !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
