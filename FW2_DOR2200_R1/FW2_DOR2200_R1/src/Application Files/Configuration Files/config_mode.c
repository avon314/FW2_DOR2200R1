/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: config_mode.c
* Created By	: Harshit Agnihotri.
* Created Date	: 20/03/2024.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 20/03/2024.
* Changes		: NA.
/*****************************************************************************/

/* System Includes */
#include "asf.h"

/* User Includes */
#include "config_mode.h"
#include "user_uart.h"
#include "app_utility.h"
#include "digital_ip_app.h"
#include "app_osdp.h"
#include "interlock.h"
#include "app_eeprom.h"
#include "definitions.h"

U8 gb_device_power_on = 0;

volatile U8 gb_config_mode_f = false;             // Configuration mode flag.
volatile U8 gb_cfg_mode_keyPressed = false;            // To check entered in config mode or not.
volatile U8 gb_exit_from_cfg_f = false;	// flag to check after exit from config mode, key released or not?

volatile int gb_config_timer = 0;                  // Configuration timer.
volatile U8 gb_config_timer_running_f = false;    // Configuration timer flag.

volatile U16 gb_config_led_timer = 0;             // Configuration LED timer.
volatile U8 gb_config_led_on_f = false;           // Configuration LED flag.

volatile U8 gb_config_rst_key_f = 0;              // Configuration mode reset key flag.
volatile int gb_config_rst_key_timer = false;     // Configuration mode reset key timer.

extern void Read_Memory_For_PowerON_State(void);

