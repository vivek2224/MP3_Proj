#include <stdint.h>

#include "adc.h"

#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void adc__initialize(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__ADC);

  const uint32_t enable_adc_mask = (1 << 21);
  LPC_ADC->CR = enable_adc_mask;

  const uint32_t max_adc_clock = (12 * 1000UL * 1000UL); // 12.4Mhz : max ADC clock in datasheet for lpc40xx
  const uint32_t adc_clock = clock__get_peripheral_clock_hz();

  // APB clock divicer to support max ADC clock
  for (uint32_t divider = 2; divider < 255; divider += 2) {
    if ((adc_clock / divider) < max_adc_clock) {
      LPC_ADC->CR |= (divider << 8);
      break;
    }
  }
}

uint16_t adc__get_adc_value(adc_channel_e channel_num) {
  uint16_t result = 0;
  const uint16_t twelve_bits = 0x0FFF;
  const uint32_t channel_masks = 0xFF;
  const uint32_t start_conversion = (1 << 24);
  const uint32_t start_conversion_mask = (7 << 24); // 3bits - B26:B25:B24
  const uint32_t adc_conversion_complete = (1 << 31);

  if ((ADC__CHANNEL_2 == channel_num) || (ADC__CHANNEL_4 == channel_num) || (ADC__CHANNEL_5 == channel_num)) {
    LPC_ADC->CR &= ~(channel_masks | start_conversion_mask);
    // Set the channel number and start the conversion now
    LPC_ADC->CR |= (1 << channel_num) | start_conversion;

    while (!(LPC_ADC->GDR & adc_conversion_complete)) { // Wait till conversion is complete
      ;
    }
    result = (LPC_ADC->GDR >> 4) & twelve_bits; // 12bits - B15:B4
  }

  return result;
}

// My addition

void adc__initialize_mine(void) {
  // LPC_SC->PCONP &= ~(0xFFFFFFFF);
  LPC_SC->PCONP |= (1 << 12);
  LPC_ADC->CR |= (1 << 21);
  const uint32_t max_adc_clock = (12 * 1000UL * 1000UL); // 12.4Mhz : max ADC clock in datasheet for lpc40xx
  const uint32_t adc_clock = clock__get_peripheral_clock_hz();

  // APB clock divicer to support max ADC clock
  for (uint32_t divider = 2; divider < 255; divider += 2) {
    if ((adc_clock / divider) < max_adc_clock) {
      LPC_ADC->CR |= (divider << 8);
      break;
    }
  }

  LPC_IOCON->P1_31 |= (3 << 0);
  // LPC_ADC->CR |= (1 << 2);
  // LPC_ADC->CR &= ~((1 << 24) | (1 << 25) | (1 << 26));

  // clear start bits
  LPC_ADC->CR &= ~(1 << 24);
  LPC_ADC->CR &= ~(1 << 25);
  LPC_ADC->CR &= ~(1 << 26);
  // clear start bits

  // enable burst mode
  LPC_ADC->CR |= (1 << 16);
}

uint16_t adc__get_adc_valueTESTXX(adc_channel_e channel_num) {
  uint16_t result = 0;
  const uint16_t twelve_bits = 0x0FFF;
  const uint32_t channel_masks = 0xFF;
  const uint32_t start_conversion = (1 << 24);
  const uint32_t start_conversion_mask = (7 << 24); // 3bits - B26:B25:B24
  const uint32_t adc_conversion_complete = (1 << 31);

  if ((ADC__CHANNEL_2 == channel_num) || (ADC__CHANNEL_4 == channel_num) || (ADC__CHANNEL_5 == channel_num)) {
    LPC_ADC->CR &= ~(channel_masks | start_conversion_mask);
    // Set the channel number and start the conversion now
    LPC_ADC->CR |= (1 << channel_num) | start_conversion;
    result = 16;
    while (!(LPC_ADC->GDR & adc_conversion_complete)) { // Wait till conversion is complete
      ;
    }
    result = (LPC_ADC->GDR >> 4) & twelve_bits; // 12bits - B15:B4
  }
  // return 14;
  return result;
}

void adc__initializeTESTXX(void) {
  LPC_SC->PCONP |= (1 << 12); // gives power to adc
  LPC_ADC->CR |= (1 << 21);   // set pdn bit
  LPC_ADC->CR |= (0x7 << 8);  // clock is 96 Mhz, this divides it by 8 to make adc clock 12
  LPC_IOCON->P1_31 &= ~(1 << 7);
  LPC_IOCON->P1_31 |= (0x3 << 0); // sets pin 1.31 as ADC channel 5 pin
  LPC_ADC->CR |= (1 << 5);        // read from channel 5

  // clear start bits
  LPC_ADC->CR &= ~(1 << 24);
  LPC_ADC->CR &= ~(1 << 25);
  LPC_ADC->CR &= ~(1 << 26);

  // set burst bit
  LPC_ADC->CR |= (1 << 16);
}

void adc__enable_burst_mode(adc_channel_e channel_num) {
  LPC_ADC->CR |= (1 << channel_num);
  LPC_ADC->CR &= ~((1 << 24) | (1 << 25) | (1 << 26));
  LPC_ADC->CR |= (1 << 16);
}

uint16_t adc__get_channel_reading_with_burst_mode() {
  uint16_t result = 5;
  // if ((LPC_ADC->DR[5] & (1 << 31))) {
  result = 4;
  // fprintf(stderr, "inside");
  result = LPC_ADC->DR[5] >> 4;
  // }
  return result;
}
