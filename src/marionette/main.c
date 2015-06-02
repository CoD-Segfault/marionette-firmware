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



#include "board.h"

#include "fetch.h"
#include "fetch_adc.h"
#include "util_general.h"
#include "util_version.h"
#include "util_messages.h"
#include "usbcfg.h"
#include "mshell.h"
#include "main.h"

/*! Virtual serial port over USB.*/
SerialUSBDriver SDU2;

//extern const struct led        LED1 ;
//extern const struct led        LED2 ;
//extern const struct led        LED3 ;
//extern const struct led        LED4 ;

//extern       struct led_config led_cfg ;

/*! status
 */
static WORKING_AREA(pwm_wa, 128);
Util_status      M_Status = { .status=GEN_OK };

//#define SHELL_WA_SIZE   THD_WA_SIZE(16384)
#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

/*! \brief Show memory usage
 */
static bool cmd_mem(BaseSequentialStream * chp, int argc, char * argv[])
{
    extern uint8_t __ram_size__[];

	size_t n, size;
	(void)argv;
	if (argc > 0)
	{
		util_message_error(chp, "extra arguments for command '+mem'");
		return false;
	}
	n = chHeapStatus(NULL, &size);
	util_message_info(chp, "core total memory :\t%u bytes", __ram_size__ );
	util_message_info(chp, "core free memory  :\t%u bytes", chCoreStatus());
	util_message_info(chp, "heap fragments    :\t%u", n);
	util_message_info(chp, "heap free total   :\t%u bytes", size);

  return true;
}

/*! \brief Show running threads
 */
static bool cmd_threads(BaseSequentialStream * chp, int argc, char * argv[])
{
	(void)chp;
	static const char * states[] = {THD_STATE_NAMES};
	Thread * tp;

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
static const MShellConfig shell_cfg1 =
{
	(BaseSequentialStream *) &SDU2,
	commands
};
/*! \brief PWM configuration for ULPI clock
 */
static PWMConfig pwmcfg = {
	168000000,
	14,
	NULL,
	{
	 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL},
	 {PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH, NULL},
	 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL},
	 {PWM_COMPLEMENTARY_OUTPUT_DISABLED, NULL}
	},
	0,
	0,
	0
};
static msg_t pwm_thd(void *arg) {
	pwmEnableChannel(&PWMD1, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD1,5000));	
//	while(TRUE) {
//	pwmEnableChannel(&PWMD8, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD8,5000));	
//	chThdSleepMilliseconds(1500);	
//Do nothing
//	}
	//chSysLock();
	return(0);
}
/*! \brief main application loop
 */
static void main_app(void)
{
	Thread             *            mshelltp = NULL;
	Thread *pwmtp = NULL;
	//Start 12 MHz PWM for ULPI
	pwmStart(&PWMD1, &pwmcfg);
//	palSetPadMode(GPIOB, GPIOB_PIN14, PAL_MODE_ALTERNATE(1));
	palSetPadMode(GPIOE, GPIOE_PIN10, PAL_MODE_ALTERNATE(1));
//	pwmEnableChannel(&PWMD8, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD8,5000));
	pwmtp = chThdCreateStatic(pwm_wa,sizeof(pwm_wa),NORMALPRIO, pwm_thd, NULL);
	mshellInit();

  
	usb_set_serial_strings( *(uint32_t*)STM32F4_UNIQUE_ID_LOW,
                          *(uint32_t*)STM32F4_UNIQUE_ID_CENTER,
                          *(uint32_t*)STM32F4_UNIQUE_ID_HIGH);

	sduObjectInit(&SDU2);
	sduStart(&SDU2, &serusbcfg);

	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1500);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);
	#if STM32_SERIAL_USE_USART2
  	sdStart(&SD2, NULL);
  /*Note: do not use GPIOA_PIN3 as it conflicts with ULPI pins.*/
	#endif

	#if STM32_SERIAL_USE_USART3
	  sdStart(&SD3, NULL);
	#endif

	//pwmtp = chThdCreateStatic(pwm_wa,sizeof(pwm_wa),NORMALPRIO, pwm_thd, NULL);

	while (true)
	{
		if (!mshelltp)
		{
			if (SDU2.config->usbp->state == USB_ACTIVE)
			{
				mshelltp = mshellCreate(&shell_cfg1, SHELL_WA_SIZE, HIGHPRIO);
			}
		}
		else
		{
			if (chThdTerminated(mshelltp))
			{
				chThdRelease(mshelltp);
				mshelltp = NULL;
			}
		}
		chThdSleepMilliseconds(500);
		palTogglePad(GPIOD, GPIOD_LED1_GREEN);
	}
}

int main(void)
{
	halInit();
	chSysInit();

	//Set up to toggle leds 
        palSetPadMode(GPIOD, GPIOD_LED1_RED,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOD, GPIOD_LED1_GREEN,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOD, GPIOD_LED1_BLUE,PAL_MODE_OUTPUT_PUSHPULL);
        palSetPadMode(GPIOD, GPIOD_LED2,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED3,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED4,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED5,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED6,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED7,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED8,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOG, GPIOG_LED9,PAL_MODE_OUTPUT_PUSHPULL);
	palSetPadMode(GPIOB, GPIOB_PIN4,PAL_MODE_OUTPUT_PUSHPULL);
	//Toggle leds and test	
	/*palTogglePad(GPIOD,GPIOD_LED1_RED);
	palTogglePad(GPIOD,GPIOD_LED1_GREEN);
	palTogglePad(GPIOD,GPIOD_LED1_BLUE);*/
	//while(1)
	//{
	palTogglePad(GPIOD,GPIOD_LED2);
	chThdSleepMilliseconds(500);

	palTogglePad(GPIOG,GPIOG_LED3);
	chThdSleepMilliseconds(500);

	palTogglePad(GPIOG,GPIOG_LED4);
	chThdSleepMilliseconds(500);
	palTogglePad(GPIOG,GPIOG_LED5);
	chThdSleepMilliseconds(500);
	palTogglePad(GPIOG,GPIOG_LED6);
	chThdSleepMilliseconds(500);

	palTogglePad(GPIOG,GPIOG_LED7);
	chThdSleepMilliseconds(500);

	palTogglePad(GPIOG,GPIOG_LED8);
	chThdSleepMilliseconds(500);

	palTogglePad(GPIOG,GPIOG_LED9);
	chThdSleepMilliseconds(500);
	palSetPad(GPIOB, GPIOB_PIN4);
//	}
	main_app();
	return(0);
}

//! @}

