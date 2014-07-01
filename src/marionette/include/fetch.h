/*! \file fetch.h
  * Support implementation of Fetch Language
  */

#ifndef FETCH_H_
#define FETCH_H_

// Defines
#define 		FETCH_MAX_COMMANDS 						8
#define 		FETCH_MAX_DATA_ITEMS					8
#define 		FETCH_MAX_LINE_CHARS					256
#define 		FETCH_MAX_CMD_STRLEN					25

#define 		HELP_MAX_DATA_BYTES 					0
#define 		HELP_MAX_DATA_BYTES 					0
#define         HELP_HELPSTRING                         "\r\n"

#define 		GPIO_MAX_DATA_BYTES 					0
#define 		GPIO_HELPSTRING   					    ((const char *) "\r\n\tGPIO:\tgpio:<set,clear,configure>:port:pin:<input,output>:<pullup,pulldown,floating,analog>")

#define 		RESETPINS_MAX_DATA_BYTES 				0
#define 		RESETPINS_HELPSTRING   					((const char *) "\r\n\tRESETPINS:\tresetpins")

// Support dictionaries
typedef struct help_command_dictionary
{
	bool     	   enabled;
	uint16_t 	   max_data_bytes;
	const char  *  helpstring;
} HELP_command_dictionary;

typedef struct gpio_command_dictionary
{
	bool     	   enabled;
	uint16_t 	   max_data_bytes;
	const char  *  helpstring;
} GPIO_command_dictionary;

typedef struct resetpins_command_dictionary
{
	bool     	   enabled;
	uint16_t 	   max_data_bytes;
	const char  *  helpstring;
} RESETPINS_command_dictionary;

// Functions
void fetch_init(BaseSequentialStream *  chp) ;
bool fetch_parse(BaseSequentialStream* chp, char * inputline);
bool fetch_dispatch(BaseSequentialStream* chp, char * command_list[], char * data_list[]);

#endif
