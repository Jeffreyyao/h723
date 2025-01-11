#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
#include "usbcfg.h"
#include "ws2812.h"
#include "mlx90640/MLX90640_I2C_Driver.h"
#include "mlx90640/MLX90640_API.h"

static bool blink = true;
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  palSetLine(LINE_LED);
  while (true) {
    if (blink)
      palToggleLine(LINE_LED);
    else
      palSetLine(LINE_LED);
    chThdSleepMilliseconds(200);
  }
}

#define MLX90640_ADDR 0x33
#define RefreshRate 0x07
#define TA_SHIFT 8
static uint16_t eeMLX90640[832];  
static float mlx90640To[768];
uint16_t frame[834];
float emissivity=0.95;
int status;

static THD_WORKING_AREA(waThreadMLX, 2048);
static THD_FUNCTION(ThreadMLX, arg) {
  (void)arg;
  chRegSetThreadName("mlx90640");

  MLX90640_I2CInit();
  MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate);
	MLX90640_SetChessMode(MLX90640_ADDR);
	paramsMLX90640 mlx90640;
  status = MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640);
  if (status != 0) chprintf((BaseSequentialStream *)&SDU1, "\r\nload system parameters error with code:%d\r\n",status);
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0) chprintf((BaseSequentialStream *)&SDU1, "\r\nParameter extraction failed with error code:%d\r\n",status);

  while (true) {
    int status = MLX90640_GetFrameData(MLX90640_ADDR, frame);
		if (status < 0)
		{
			chprintf((BaseSequentialStream *)&SDU1, "GetFrame Error: %d\r\n",status);
		}
		float vdd = MLX90640_GetVdd(frame, &mlx90640);
		float Ta = MLX90640_GetTa(frame, &mlx90640);

		float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
		chprintf((BaseSequentialStream *)&SDU1, "vdd:  %f Tr: %f\r\n",vdd,tr);
		MLX90640_CalculateTo(frame, &mlx90640, emissivity , tr, mlx90640To);

		chprintf((BaseSequentialStream *)&SDU1, "\r\n==========================\r\n");
		for(int i = 0; i < 768; i++){
			if(i%32 == 0 && i != 0){
				chprintf((BaseSequentialStream *)&SDU1, "\r\n");
			}
			chprintf((BaseSequentialStream *)&SDU1, "%2.2f ", mlx90640To[i]);
		}
    chprintf((BaseSequentialStream *)&SDU1, "\r\n==========================\r\n");
    chThdSleepMilliseconds(1000);
  }
}

#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2048)

static void cmd_clock(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv; (void)argc;
  chprintf(chp, "STM32_SYS_CK: %d\r\n", STM32_SYS_CK);
  chprintf(chp, "STM32_HCLK: %d\r\n", STM32_HCLK);
  chprintf(chp, "STM32_HSECLK: %d\r\n", STM32_HSECLK);
}

static void cmd_blink(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)chp; (void)argv; (void)argc;
  blink = !blink;
}

static void cmd_ws2812(BaseSequentialStream *chp, int argc, char *argv[]) {
  if (argc == 3) {
    uint8_t r = atoi(argv[0]);
    uint8_t g = atoi(argv[1]);
    uint8_t b = atoi(argv[2]);
    ws2812_fill_rgb(g, r, b);
  }
}

static void cmd_mlx90640(BaseSequentialStream *chp, int argc, char *argv[]) {
  chprintf((BaseSequentialStream *)&SDU1, "\r\ninit i2c\r\n");
  MLX90640_I2CInit();
  chprintf((BaseSequentialStream *)&SDU1, "\r\nset refresh rate\r\n");
  MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate);
  chprintf((BaseSequentialStream *)&SDU1, "\r\nset chess mode\r\n");
	MLX90640_SetChessMode(MLX90640_ADDR);
	paramsMLX90640 mlx90640;
  chprintf((BaseSequentialStream *)&SDU1, "\r\nload system parameters\r\n");
  status = MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640);
  if (status != 0) chprintf((BaseSequentialStream *)&SDU1, "\r\nload system parameters error with code:%d\r\n",status);
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0) chprintf((BaseSequentialStream *)&SDU1, "\r\nParameter extraction failed with error code:%d\r\n",status);
  int status = MLX90640_GetFrameData(MLX90640_ADDR, frame);
  if (status < 0)
  {
    chprintf((BaseSequentialStream *)&SDU1, "GetFrame Error: %d\r\n",status);
  }
  float vdd = MLX90640_GetVdd(frame, &mlx90640);
  float Ta = MLX90640_GetTa(frame, &mlx90640);

  float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
  chprintf((BaseSequentialStream *)&SDU1, "vdd:  %f Tr: %f\r\n",vdd,tr);
  MLX90640_CalculateTo(frame, &mlx90640, emissivity , tr, mlx90640To);

  chprintf((BaseSequentialStream *)&SDU1, "\r\n==========================\r\n");
  for(int i = 0; i < 768; i++){
    if(i%32 == 0 && i != 0){
      chprintf((BaseSequentialStream *)&SDU1, "\r\n");
    }
    chprintf((BaseSequentialStream *)&SDU1, "%2.2f ", mlx90640To[i]);
  }
  chprintf((BaseSequentialStream *)&SDU1, "\r\n==========================\r\n");
}

static const ShellCommand commands[] = {
  {"clock", cmd_clock},
  {"blink", cmd_blink},
  {"ws2812", cmd_ws2812},
  {"mlx90640", cmd_mlx90640},
  {NULL, NULL},
};

char shell_history[SHELL_MAX_HIST_BUFF];
char *shell_completions[SHELL_MAX_COMPLETIONS];

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands,
  shell_history,
  sizeof(shell_history),
  shell_completions,
};

int main(void) {
  halInit();
  chSysInit();

  // shell
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);
  shellInit();

  // ws2812 rgb
  ws2812_init();

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  // chThdCreateStatic(waThreadMLX, sizeof(waThreadMLX), NORMALPRIO, ThreadMLX, NULL);

  while (true) {
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE, "shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg1);
    chThdWait(shelltp); /* Waiting termination. */
    chThdSleepMilliseconds(500);
  }
}
