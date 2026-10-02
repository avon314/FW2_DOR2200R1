/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_ip_app.c
* Created By	: Harshit Agnihotri.
* Created Date	: 11/10/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 11/10/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "digital_ip_app.h"
#include "input_config.h"
#include "user_uart.h"
#include "definitions.h"
#include "digital_input.h"
#include "user_timer.h"
#include "group_config.h"
#include "interlock.h"


volatile bool ip_fire_detect_f;              // Flag for Fire.
volatile bool control_emg_pin_detect = false; // Flag for Emergency.
volatile bool emg_ip_timer_start_f = false;         // Flag for Emergency-Timer status.
volatile bool gb_ip_emg_detected_f = 0;
volatile bool gb_ip_emg_reset_f = 0;

// Variables to store function of different inputs.
volatile U8 gb_ip1_func = 0;
volatile U8 gb_ip2_func = 0;
volatile U8 gb_ip3_func = 0;
volatile U8 gb_ip4_func = 0;
volatile U8 gb_ip5_func = 0;
volatile U8 gb_ip6_func = 0;

// Variable to store interlocking groups of different inputs.
volatile U8 gb_ip1_ilock_grps = 0;
volatile U8 gb_ip2_ilock_grps = 0;
volatile U8 gb_ip3_ilock_grps = 0;
volatile U8 gb_ip4_ilock_grps = 0;
volatile U8 gb_ip5_ilock_grps = 0;
volatile U8 gb_ip6_ilock_grps = 0;

// Toggle and Momentary state control flags.
static U8 gb_tog_ctrl_f = false;
static U8 gb_mom_ctrl_f = false;
static U8 gb_mom_set_f = false;

U8 grp_fire_state = 0x00;                    // Status of fire for each group bitwise.
U8 grp_emergency_state = 0x00;               // Status of emergency for each group bitwise.
volatile U16 emg_ip_time_count = 0;                // Timer for Emergency.

/*****************************************************************************
* Function name: void Check_Fire_Input_detection(void).
* Returns		: nothing.
* Arguments    : DIG_INPUT* digInput.
* Created by	: Harshit Agnihotri.
* Date created	: 18/10/2023.
*
* Description	: This function is used to handle the detection of a fire condition based on digital input.
*              :
* Notes	     : This function should be called again and again continuously in a loop in main logic to check for fire input continuously and set or reset the ip_fire_detect_f.
* Global Variables Affected : ip_fire_detect_f.
*****************************************************************************/
void Check_Fire_Input_detection(void)
{
     if(digInput.ip1_htl_f == true)           // Check if the Input 1 HTL flag is true.
     {
          ip_fire_detect_f = true;             // Set the "fire input detected" flag to true.
     }
     else if(digInput.ip1_lth_f == true)      // If the Input 1 LTH flag is true.
     {
          ip_fire_detect_f = false;            // Reset the "fire input detected" flag to false.
     }
}

/*****************************************************************************
* Function name: void test_ip_fire_detect().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 18/10/2023.
*
* Description	: This function tests the Check_Fire_Input_detection function.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_ip_fire_detect(void)
{
     #if (TEST_IP_FIRE_DETECT == uDISABLE)
     Check_Fire_Input_detection();               // Test Check_Fire_Input_detection.
     #endif
}

/*****************************************************************************
* Function name: void Is_Fire_Input_Enabled_InGroup().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 07/12/2023.
*
* Description	: Function to detect fire in groups and set the corresponding bit in grp_fire_state.
*              :
* Notes	     :
* Global Variables Affected : grp_fire_state.
*****************************************************************************/
void Is_Fire_Input_Enabled_InGroup(void)
{
	grp_fire_state = 0x00;

	for (U8 group_index = 0; group_index < NUM_OF_GROUPS; group_index++)
	{
		if(IS_BIT_SET(Group[group_index].ips_in_grp, FIRE_IP))           // Check if fire input is enabled in specific group.
		SET_BIT(&grp_fire_state, group_index);                           // If enabled set the equivalent bit of group fire state.
	}
}

