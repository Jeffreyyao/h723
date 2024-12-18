#ifndef WS2812_H
#define WS2812_H

#include "ch.h"
#include "hal.h"

#define WS2812_PWM_DRIVER       PWMD5
#define WS2812_PWM_TIM_CHANNEL  0
#define WS2812_PWM_FREQUENCY    (4000000)
#define WS2812_PWM_PERIOD       (WS2812_PWM_FREQUENCY / 800000) // must be 800kHz
#define WS2812_PWM_WIDTH_0      (1)
#define WS2812_PWM_WIDTH_1      (4)
#define WS2812_LED_WIDTH        (16)
#define WS2812_LED_HEIGHT       (16)
#define WS2812_LED_NUM          (WS2812_LED_WIDTH * WS2812_LED_HEIGHT)

#define WS2812_COLOR_BITS       (WS2812_LED_NUM * 3 * 8)
#define WS2812_RESET_BITS       (50)
#define WS2812_BUFFER_SIZE_DMA  (WS2812_COLOR_BITS + WS2812_RESET_BITS)

extern uint32_t ws2812_frame_buffer_dma[WS2812_BUFFER_SIZE_DMA + 1];
extern const stm32_dma_stream_t* dma;
void ws2812_init(void);
void ws2812_fill_rgb(uint8_t r, uint8_t g, uint8_t b);
void ws2812_latch_grbarr(uint8_t *img_arr, int length);

#endif // WS2812_H