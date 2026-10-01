/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: op_func.c
* Created By	: Harshit Agnihotri.
* Created Date	: 16/03/2024.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 16/03/2024.
* Changes		: NA.
/*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "op_func.h"
#include "output_config.h"
#include "relay_output.h"
#include "user_uart.h"
#include "app_utility.h"
#include "definitions.h"

#define SET_BIT(num, bitposition) ((*num) |= ((1u) << (bitposition)))
#define CLEAR_BIT(num, bitposition)  ((*num) &= (~((1u) << (bitposition))))
#define IS_BIT_SET(num, bitposition)  (((num) & ((1u) << (bitposition))) != (0))

// External definitions for door and operational states.
volatile U8 door_state[MAX_DOORS];
volatile U8 operational_state[MAX_DOORS];

// External definitions for momentary and toggle set/reset flags.
volatile U8 gb_op_mom_set_reset_f = false;
volatile U8 gb_op_tog_set_reset_f = false;

// External definitions for momentary timer running flag and timer array.
volatile U8 gb_op_mom_timer_running = false;
volatile U16 gb_op_mom_timer[4];

// External definition for momentary pulse flag.
volatile U8 gb_op_mom_pulse = false;

// External definitions for test index, frame received flag, and test frame buffer.
volatile U8 gb_op_test_idx = 0;
volatile U8 gb_op_test_frame_received_f = false;
volatile U8 gb_op_test_frame_buff[2];

/*****************************************************************************
* Function name: void Output_1_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 16/03/2024.
*
* Description	: This function is responsible for managing output 1 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_set_reset_f, gb_op_tog_set_reset_f, gb_op_mom_pulse.
*****************************************************************************/
void Output_1_Functionality(void)
{
     if(gb_output[OP_1_IDX].op_en)                                              // Check if output is enabled.
     {
          if(gb_output[OP_1_IDX].op_dflt_state == STATE_NO)                           // Check if the default state of output is set to NO.
          {
               if(gb_output[OP_1_IDX].op_type == OP_TYPE_MOMENTARY)             // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function) || (operational_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_1_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_1_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_1_IDX);
                    }
                    else if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function) && (operational_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_1_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_1_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_1_IDX);
                    }
               }
               else if(gb_output[OP_1_IDX].op_type == OP_TYPE_TOGGLE)           // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function) || (operational_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_1_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_1_IDX);
                         Operate_Relay(RELAY1, ON);
                    }
                    else if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function) && (operational_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_1_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_1_IDX);
                         Operate_Relay(RELAY1, OFF);
                    }
               }
          }
          else if(gb_output[OP_1_IDX].op_dflt_state == STATE_NC)                      // Check if the default state of output is set to NC.
          {
                if(gb_output[OP_1_IDX].op_type == OP_TYPE_MOMENTARY)            // If output type is MOMENTARY.
                {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                     if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function) || (operational_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_1_IDX) == false))
                     {
                          SET_BIT(&gb_op_mom_pulse, OP_1_IDX);
                          SET_BIT(&gb_op_mom_set_reset_f, OP_1_IDX);
                     }
                     else if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function) && (operational_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_1_IDX) == true))
                     {
                          //SET_BIT(&gb_op_mom_pulse, OP_1_IDX);
                          CLEAR_BIT(&gb_op_mom_set_reset_f, OP_1_IDX);
                     }
                }
                else if(gb_output[OP_1_IDX].op_type == OP_TYPE_TOGGLE)          // If output type is TOGGLE.
                {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                     if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function) || (operational_state[(gb_output[OP_1_IDX].door_number) - 1] == gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_1_IDX) == false))
                     {
                          SET_BIT(&gb_op_tog_set_reset_f, OP_1_IDX);
                          Operate_Relay(RELAY1, OFF);
                     }
                     else if(((door_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function) && (operational_state[(gb_output[OP_1_IDX].door_number) - 1] != gb_output[OP_1_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_1_IDX) == true))
                     {
                          CLEAR_BIT(&gb_op_tog_set_reset_f, OP_1_IDX);
                          Operate_Relay(RELAY1, ON);
                     }
                }
          }
     }
}