/*****************************************************************************
* Function name: void test_grp_fire_detect().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 07/12/2023.
*
* Description	: This function tests the Is_Fire_Input_Enabled_InGroup function.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_grp_fire_detect(void)
{
     #if (TEST_GRP_FIRE_DETECT == uDISABLE)
     Is_Fire_Input_Enabled_InGroup();                      // Test Is_Fire_Input_Enabled_InGroup.
     #endif
}

/*****************************************************************************
* Function name: void Check_EMG_Input_Detection(void).
* Returns		: nothing.
* Arguments    : DIG_INPUT* digInput.
* Created by	: Harshit Agnihotri.
* Date created	: 19/10/2023.
*
* Description	: This function is used to handle the detection of an emergency condition based on digital input.
*              :
* Notes	     : This function should be called again and again continuously in a loop in main logic to check for emergency input continuously and set or reset the control_emg_pin_detect.
* Global Variables Affected : control_emg_pin_detect, emg_ip_time_count, emg_ip_timer_start_f, ip2_htl_f, ip2_lth_f.
*****************************************************************************/
void Check_EMG_Input_Detection(void)
{
     /*	The emergency stays active for as long as the input is in its active
		state. It is reset only when the input returns to its normal state.
		(Previously a 5 s timer in the timer ISR raised the reset by itself, so
		the system left emergency while the field switch was still operated, and
		the real return of the input was ignored.) */
     if((digInput.ip2_htl_f == FLAG_SET) && (control_emg_pin_detect == FLAG_RST))    // Check if input 2 high-to-low flag is set and emergency flag is not set.
     {
          control_emg_pin_detect = FLAG_SET;                                 // Set emergency flag.
		  gb_ip_emg_detected_f = FLAG_SET;
		  gb_ip_emg_reset_f = FLAG_RST;
     }
	 else if ((digInput.ip2_htl_f == FLAG_RST) && (control_emg_pin_detect == FLAG_SET))
	 {
		 control_emg_pin_detect = FLAG_RST;
		 
		 if (gb_ip_emg_detected_f == FLAG_SET)
		 {
			 /* Emergency was not started yet (bus was busy): cancel it. */
			 gb_ip_emg_detected_f = FLAG_RST;
		 }
		 else
		 {
			 gb_ip_emg_reset_f = FLAG_SET;	/* Input back to normal: reset the emergency. */
		 }
	 }

//      else if((digInput.ip2_lth_f) && (emg_ip_time_count<IP_EMG_RST_TIME))      // Check if input 2 low-to-high flag is set and timer count is less than Emergency Reset Time.
//      {
//           emg_ip_time_count = 0;                                              // Reset the timer count.
//      }
//      else if((emg_ip_time_count >= IP_EMG_RST_TIME) && (digInput.ip2_htl_f))     // Check if timer count has reached 5Emergency Reset Time and input 2 high-to-low flag is set.
//      {
//           control_emg_pin_detect=false;                                // Reset emergency flag, stop the timer, and update input flags.
//           emg_ip_timer_start_f=false;
//           digInput.ip2_htl_f=false;
//           digInput.ip2_lth_f=true;
//      }
}

/*****************************************************************************
* Function name: void test_ip_emergency_detect().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 19/10/2023.
*
* Description	: This function tests the Check_EMG_Input_Detection function.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_ip_emergency_detect(void)
{
     #if (TEST_IP_EMERGENCY_DETECT == uDISABLE)
     Check_EMG_Input_Detection();               // Test Check_EMG_Input_Detection.
     #endif
}

/*****************************************************************************
* Function name: void Is_EMG_Input_Enabled_InGroup().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 08/12/2023.
*
* Description	: Function to detect emergency in groups and set the corresponding bit in grp_emergency_state.
*              :
* Notes	     :
* Global Variables Affected : grp_emergency_state.
*****************************************************************************/
void Is_EMG_Input_Enabled_InGroup(void)
{
	grp_emergency_state = 0x00;

	for(U8 group_index = 0; group_index < NUM_OF_GROUPS; group_index++)
    {
        if (IS_BIT_SET(Group[group_index].ips_in_grp, EMERGENCY_IP))           // Check if emergency input is enabled in specific group.
        SET_BIT(&grp_emergency_state, group_index);                           // If enabled set the equivalent bit of group emergency state.
    }

}

