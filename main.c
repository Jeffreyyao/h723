#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
#include "usbcfg.h"
#include "ws2812.h"

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

static const ShellCommand commands[] = {
  {"clock", cmd_clock},
  {"blink", cmd_blink},
  {"ws2812", cmd_ws2812},
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

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  while (true) {
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE, "shell", NORMALPRIO + 1, shellThread, (void *)&shell_cfg1);
    chThdWait(shelltp); /* Waiting termination. */
    chThdSleepMilliseconds(500);
  }
}