/*****************************************************************************
* Function name: void operate_relay_1_mom(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for giving a momentary pulse on output 1.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_timer_running, gb_op_mom_timer, gb_op_mom_pulse.
*****************************************************************************/
void operate_relay_1_mom(void)
{
     if(gb_output[OP_1_IDX].op_dflt_state == STATE_NO)                                // Check if default state of output is NO.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_1_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_1_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_1_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_1_IDX);
                    Operate_Relay(RELAY1, ON);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_1_IDX) == true) && (gb_op_mom_timer[OP_1_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_1_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_1_IDX);
                    Operate_Relay(RELAY1, OFF);
               }
          }
     }
     else if(gb_output[OP_1_IDX].op_dflt_state == STATE_NC)                           // Check if default state of output is NC.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_1_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_1_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_1_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_1_IDX);
                    Operate_Relay(RELAY1, OFF);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_1_IDX) == true) && (gb_op_mom_timer[OP_1_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_1_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_1_IDX);
                    Operate_Relay(RELAY1, ON);
               }
          }
     }
}

/*****************************************************************************
* Function name: void Output_2_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 16/03/2024.
*
* Description	: This function is responsible for managing output 2 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_set_reset_f, gb_op_tog_set_reset_f, gb_op_mom_pulse.
*****************************************************************************/
void Output_2_Functionality(void)
{
     if(gb_output[OP_2_IDX].op_en)                                              // Check if output is enabled.
     {
          if(gb_output[OP_2_IDX].op_dflt_state == STATE_NO)                           // Check if the default state of output is set to NO.
          {
               if(gb_output[OP_2_IDX].op_type == OP_TYPE_MOMENTARY)             // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function) || (operational_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_2_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_2_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_2_IDX);
                    }
                    else if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function) && (operational_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_2_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_2_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_2_IDX);
                    }
               }
               else if(gb_output[OP_2_IDX].op_type == OP_TYPE_TOGGLE)           // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function) || (operational_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_2_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_2_IDX);
                         Operate_Relay(RELAY2, ON);
                    }
                    else if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function) && (operational_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_2_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_2_IDX);
                         Operate_Relay(RELAY2, OFF);
                    }
               }
          }
          else if(gb_output[OP_2_IDX].op_dflt_state == STATE_NC)                      // Check if the default state of output is set to NC.
          {
               if(gb_output[OP_2_IDX].op_type == OP_TYPE_MOMENTARY)            // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function) || (operational_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_2_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_2_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_2_IDX);
                    }
                    else if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function) && (operational_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_2_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_2_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_2_IDX);
                    }
               }
               else if(gb_output[OP_2_IDX].op_type == OP_TYPE_TOGGLE)          // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function) || (operational_state[(gb_output[OP_2_IDX].door_number) - 1] == gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_2_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_2_IDX);
                         Operate_Relay(RELAY2, OFF);
                    }
                    else if(((door_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function) && (operational_state[(gb_output[OP_2_IDX].door_number) - 1] != gb_output[OP_2_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_2_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_2_IDX);
                         Operate_Relay(RELAY2, ON);
                    }
               }
          }
     }
}

/*****************************************************************************
* Function name: void operate_relay_2_mom(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for giving a momentary pulse on output 2.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_timer_running, gb_op_mom_timer, gb_op_mom_pulse.
*****************************************************************************/
void operate_relay_2_mom(void)
{
     if(gb_output[OP_2_IDX].op_dflt_state == STATE_NO)                                // Check if default state of output is NO.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_2_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_2_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_2_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_2_IDX);
                    Operate_Relay(RELAY2, ON);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_2_IDX) == true) && (gb_op_mom_timer[OP_2_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_2_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_2_IDX);
                    Operate_Relay(RELAY2, OFF);
               }
          }
     }
     else if(gb_output[OP_2_IDX].op_dflt_state == STATE_NC)                           // Check if default state of output is NC.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_2_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_2_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_2_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_2_IDX);
                    Operate_Relay(RELAY2, OFF);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_2_IDX) == true) && (gb_op_mom_timer[OP_2_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_2_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_2_IDX);
                    Operate_Relay(RELAY2, ON);
               }
          }
     }
}