/*****************************************************************************
* Function name: void test_grp_emergency_detect().
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 08/12/2023.
*
* Description	: This function tests the Is_EMG_Input_Enabled_InGroup function.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_grp_emergency_detect(void)
{
     #if (TEST_GRP_EMERGENCY_DETECT == uDISABLE)
     Is_EMG_Input_Enabled_InGroup();                      // Test Is_EMG_Input_Enabled_InGroup.
     #endif
}

/*****************************************************************************
* Function name: void Input_1_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 1 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip1_func, gb_ip1_ilock_grps.
*****************************************************************************/
void Input_1_Functionality(void)
{
	if(gb_input[IP_1_IDX].ip_en)                                               // Check if input is enabled.
	{
		if(gb_input[IP_1_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
		{
			for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
			{
				if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_1_IDX))
				{
					SET_BIT(&gb_ip1_ilock_grps, grp_idx);
				}
			}
			if(gb_ip1_ilock_grps != 0)                                       // If input is in any interlock group.
			{
				if(gb_input[IP_1_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
				{
					if(((digInput.ip3_htl_f == true) && (digInput.ip3_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
					{
						gb_ip1_func = IP_ILOCK;                           // Set input function to interlock.

						SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
						Print_Message("\n\nInput Low to High Interlock");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(((digInput.ip3_htl_f == false) && (digInput.ip3_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
					{
						gb_ip1_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

						CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
						Print_Message("\n\nInput Low to High Not Interlock");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
				/*********************************************************************************************************************/
				else if(gb_input[IP_1_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
				{
					if(((digInput.ip3_lth_f == true) && (digInput.ip3_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
					{
						gb_ip1_func = IP_ILOCK;                           // Set input function to interlock.

						SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
						Print_Message("\n\nInput High to Low Interlock");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(((digInput.ip3_lth_f == false) && (digInput.ip3_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
					{
						gb_ip1_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

						CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
						Print_Message("\n\nInput High to Low Not Interlock");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
			}
		}
		/*********************************************************************************************************************/
		/*********************************************************************************************************************/
		else if(gb_input[IP_1_IDX].ip_type == TOGGLE)                         // If input type is toggle.
		{
			if(gb_input[IP_1_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
			{
				if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
				{
					if(((digInput.ip3_htl_f == true) && (digInput.ip3_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
						gb_ip1_func = TOG_DOR_AD;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = TOG_GRP_AD;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = TOG_ALLDOR_AD;

						SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
						Print_Message("\n\nToggle Low to High Access Denied");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(((digInput.ip3_htl_f == false) && (digInput.ip3_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
						gb_ip1_func = TOG_DOR_AD_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = TOG_GRP_AD_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = TOG_ALLDOR_AD_N;

						CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

						#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
                    else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
                    {
                         if(((digInput.ip3_htl_f == true) && (digInput.ip3_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                         {
                              if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                              gb_ip1_func = TOG_DOR_DR;
                              else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
                              gb_ip1_func = TOG_GRP_DR;
                              else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
                              gb_ip1_func = TOG_ALLDOR_DR;

                              SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door");
                              Print_Message("\nInput 1 function value = ");
                              Print_Number(gb_ip1_func);
                              #endif

                         }
                         else if(((digInput.ip3_htl_f == false) && (digInput.ip3_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                         {
                              if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                              gb_ip1_func = TOG_DOR_DR_N;
                              else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
                              gb_ip1_func = TOG_GRP_DR_N;
                              else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
                              gb_ip1_func = TOG_ALLDOR_DR_N;

                              CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door to Normal");
                              Print_Message("\nInput 1 function value = ");
                              Print_Number(gb_ip1_func);
                              #endif

                         }
                    }
                    else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
                    {
                         if(((digInput.ip3_htl_f == true) && (digInput.ip3_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                         {
                              if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                              gb_ip1_func = TOG_DOR_DRT;
                              else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
                              gb_ip1_func = TOG_GRP_DRT;
                              else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
                              gb_ip1_func = TOG_ALLDOR_DRT;

                              SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT");
                              Print_Message("\nInput 1 function value = ");
                              Print_Number(gb_ip1_func);
                              #endif

                         }
                         else if(((digInput.ip3_htl_f == false) && (digInput.ip3_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                         {
                              if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                              gb_ip1_func = TOG_DOR_DRT_N;
                              else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
                              gb_ip1_func = TOG_GRP_DRT_N;
                              else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
                              gb_ip1_func = TOG_ALLDOR_DRT_N;

                              CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                              Print_Message("\nInput 1 function value = ");
                              Print_Number(gb_ip1_func);
                              #endif

                         }
                    }
			}
			/*********************************************************************************************************************/
			else if(gb_input[IP_1_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
			{
				if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
				{
     				if(((digInput.ip3_lth_f == true) && (digInput.ip3_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_AD;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_AD;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_AD;

          				SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
     				else if(((digInput.ip3_lth_f == false) && (digInput.ip3_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_AD_N;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_AD_N;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_AD_N;

          				CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied to Normal");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
				}
				else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
				{
     				if(((digInput.ip3_lth_f == true) && (digInput.ip3_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for release door activation (Input detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_DR;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_DR;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_DR;

          				SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
     				else if(((digInput.ip3_lth_f == false) && (digInput.ip3_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_DR_N;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_DR_N;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_DR_N;

          				CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door to Normal");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
				}
				else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
				{
     				if(((digInput.ip3_lth_f == true) && (digInput.ip3_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_DRT;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_DRT;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_DRT;

          				SET_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
     				else if(((digInput.ip3_lth_f == false) && (digInput.ip3_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_1_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
     				{
          				if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
          				gb_ip1_func = TOG_DOR_DRT_N;
          				else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
          				gb_ip1_func = TOG_GRP_DRT_N;
          				else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
          				gb_ip1_func = TOG_ALLDOR_DRT_N;

          				CLEAR_BIT(&gb_tog_ctrl_f, IP_1_IDX);

          				#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
          				Print_Message("\nInput 1 function value = ");
          				Print_Number(gb_ip1_func);
                              #endif

     				}
				}
			}
		}
		/*********************************************************************************************************************/
		/*********************************************************************************************************************/
		else if(gb_input[IP_1_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
		{
			if(gb_input[IP_1_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
			{
				if((digInput.ip3_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
				{
					SET_BIT(&gb_mom_ctrl_f, IP_1_IDX);
				}
				if((digInput.ip3_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_1_IDX) == false))
				{

					SET_BIT(&gb_mom_set_f, IP_1_IDX);
					CLEAR_BIT(&gb_mom_ctrl_f, IP_1_IDX);

					if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_AD;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_AD;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DR;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DR;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DRT;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DRT;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
				else if((digInput.ip3_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_1_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
				{

					CLEAR_BIT(&gb_mom_set_f, IP_1_IDX);
					CLEAR_BIT(&gb_mom_ctrl_f, IP_1_IDX);

					if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_AD_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_AD_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DR_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DR_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DRT_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DRT_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
			}
			/*********************************************************************************************************************/
			else if(gb_input[IP_1_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
			{
				if((digInput.ip3_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
				{
					SET_BIT(&gb_mom_ctrl_f, IP_1_IDX);
				}
				if((digInput.ip3_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_1_IDX) == false))
				{

					SET_BIT(&gb_mom_set_f, IP_1_IDX);
					CLEAR_BIT(&gb_mom_ctrl_f, IP_1_IDX);

					if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_AD;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_AD;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DR;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DR;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DRT;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DRT;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
				else if((digInput.ip3_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_1_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_1_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
				{

					CLEAR_BIT(&gb_mom_set_f, IP_1_IDX);
					CLEAR_BIT(&gb_mom_ctrl_f, IP_1_IDX);

					if(gb_input[IP_1_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_AD_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_AD_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DR_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DR_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
					else if(gb_input[IP_1_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
					{
						if((gb_input[IP_1_IDX].door_grp_number >= 0x01) && (gb_input[IP_1_IDX].door_grp_number <= 0x10))
						gb_ip1_func = MOM_DOR_DRT_N;
						else if((gb_input[IP_1_IDX].door_grp_number >= 0x11) && (gb_input[IP_1_IDX].door_grp_number <= 0x18))
						gb_ip1_func = MOM_GRP_DRT_N;
						else if(gb_input[IP_1_IDX].door_grp_number == 0x19)
						gb_ip1_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
						Print_Message("\nInput 1 function value = ");
						Print_Number(gb_ip1_func);
                              #endif

					}
				}
			}
		}
	}
}





/*****************************************************************************
* Function name: void Input_2_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 2 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip2_func, gb_ip2_ilock_grps.
*****************************************************************************/
void Input_2_Functionality(void)
{
	if(gb_input[IP_2_IDX].ip_en)                                               // Check if input is enabled.
	{
     	if(gb_input[IP_2_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
     	{
          	for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
          	{
               	if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_2_IDX))
               	{
                    SET_BIT(&gb_ip2_ilock_grps, grp_idx);
               	}
          	}
          	if(gb_ip2_ilock_grps != 0)                                       // If input is in any interlock group.
          	{
               	if(gb_input[IP_2_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
               	{
                    	if(((digInput.ip4_htl_f == true) && (digInput.ip4_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip2_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nInput Low to High Interlock");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_htl_f == false) && (digInput.ip4_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip2_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nInput Low to High Not Interlock");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	/*********************************************************************************************************************/
               	else if(gb_input[IP_2_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
               	{
                    	if(((digInput.ip4_lth_f == true) && (digInput.ip4_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip2_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nInput High to Low Interlock");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_lth_f == false) && (digInput.ip4_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip2_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nInput High to Low Not Interlock");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_2_IDX].ip_type == TOGGLE)                         // If input type is toggle.
     	{
          	if(gb_input[IP_2_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
          	{
               	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip4_htl_f == true) && (digInput.ip4_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_AD;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_AD;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Access Denied");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_htl_f == false) && (digInput.ip4_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Access Denied to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip4_htl_f == true) && (digInput.ip4_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DR;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DR;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Release Door");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif
                    	}
                    	else if(((digInput.ip4_htl_f == false) && (digInput.ip4_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Release Door to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip4_htl_f == true) && (digInput.ip4_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Release Door DRT");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_htl_f == false) && (digInput.ip4_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_2_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip4_lth_f == true) && (digInput.ip4_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_AD;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_AD;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Access Denied");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_lth_f == false) && (digInput.ip4_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Access Denied to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip4_lth_f == true) && (digInput.ip4_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DR;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DR;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Release Door");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_lth_f == false) && (digInput.ip4_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Release Door to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip4_lth_f == true) && (digInput.ip4_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Release Door DRT");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(((digInput.ip4_lth_f == false) && (digInput.ip4_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_2_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip2_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_2_IDX);

                              #if DEBUG_ALL || DEBUG_DIG_IP
                         	Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_2_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
     	{
          	if(gb_input[IP_2_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
          	{
               	if((digInput.ip4_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_2_IDX);
               	}
               	if((digInput.ip4_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_2_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_2_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_2_IDX);

                    	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_AD;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_AD;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DR;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DR;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip4_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_2_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_2_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_2_IDX);

                    	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_2_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if((digInput.ip4_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_2_IDX);
               	}
               	if((digInput.ip4_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_2_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_2_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_2_IDX);

                    	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_AD;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_AD;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DR;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DR;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip4_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_2_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_2_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_2_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_2_IDX);

                    	if(gb_input[IP_2_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
                    	else if(gb_input[IP_2_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_2_IDX].door_grp_number >= 0x01) && (gb_input[IP_2_IDX].door_grp_number <= 0x10))
                         	gb_ip2_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_2_IDX].door_grp_number >= 0x11) && (gb_input[IP_2_IDX].door_grp_number <= 0x18))
                         	gb_ip2_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_2_IDX].door_grp_number == 0x19)
                         	gb_ip2_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 2 function value = ");
                         	Print_Number(gb_ip2_func);
                              #endif

                    	}
               	}
          	}
     	}
	}
}





/*****************************************************************************
* Function name: void Input_3_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 3 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip3_func, gb_ip3_ilock_grps.
*****************************************************************************/
void Input_3_Functionality(void)
{
	if(gb_input[IP_3_IDX].ip_en)                                               // Check if input is enabled.
	{
     	if(gb_input[IP_3_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
     	{
          	for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
          	{
               	if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_3_IDX))
               	{
                    	SET_BIT(&gb_ip3_ilock_grps, grp_idx);
               	}
          	}
          	if(gb_ip3_ilock_grps != 0)                                       // If input is in any interlock group.
          	{
               	if(gb_input[IP_3_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
               	{
                    	if(((digInput.ip5_htl_f == true) && (digInput.ip5_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip3_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Interlock");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_htl_f == false) && (digInput.ip5_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip3_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Not Interlock");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	/*********************************************************************************************************************/
               	else if(gb_input[IP_3_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
               	{
                    	if(((digInput.ip5_lth_f == true) && (digInput.ip5_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip3_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Interlock");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_lth_f == false) && (digInput.ip5_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip3_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Not Interlock");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_3_IDX].ip_type == TOGGLE)                         // If input type is toggle.
     	{
          	if(gb_input[IP_3_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
          	{
               	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip5_htl_f == true) && (digInput.ip5_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_AD;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_AD;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_htl_f == false) && (digInput.ip5_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip5_htl_f == true) && (digInput.ip5_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DR;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DR;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_htl_f == false) && (digInput.ip5_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip5_htl_f == true) && (digInput.ip5_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_htl_f == false) && (digInput.ip5_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_3_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip5_lth_f == true) && (digInput.ip5_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_AD;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_AD;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_lth_f == false) && (digInput.ip5_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip5_lth_f == true) && (digInput.ip5_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DR;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DR;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_lth_f == false) && (digInput.ip5_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip5_lth_f == true) && (digInput.ip5_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(((digInput.ip5_lth_f == false) && (digInput.ip5_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_3_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip3_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_3_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_3_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
     	{
          	if(gb_input[IP_3_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
          	{
               	if((digInput.ip5_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_3_IDX);
               	}
               	if((digInput.ip5_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_3_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_3_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_3_IDX);

                    	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_AD;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_AD;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DR;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DR;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip5_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_3_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_3_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_3_IDX);

                    	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_3_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if((digInput.ip5_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_3_IDX);
               	}
               	if((digInput.ip5_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_3_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_3_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_3_IDX);

                    	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_AD;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_AD;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DR;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DR;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip5_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_3_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_3_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_3_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_3_IDX);

                    	if(gb_input[IP_3_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
                    	else if(gb_input[IP_3_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_3_IDX].door_grp_number >= 0x01) && (gb_input[IP_3_IDX].door_grp_number <= 0x10))
                         	gb_ip3_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_3_IDX].door_grp_number >= 0x11) && (gb_input[IP_3_IDX].door_grp_number <= 0x18))
                         	gb_ip3_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_3_IDX].door_grp_number == 0x19)
                         	gb_ip3_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 3 function value = ");
                         	Print_Number(gb_ip3_func);
                              #endif

                    	}
               	}
          	}
     	}
	}
}

/*****************************************************************************
* Function name: void Input_4_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 4 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip4_func, gb_ip4_ilock_grps.
*****************************************************************************/
void Input_4_Functionality(void)
{
	if(gb_input[IP_4_IDX].ip_en)                                               // Check if input is enabled.
	{
     	if(gb_input[IP_4_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
     	{
          	for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
          	{
               	if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_4_IDX))
               	{
                    	SET_BIT(&gb_ip4_ilock_grps, grp_idx);
               	}
          	}
          	if(gb_ip4_ilock_grps != 0)                                       // If input is in any interlock group.
          	{
               	if(gb_input[IP_4_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
               	{
                    	if(((digInput.ip6_htl_f == true) && (digInput.ip6_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip4_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Interlock");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_htl_f == false) && (digInput.ip6_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip4_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Not Interlock");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	/*********************************************************************************************************************/
               	else if(gb_input[IP_4_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
               	{
                    	if(((digInput.ip6_lth_f == true) && (digInput.ip6_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip4_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Interlock");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_lth_f == false) && (digInput.ip6_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip4_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Not Interlock");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_4_IDX].ip_type == TOGGLE)                         // If input type is toggle.
     	{
          	if(gb_input[IP_4_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
          	{
               	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip6_htl_f == true) && (digInput.ip6_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_AD;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_AD;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_htl_f == false) && (digInput.ip6_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip6_htl_f == true) && (digInput.ip6_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DR;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DR;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_htl_f == false) && (digInput.ip6_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip6_htl_f == true) && (digInput.ip6_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_htl_f == false) && (digInput.ip6_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_4_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip6_lth_f == true) && (digInput.ip6_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_AD;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_AD;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_lth_f == false) && (digInput.ip6_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip6_lth_f == true) && (digInput.ip6_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DR;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DR;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_lth_f == false) && (digInput.ip6_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip6_lth_f == true) && (digInput.ip6_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(((digInput.ip6_lth_f == false) && (digInput.ip6_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_4_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip4_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_4_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_4_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
     	{
          	if(gb_input[IP_4_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
          	{
               	if((digInput.ip6_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_4_IDX);
               	}
               	if((digInput.ip6_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_4_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_4_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_4_IDX);

                    	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_AD;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_AD;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DR;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DR;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip6_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_4_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_4_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_4_IDX);

                    	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_4_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if((digInput.ip6_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_4_IDX);
               	}
               	if((digInput.ip6_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_4_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_4_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_4_IDX);

                    	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_AD;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_AD;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DR;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DR;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip6_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_4_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_4_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_4_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_4_IDX);

                    	if(gb_input[IP_4_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
                    	else if(gb_input[IP_4_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_4_IDX].door_grp_number >= 0x01) && (gb_input[IP_4_IDX].door_grp_number <= 0x10))
                         	gb_ip4_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_4_IDX].door_grp_number >= 0x11) && (gb_input[IP_4_IDX].door_grp_number <= 0x18))
                         	gb_ip4_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_4_IDX].door_grp_number == 0x19)
                         	gb_ip4_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 4 function value = ");
                         	Print_Number(gb_ip4_func);
                              #endif

                    	}
               	}
          	}
     	}
	}
}

/*****************************************************************************
* Function name: void Input_5_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 5 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip5_func, gb_ip5_ilock_grps.
*****************************************************************************/
void Input_5_Functionality(void)
{
	if(gb_input[IP_5_IDX].ip_en)                                               // Check if input is enabled.
	{
     	if(gb_input[IP_5_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
     	{
          	for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
          	{
               	if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_5_IDX))
               	{
                    	SET_BIT(&gb_ip5_ilock_grps, grp_idx);
               	}
          	}
          	if(gb_ip5_ilock_grps != 0)                                       // If input is in any interlock group.
          	{
               	if(gb_input[IP_5_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
               	{
                    	if(((digInput.ip7_htl_f == true) && (digInput.ip7_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip5_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Interlock");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_htl_f == false) && (digInput.ip7_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip5_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Not Interlock");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	/*********************************************************************************************************************/
               	else if(gb_input[IP_5_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
               	{
                    	if(((digInput.ip7_lth_f == true) && (digInput.ip7_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip5_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Interlock");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_lth_f == false) && (digInput.ip7_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip5_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Not Interlock");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_5_IDX].ip_type == TOGGLE)                         // If input type is toggle.
     	{
          	if(gb_input[IP_5_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
          	{
               	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip7_htl_f == true) && (digInput.ip7_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_AD;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_AD;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_htl_f == false) && (digInput.ip7_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip7_htl_f == true) && (digInput.ip7_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DR;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DR;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_htl_f == false) && (digInput.ip7_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip7_htl_f == true) && (digInput.ip7_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_htl_f == false) && (digInput.ip7_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_5_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip7_lth_f == true) && (digInput.ip7_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_AD;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_AD;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_lth_f == false) && (digInput.ip7_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip7_lth_f == true) && (digInput.ip7_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DR;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DR;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(((digInput.ip7_lth_f == false) && (digInput.ip7_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip7_lth_f == true) && (digInput.ip7_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                         	#endif

                    	}
                    	else if(((digInput.ip7_lth_f == false) && (digInput.ip7_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_5_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip5_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_5_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                         	#endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_5_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
     	{
          	if(gb_input[IP_5_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
          	{
               	if((digInput.ip7_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_5_IDX);
               	}
               	if((digInput.ip7_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_5_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_5_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_5_IDX);

                    	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_AD;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_AD;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                         	#endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DR;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DR;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                         	#endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip7_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_5_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_5_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_5_IDX);

                    	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_5_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if((digInput.ip7_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_5_IDX);
               	}
               	if((digInput.ip7_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_5_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_5_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_5_IDX);

                    	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_AD;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_AD;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DR;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DR;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip7_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_5_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_5_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_5_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_5_IDX);

                    	if(gb_input[IP_5_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
                    	else if(gb_input[IP_5_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_5_IDX].door_grp_number >= 0x01) && (gb_input[IP_5_IDX].door_grp_number <= 0x10))
                         	gb_ip5_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_5_IDX].door_grp_number >= 0x11) && (gb_input[IP_5_IDX].door_grp_number <= 0x18))
                         	gb_ip5_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_5_IDX].door_grp_number == 0x19)
                         	gb_ip5_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 5 function value = ");
                         	Print_Number(gb_ip5_func);
                              #endif

                    	}
               	}
          	}
     	}
	}
}

/*****************************************************************************
* Function name: void Input_6_Functionality(void).
* Returns		: nothing.
* Arguments    : nothing.
* Created by	: Harshit Agnihotri.
* Date created	: 13/03/2024.
*
* Description	: This function is responsible for managing input 6 based on its configuration and current state.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_ip6_func, gb_ip6_ilock_grps.
*****************************************************************************/
void Input_6_Functionality(void)
{
	if(gb_input[IP_6_IDX].ip_en)                                               // Check if input is enabled.
	{
     	if(gb_input[IP_6_IDX].ip_for_interlock == IP_FOR_INTERLOCK)           // Check if input is for interlock.
     	{
          	for(U8 grp_idx = 0; grp_idx < 8; grp_idx++)                      // Loop through groups to check if input is part of a group.
          	{
               	if(IS_BIT_SET(Group[grp_idx].ips_in_grp, IP_6_IDX))
               	{
                    	SET_BIT(&gb_ip6_ilock_grps, grp_idx);
               	}
          	}
          	if(gb_ip6_ilock_grps != 0)                                       // If input is in any interlock group.
          	{
               	if(gb_input[IP_6_IDX].ip_activate_state == LOW_TO_HIGH)    // If input activate state is low to high.
               	{
                    	if(((digInput.ip8_htl_f == true) && (digInput.ip8_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip6_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Interlock");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_htl_f == false) && (digInput.ip8_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip6_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput Low to High Not Interlock");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	/*********************************************************************************************************************/
               	else if(gb_input[IP_6_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
               	{
                    	if(((digInput.ip8_lth_f == true) && (digInput.ip8_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for interlock activation (Input detected).
                    	{
                         	gb_ip6_func = IP_ILOCK;                           // Set input function to interlock.

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Interlock");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_lth_f == false) && (digInput.ip8_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating interlock (Input not detected).
                    	{
                         	gb_ip6_func = IP_ILOCK_N;                         // Set input function from interlock to normal.

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nInput High to Low Not Interlock");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_6_IDX].ip_type == TOGGLE)                         // If input type is toggle.
     	{
          	if(gb_input[IP_6_IDX].ip_activate_state == LOW_TO_HIGH)          // If input activate state is low to high.
          	{
               	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip8_htl_f == true) && (digInput.ip8_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_AD;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_AD;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_htl_f == false) && (digInput.ip8_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Access Denied to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip8_htl_f == true) && (digInput.ip8_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DR;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DR;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_htl_f == false) && (digInput.ip8_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip8_htl_f == true) && (digInput.ip8_lth_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_htl_f == false) && (digInput.ip8_lth_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_6_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)         // If input function is access denied.
               	{
                    	if(((digInput.ip8_lth_f == true) && (digInput.ip8_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for access denied activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_AD;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_AD;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_AD;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_lth_f == false) && (digInput.ip8_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating access denied (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_AD_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_AD_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_AD_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Access Denied to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)           // If input function is release door.
               	{
                    	if(((digInput.ip8_lth_f == true) && (digInput.ip8_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for release door activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DR;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DR;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DR;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_lth_f == false) && (digInput.ip8_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating release door (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DR_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DR_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DR_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT)           // If input function is release door drt.
               	{
                    	if(((digInput.ip8_lth_f == true) && (digInput.ip8_htl_f == false)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == false))   // If input conditions are met for release door drt activation (Input detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DRT;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DRT;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DRT;

                         	SET_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(((digInput.ip8_lth_f == false) && (digInput.ip8_htl_f == true)) && (IS_BIT_SET(gb_tog_ctrl_f, IP_6_IDX) == true))    // If input conditions are met for deactivating release door drt (Input not detected).
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))    // Set input function to corresponding toggle type.
                         	gb_ip6_func = TOG_DOR_DRT_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = TOG_GRP_DRT_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = TOG_ALLDOR_DRT_N;

                         	CLEAR_BIT(&gb_tog_ctrl_f, IP_6_IDX);

                         	#if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nToggle High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
          	}
     	}
     	/*********************************************************************************************************************/
     	/*********************************************************************************************************************/
     	else if(gb_input[IP_6_IDX].ip_type == MOMENTARY)                      // If input type is momentary.
     	{
          	if(gb_input[IP_6_IDX].ip_activate_state == LOW_TO_HIGH)         // If input activate state is low to high.
          	{
               	if((digInput.ip8_htl_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_6_IDX);
               	}
               	if((digInput.ip8_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_6_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_6_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_6_IDX);

                    	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_AD;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_AD;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DR;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DR;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip8_htl_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_6_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_6_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_6_IDX);

                    	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Access Denied to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary Low to High Release Door DRT to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
          	}
          	/*********************************************************************************************************************/
          	else if(gb_input[IP_6_IDX].ip_activate_state == HIGH_TO_LOW)     // If input activate state is high to low.
          	{
               	if((digInput.ip8_lth_f == true) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == false))   // If input conditions are met set input function to corresponding momentary type.
               	{
                    	SET_BIT(&gb_mom_ctrl_f, IP_6_IDX);
               	}
               	if((digInput.ip8_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_6_IDX) == false))
               	{

                    	SET_BIT(&gb_mom_set_f, IP_6_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_6_IDX);

                    	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)    // If input function is access denied.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_AD;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_AD;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_AD;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)     // If input function is release door.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DR;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DR;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DR;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT) // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DRT;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DRT;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DRT;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
               	else if((digInput.ip8_lth_f == false) && (IS_BIT_SET(gb_mom_ctrl_f, IP_6_IDX) == true) && (IS_BIT_SET(gb_mom_set_f, IP_6_IDX) == true))    // If input conditions are met set input function to corresponding momentary type.
               	{

                    	CLEAR_BIT(&gb_mom_set_f, IP_6_IDX);
                    	CLEAR_BIT(&gb_mom_ctrl_f, IP_6_IDX);

                    	if(gb_input[IP_6_IDX].ip_function == ACCESS_DENIED)     // If input function is acces denied.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_AD_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_AD_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_AD_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Access Denied to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR)      // If input function is release door.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DR_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DR_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DR_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
                    	else if(gb_input[IP_6_IDX].ip_function == RELEASE_DOOR_DRT)  // If input function is release door for drt.
                    	{
                         	if((gb_input[IP_6_IDX].door_grp_number >= 0x01) && (gb_input[IP_6_IDX].door_grp_number <= 0x10))
                         	gb_ip6_func = MOM_DOR_DRT_N;
                         	else if((gb_input[IP_6_IDX].door_grp_number >= 0x11) && (gb_input[IP_6_IDX].door_grp_number <= 0x18))
                         	gb_ip6_func = MOM_GRP_DRT_N;
                         	else if(gb_input[IP_6_IDX].door_grp_number == 0x19)
                         	gb_ip6_func = MOM_ALLDOR_DRT_N;

                              #if DEBUG_ALL || DEBUG_DIG_IP
                              Print_Message("\n\nMomentary High to Low Release Door DRT to Normal");
                         	Print_Message("\nInput 6 function value = ");
                         	Print_Number(gb_ip6_func);
                              #endif

                    	}
               	}
          	}
     	}
	}
}
