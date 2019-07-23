/*! \file main.c
 */

/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*!
 * \defgroup main Marionette main
 *
 * @{
 */

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "mshell.h"
#include "mpipe.h"
#include "board.h"

#include "fetch.h"
#include "fetch_adc.h"
#include "util_general.h"
#include "util_version.h"
#include "util_messages.h"
#include "util_io.h"
#include "usbcfg.h"


/*! \brief Show memory usage
 */
static bool cmd_mem(BaseSequentialStream * chp, int argc, char * argv[])
{
    extern uint8_t __ram0_size__[];

	size_t n, size;
	(void)argv;
	if (argc > 0)
	{
		util_message_error(chp, "extra arguments for command '+mem'");
		return false;
	}
	n = chHeapStatus(NULL, &size);
	util_message_info(chp, "core total memory :\t%u bytes", __ram0_size__ );
	util_message_info(chp, "core free memory  :\t%u bytes", chCoreGetStatusX());
	util_message_info(chp, "heap fragments    :\t%u", n);
	util_message_info(chp, "heap free total   :\t%u bytes", size);

  return true;
}

/*! \brief Show running threads
 */
static bool cmd_threads(BaseSequentialStream * chp, int argc, char * argv[])
{
	(void)chp;
	static const char * states[] = {CH_STATE_NAMES};
	thread_t * tp;

	(void)argv;
	if (argc > 0)
	{
		util_message_error(chp, "extra arguments for command '+threads'");
		return false;
	}
	util_message_info(chp, "addr\t\tstack\t\tprio\trefs\tstate\t\ttime\tname");
	tp = chRegFirstThread();
	do
	{
		util_message_info(chp, "%.8lx\t%.8lx\t%4lu\t%4lu\t%9s\t%lu\t%s",
		         (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
		         (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
		         states[tp->p_state], (uint32_t)tp->p_time, tp->p_name);
		tp = chRegNextThread(tp);
	}
	while (tp != NULL);

  return true;
}

/*! \brief MShell commands described in main
 * \sa MSHELL
 */
static const mshell_command_t commands[] =
{
	{cmd_mem,     "mem",      "Display memory usage"},
	{cmd_threads, "threads",  "Display threads"},
	{NULL, NULL, NULL}
};

/*! \brief MShell configuration
 * \sa MSHELL
 */
static const mshell_config_t mshell_cfg =
{
	(BaseAsynchronousChannel *) &SDU1,
  "m > ", // prompt
  true,   // show prompt
  true,   // echo chars
	commands
};

static const mpipe_config_t mpipe_cfg = 
{
  (BaseAsynchronousChannel *) &SDU2
};


/*! \brief main application loop
 */
static void main_app(void)
{
  chprintf(DEBUG_CHP, "Marionette start\r\n");
  set_status_led(1,0,0);

	fetch_init();
	mshell_init();
  mpipe_init();


#if STM32_USB_USE_OTG2
	palSetPad(GPIOB, GPIOB_ULPI_RST_B); //Take ulpi out of reset
	chThdSleepMilliseconds(200);
#else
	palClearPad(GPIOB, GPIOB_ULPI_RST_B); //Hold ulpi in reset
#endif

	usb_set_serial_strings( *(uint32_t*)STM32F4_UNIQUE_ID_LOW,
                          *(uint32_t*)STM32F4_UNIQUE_ID_CENTER,
                          *(uint32_t*)STM32F4_UNIQUE_ID_HIGH);

	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);
	sduObjectInit(&SDU2);
	sduStart(&SDU2, &serusbcfg2);

	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1000);
	usbStart(serusbcfg.usbp, &usbcfg);
	chThdSleepMilliseconds(200);
	usbConnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(200);
  
  uint8_t led_blank_count = 0;

	while (true)
	{
    if( serusbcfg.usbp->state == USB_ACTIVE )
    {
      mshell_start(&mshell_cfg);
      mpipe_start(&mpipe_cfg);
    }
    else
    {
      mshell_stop();
      mpipe_stop();
    }
    
    if( (++led_blank_count) >= 5 )
    {
      set_status_led(0,0,0);

      if(led_blank_count >= 10)
      {
        led_blank_count = 0;
      }
    }
    else if( serusbcfg.usbp->state == USB_ACTIVE )
    {
      set_status_led(0,1,0);
    }
    else
    {
      set_status_led(1,1,0);
    }
		chThdSleepMilliseconds(100);
	}
}

int main(void)
{
	halInit();
	chSysInit();

  // start up deboug output, chprintf(DEBUG_CHP,...)
  sdStart(&DEBUG_SERIAL, NULL);

	main_app();

	return(0);
}

//! @}