/*****************************************************************************
* Function name: void Output_3_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 16/03/2024.
*
* Description	: This function is responsible for managing output 3 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_set_reset_f, gb_op_tog_set_reset_f, gb_op_mom_pulse.
*****************************************************************************/
void Output_3_Functionality(void)
{
     if(gb_output[OP_3_IDX].op_en)                                              // Check if output is enabled.
     {
          if(gb_output[OP_3_IDX].op_dflt_state == STATE_NO)                           // Check if the default state of output is set to NO.
          {
               if(gb_output[OP_3_IDX].op_type == OP_TYPE_MOMENTARY)             // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function) || (operational_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_3_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_3_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_3_IDX);
                    }
                    else if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function) && (operational_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_3_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_3_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_3_IDX);
                    }
               }
               else if(gb_output[OP_3_IDX].op_type == OP_TYPE_TOGGLE)           // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function) || (operational_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_3_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_3_IDX);
                         Operate_Relay(RELAY3, ON);
                    }
                    else if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function) && (operational_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_3_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_3_IDX);
                         Operate_Relay(RELAY3, OFF);
                    }
               }
          }
          else if(gb_output[OP_3_IDX].op_dflt_state == STATE_NC)                      // Check if the default state of output is set to NC.
          {
               if(gb_output[OP_3_IDX].op_type == OP_TYPE_MOMENTARY)            // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function) || (operational_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_3_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_3_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_3_IDX);
                    }
                    else if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function) && (operational_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_3_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_3_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_3_IDX);
                    }
               }
               else if(gb_output[OP_3_IDX].op_type == OP_TYPE_TOGGLE)          // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function) || (operational_state[(gb_output[OP_3_IDX].door_number) - 1] == gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_3_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_3_IDX);
                         Operate_Relay(RELAY3, OFF);
                    }
                    else if(((door_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function) && (operational_state[(gb_output[OP_3_IDX].door_number) - 1] != gb_output[OP_3_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_3_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_3_IDX);
                         Operate_Relay(RELAY3, ON);
                    }
               }
          }
     }
}

/*****************************************************************************
* Function name: void operate_relay_3_mom(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for giving a momentary pulse on output 3.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_timer_running, gb_op_mom_timer, gb_op_mom_pulse.
*****************************************************************************/
void operate_relay_3_mom(void)
{
     if(gb_output[OP_3_IDX].op_dflt_state == STATE_NO)                                // Check if default state of output is NO.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_3_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_3_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_3_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_3_IDX);
                    Operate_Relay(RELAY3, ON);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_3_IDX) == true) && (gb_op_mom_timer[OP_3_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_3_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_3_IDX);
                    Operate_Relay(RELAY3, OFF);
               }
          }
     }
     else if(gb_output[OP_3_IDX].op_dflt_state == STATE_NC)                           // Check if default state of output is NC.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_3_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_3_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_3_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_3_IDX);
                    Operate_Relay(RELAY3, OFF);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_3_IDX) == true) && (gb_op_mom_timer[OP_3_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_3_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_3_IDX);
                    Operate_Relay(RELAY3, ON);
               }
          }
     }
}

/*****************************************************************************
* Function name: void Output_4_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 16/03/2024.
*
* Description	: This function is responsible for managing output 4 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_set_reset_f, gb_op_tog_set_reset_f, gb_op_mom_pulse.
*****************************************************************************/
void Output_4_Functionality(void)
{
     if(gb_output[OP_4_IDX].op_en)                                              // Check if output is enabled.
     {
          if(gb_output[OP_4_IDX].op_dflt_state == STATE_NO)                           // Check if the default state of output is set to NO.
          {
               if(gb_output[OP_4_IDX].op_type == OP_TYPE_MOMENTARY)             // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function) || (operational_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_4_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_4_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_4_IDX);
                    }
                    else if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function) && (operational_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_4_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_4_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_4_IDX);
                    }
               }
               else if(gb_output[OP_4_IDX].op_type == OP_TYPE_TOGGLE)           // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function) || (operational_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_4_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_4_IDX);
                         Operate_Relay(RELAY4, ON);
                    }
                    else if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function) && (operational_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_4_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_4_IDX);
                         Operate_Relay(RELAY4, OFF);
                    }
               }
          }
          else if(gb_output[OP_4_IDX].op_dflt_state == STATE_NC)                      // Check if the default state of output is set to NC.
          {
               if(gb_output[OP_4_IDX].op_type == OP_TYPE_MOMENTARY)            // If output type is MOMENTARY.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags.
                    if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function) || (operational_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_4_IDX) == false))
                    {
                         SET_BIT(&gb_op_mom_pulse, OP_4_IDX);
                         SET_BIT(&gb_op_mom_set_reset_f, OP_4_IDX);
                    }
                    else if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function) && (operational_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_mom_set_reset_f, OP_4_IDX) == true))
                    {
                         //SET_BIT(&gb_op_mom_pulse, OP_4_IDX);
                         CLEAR_BIT(&gb_op_mom_set_reset_f, OP_4_IDX);
                    }
               }
               else if(gb_output[OP_4_IDX].op_type == OP_TYPE_TOGGLE)          // If output type is TOGGLE.
               {
                    // Check if the door state or operational state matches the required function and set or reset the corresponding flags and operate the relay.
                    if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function) || (operational_state[(gb_output[OP_4_IDX].door_number) - 1] == gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_4_IDX) == false))
                    {
                         SET_BIT(&gb_op_tog_set_reset_f, OP_4_IDX);
                         Operate_Relay(RELAY4, OFF);
                    }
                    else if(((door_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function) && (operational_state[(gb_output[OP_4_IDX].door_number) - 1] != gb_output[OP_4_IDX].op_function)) && (IS_BIT_SET(gb_op_tog_set_reset_f, OP_4_IDX) == true))
                    {
                         CLEAR_BIT(&gb_op_tog_set_reset_f, OP_4_IDX);
                         Operate_Relay(RELAY4, ON);
                    }
               }
          }
     }
}

