/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: application.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 18/08/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NA.
*****************************************************************************/

/***** User defined Includes *****/
#include "application.h"
#include "user_uart.h"
#include "rs485_uart.h"
#include "rs485_driver.h"
#include "user_rtos.h"
#include "digital_input.h"
#include "relay_output.h"
#include "test_code.h"
#include "osdp_protocol_master.h"
#include "user_timer.h"
#include "led_operation.h"
#include "onboard_key.h"
#include "config_mode.h"
#include "ext_eeprom.h"

/**************************************/
#include "ethernet.h"
//#include "task_defs.h"
#include "tcp_server.h"
#include "general_application.h"

xSemaphoreHandle xMutex1;

/*****************************************************************************
* Function name	: void Driver_Initialization(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
*
* Description	:	All the drivers all initialized here.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Driver_Initialization(void)
{
	wdt_disable(WDT);				/* Disable WDT at power on */
	UART_Debug_Init();				/* Initialization of UART0 for debug communication. */
	Configure_Onboard_Key();		/* Configure port pin as an input and interrupt edge for inputs. */
	Configure_LED_PortPins();		/* Configure port pins for leds. */
	Timer_Init();					/* Configure timer to generate an interrupt at every 1 ms. */
	Digital_Input_Driver_Init();	/* Configure port pins as an input and interrupt edge for inputs. */
	Relay_Output_Driver_Init();		/* Configure port pins as an output & enable pull ups. */
	RS485_UART_Init();				/* Initialization of USART0 for RS485 communication. */
	RS485_Driver_Init();			/* Initialization of RS485 Driver. */
	configure_twi();                /* Configure twi(i2c) to write data to eeprom. */
	eeprom_pin_config();            /* Configure pin used for eeprom write protect. */
	enter_config_mode();            // Enter configuration mode.
	init_ethernet();				/* Configure the Ethernet driver */
	Configure_WDT();				/* Configure 3 sec's of WDT timer to prevent the software stuck. */
	
	Print_Message("\nWelcome Message.");
}



/*****************************************************************************
* Function name	: void Run_RTOS(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	:	Create tasks, semaphores, mutexes and set priority
*					and run scheduler.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Run_RTOS(void)
{
	Create_TCP_Server_Task();
	Create_Tasks();		// Create tasks and set priority.
	//xMutex1 = Create_Mutex();		// Create Mutex to Handle shared resources.
	Create_Binary_Semaphore();		// Create Binary Semaphore to access shared resource or synchronization.
	
	/* Start the scheduler. */
	vTaskStartScheduler();
}
