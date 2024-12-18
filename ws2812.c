#include "ws2812.h"
#include "ws2812_piximg.h"

static const PWMConfig pwmcfg = {
  .frequency = WS2812_PWM_FREQUENCY,
  .period = WS2812_PWM_PERIOD,
  .callback = NULL,
  .channels ={
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}
  },
  .cr2 = 0,
  .bdtr = 0,
  .dier = TIM_DIER_UDE
};

uint32_t ws2812_frame_buffer_dma[WS2812_BUFFER_SIZE_DMA + 1] __attribute__((section(".ram_d2_sram1"))); // fuck this
const stm32_dma_stream_t* dma;

void ws2812_init(void) {
  dma = dmaStreamAlloc(STM32_DMA_STREAM_ID_ANY_DMA1, 10, NULL, NULL);
  dmaSetRequestSource(dma, STM32_DMAMUX1_TIM5_UP);
  dmaStreamSetPeripheral(dma, &(WS2812_PWM_DRIVER.tim->CCR[WS2812_PWM_TIM_CHANNEL]));
  dmaStreamSetMemory0(dma, ws2812_frame_buffer_dma);
  dmaStreamSetTransactionSize(dma, WS2812_BUFFER_SIZE_DMA);
  dmaStreamSetMode(dma,
    STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_PSIZE_WORD | STM32_DMA_CR_MSIZE_WORD |
    STM32_DMA_CR_MINC | STM32_DMA_CR_PL(3));

  pwmStart(&WS2812_PWM_DRIVER, &pwmcfg);
  pwmEnableChannel(&WS2812_PWM_DRIVER, WS2812_PWM_TIM_CHANNEL, 0);

  for (int i = 0; i < WS2812_BUFFER_SIZE_DMA; i++) {
    ws2812_frame_buffer_dma[i] = 0;
  }
}

void ws2812_fill_rgb(uint8_t r, uint8_t g, uint8_t b) {
  int size = WS2812_LED_WIDTH * WS2812_LED_HEIGHT;
  uint8_t img_arr[size * 3];
  for (int i = 0; i < size; i++) {
    img_arr[i * 3] = g;
    img_arr[i * 3 + 1] = r;
    img_arr[i * 3 + 2] = b;
  }
  ws2812_latch_grbarr(img_arr, size * 3);
}

void ws2812_latch_grbarr(uint8_t *img_arr, int length) {
  int fb_idx = 0;
  for (int i = 0; i < length; i++) {
    for (int j = 0; j < 8; j++) {
      if (fb_idx >= WS2812_COLOR_BITS) {
        return;
      }
      ws2812_frame_buffer_dma[fb_idx] = (img_arr[i] & (128 >> j)) ? WS2812_PWM_WIDTH_1 : WS2812_PWM_WIDTH_0;
      fb_idx++;
    }
  }
  dmaStreamClearInterrupt(dma);
  dmaStreamEnable(dma);
}