/*****************************************************************************
* Function name: void config_mode_init(void).
* Returns		: nothing.
* Arguments    : None.
* Created by	: Harshit Agnihotri.
* Date created	: 20/03/2024.
*
* Description	: Function to initialize the key used to enter configuration mode.
*              :
* Notes		: NA.
* Global Variables Affected : .
*****************************************************************************/
 void config_mode_init(void)
 {
     REG_CCFG_SYSIO = CCFG_SYSIO_SYSIO10;         // Instead of system pin, configure PB10 as I/O.
     pmc_enable_periph_clk(ID_PIOB);              /* Enable Peripheral clock */
     pio_set_input(PIOB, PIO_PB10, PIO_PULLUP);   // Set as input pin.
 }

 /*****************************************************************************
 * Function name: void config_mode_key_detect(void).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Function to enter configuration mode by detecting whether configuration mode key is pressed at power on and set config mode flag.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f.
 *****************************************************************************/
 void enter_config_mode(void)
 {
     if (!pio_get(PIOB, PIO_TYPE_PIO_INPUT, PIO_PB10))      // Check if key is pressed at power on.
     {
          gb_config_mode_f = true;                         // Set configuration mode flag.
			
			if (gb_cfg_mode_keyPressed != 2)
			{
				/*****
				Set below flag to indicate that key pressed first time.
				*****/
				gb_cfg_mode_keyPressed = 1;
			}
			
			
			
          #if DEBUG_ALL || DEBUG_CONFIG_MODE
          Print_Message("\nIn the configuration mode, key pressed at power on.");
          #endif

     }
     else
     {
          gb_config_mode_f = false;                         // If not reset configuration mode flag.

          #if DEBUG_ALL || DEBUG_CONFIG_MODE
          Print_Message("\nNot in configuration mode, key not pressed at power on.");
          #endif

          gb_device_power_on = 1;
     }

	read_config_eeprom();                                  // Read all configuration data from EEPROM.
	Read_Memory_For_PowerON_State();
 }

  /*****************************************************************************
 * Function name: void exit_config_mode(void).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Function to exit configuration mode by reseting the configuration mode flag and connect with device flag.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f, gb_conn_dvc_f.
 *****************************************************************************/
 void exit_config_mode(void)
 {
	/*	Write the configuration while gb_config_mode_f is still set. Clearing it
		first restarted the application in General_Task (higher priority) in
		the middle of this write; its EEPROM accesses then interleaved with the
		configuration write and corrupted it. */
	write_config_eeprom();                            // Write all configuration data to EEPROM.

     gb_config_mode_f = false;                         // Exit configuration mode by reseting configuration mode flag and connect with device flag.
     gb_conn_dvc_f = false;

     #if DEBUG_ALL || DEBUG_CONFIG_MODE
     Print_Message("\nOut of configuration mode.");
     #endif

	gb_device_power_on = 1;
	ON_RED_LED;
	
	// Trigger a software reset
	RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
 }

 /*****************************************************************************
 * Function name: void config_mode(void *pvParameters).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Main logic of configuration mode.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f.
 *****************************************************************************/
 void config_mode(void *pvParameters)
 {
     /* Just to avoid compiler warnings. */
     UNUSED(pvParameters);

     while(1)
     {
          if(gb_config_mode_f)                                                  // Check if configuration mode flag is set.
          {
               if(gb_config_timer_running_f == false)                           // Check if configuration mode timer is running.
               {
                    gb_config_timer = 0;                                        // Start the configuration mode timer and configuration LED timer and set the configuration mode timer running flag.
                    gb_config_led_timer = 0;
                    gb_config_timer_running_f = true;
               }
               else if(gb_config_timer_running_f == true && gb_utility_rcv_f == false && gb_config_timer >= CONFIG_FRAME_RCV_TIME_OUT)      // Check if configuration mode timer running flag is set and utility receive flag is not set and utility receive frame timeout value.
               {
				   /*****
				   Exit from configuration due to no frame
				   *****/
				   gb_cfg_mode_keyPressed = 0;
                    exit_config_mode();                                         // Exit configuration mode.
               }




               if((gb_config_led_on_f == false) && (gb_config_led_timer >= CONFIG_LED_TOGGLE_TIME))      // Toggle configuration LED for every CONFIG_LED_TOGGLE_TIME ms.
               {
                    gb_config_led_on_f = true;
                    gb_config_led_timer = 0;
                    ON_RED_LED;
               }
               else if((gb_config_led_on_f == true) && (gb_config_led_timer >= CONFIG_LED_TOGGLE_TIME))
               {
                    gb_config_led_on_f = false;
                    gb_config_led_timer = 0;
                    OFF_RED_LED;
               }


               if((!pio_get(PIOB, PIO_TYPE_PIO_INPUT, PIO_PB10)) && (gb_config_rst_key_f == false))      // Check if configuration mode key is pressed continuously for CONFIG_RST_KEY_TIME and exit configuration mode or else reload the timer.
               {
                    gb_config_rst_key_f = true;
                    gb_config_rst_key_timer = 0;
					
					if (gb_cfg_mode_keyPressed == 2)
					{
						/* Key Pressed second time in config mode */
					}
               }
               else if(pio_get(PIOB, PIO_TYPE_PIO_INPUT, PIO_PB10))
               {
                    gb_config_rst_key_f = false;
					
					if (gb_cfg_mode_keyPressed == 1)
					{
						/*****
						Key released first time, in config mode
						*****/
						gb_cfg_mode_keyPressed = 2;
					}
               }
               else if((gb_config_rst_key_f == true) && (gb_config_rst_key_timer >= CONFIG_RST_KEY_TIME))
               {
                    gb_config_rst_key_f = false;
					gb_cfg_mode_keyPressed = 0;
                    
					/*****
					Set a below flag to exit from configuration mode & and wait for key release. (check below if() routine.)
					*****/
					gb_exit_from_cfg_f = true;
					ON_RED_LED;	// Set a steady red led to indicate that, it is ready to come out from config mode.
               }
			   
			   if ((gb_exit_from_cfg_f == true) && (pio_get(PIOB, PIO_TYPE_PIO_INPUT, PIO_PB10)))
			   {
				   gb_exit_from_cfg_f = false;
				   
				   /*****
				   Exit from config mode after timer completed and key released.
				   *****/
				   exit_config_mode();
			   }
          }
          vTaskDelay(1);
     }
 }