/*****************************************************************************
* Function name: void operate_relay_4_mom(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for giving a momentary pulse on output 4.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_mom_timer_running, gb_op_mom_timer, gb_op_mom_pulse.
*****************************************************************************/
void operate_relay_4_mom(void)
{
     if(gb_output[OP_4_IDX].op_dflt_state == STATE_NO)                                // Check if default state of output is NO.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_4_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_4_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_4_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_4_IDX);
                    Operate_Relay(RELAY4, ON);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_4_IDX) == true) && (gb_op_mom_timer[OP_4_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_4_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_4_IDX);
                    Operate_Relay(RELAY4, OFF);
               }
          }
     }
     else if(gb_output[OP_4_IDX].op_dflt_state == STATE_NC)                           // Check if default state of output is NC.
     {
          if(IS_BIT_SET(gb_op_mom_pulse, OP_4_IDX) == true)                     // Check if the pulse bit for output is set.
          {
               if(IS_BIT_SET(gb_op_mom_timer_running, OP_4_IDX) == false)       // If the timer for output is not running.
               {
                    gb_op_mom_timer[OP_4_IDX] = 0;                              // Initialize timer, set timer running bit, and operate relay.
                    SET_BIT(&gb_op_mom_timer_running, OP_4_IDX);
                    Operate_Relay(RELAY4, OFF);
               }
               else if((IS_BIT_SET(gb_op_mom_timer_running, OP_4_IDX) == true) && (gb_op_mom_timer[OP_4_IDX] >= OP_RST_TIME))   // If timer is running and reached the threshold.
               {
                    CLEAR_BIT(&gb_op_mom_timer_running, OP_4_IDX);              // Clear timer running and pulse bits, and operate relay.
                    CLEAR_BIT(&gb_op_mom_pulse, OP_4_IDX);
                    Operate_Relay(RELAY4, ON);
               }
          }
     }
}

/*****************************************************************************
* Function name: void update_op_test_buff(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for updating the test data of door and operational state continuously.
*              :
* Notes	     : NA.
* Global Variables Affected : door_state, operational_state.
*****************************************************************************/
void update_op_test_buff(void)
{
     door_state[0] = gb_op_test_frame_buff[0];                                  // Update the door state and operational state.
     operational_state[0] = gb_op_test_frame_buff[1];
}

/*****************************************************************************
* Function name: void Set_Default_State_NO_NC (void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 19/03/2024.
*
* Description	: This function is responsible for setting the default state of output after checking the configuration data.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_op_config_cplt.
*****************************************************************************/
void Set_Default_State_NO_NC (void)
{
     if(gb_op_config_cplt)
     {

          #if DEBUG_ALL || DEBUG_OP
          Print_Message("\nSetting default state of outputs");
          #endif

          if(gb_output[OP_1_IDX].op_dflt_state == STATE_NC)                           // Set the default state of all the outputs after checking whether it is NO or NC.
          Operate_Relay(RELAY1, ON);
          else
          Operate_Relay(RELAY1, OFF);

          if(gb_output[OP_2_IDX].op_dflt_state == STATE_NC)
          Operate_Relay(RELAY2, ON);
          else
          Operate_Relay(RELAY2, OFF);

          if(gb_output[OP_3_IDX].op_dflt_state == STATE_NC)
          Operate_Relay(RELAY3, ON);
          else
          Operate_Relay(RELAY3, OFF);

          if(gb_output[OP_4_IDX].op_dflt_state == STATE_NC)
          Operate_Relay(RELAY4, ON);
          else
          Operate_Relay(RELAY4, OFF);

          gb_op_config_cplt = false;
     }
}