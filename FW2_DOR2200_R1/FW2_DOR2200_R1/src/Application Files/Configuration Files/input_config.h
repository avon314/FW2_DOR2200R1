/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: input_config.h
* Created By	: Harshit Agnihotri.
* Created Date	: 04/12/2023.
* Module
* Description	: Header file for input_config.c
				  Defines constants and macros for digital_ip_app.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 04/12/2023.
* Changes		: NA.
*****************************************************************************/

/***** User Includes *****/

#ifndef INPUT_CONFIG_H_
#define INPUT_CONFIG_H_

/***** Definitions / Macros *****/
#define IP_CONFIG_BYTES			(36)
#define MAX_INPUTS				(6)
#define MAX_IP_LEN				(6)
#define IP_NOT_FOR_INTERLOCK	(0x00)
#define IP_FOR_INTERLOCK		(0x01)
#define HIGH_TO_LOW				(0x00)
#define LOW_TO_HIGH				(0x01)
#define MOMENTARY				(0x00)
#define TOGGLE					(0x01)
#define ACCESS_DENIED			(0x01)
#define RELEASE_DOOR			(0x02)
#define RELEASE_DOOR_DRT		(0x03)
#define TEST_IP_CONFIG			(1)

/***** Structure variable that represents the write configuration of an input, which can be used to store information about the behavior and characteristics of an input *****/
typedef struct
{
     U8 ip_en;                         // Whether the input is enabled or not.
     U8 door_grp_number;               // The number of the door or group associated with this input.
     U8 ip_for_interlock;              // Whether the input should be considered for interlock or not.
     U8 ip_activate_state;             // Whether the input functionality should be activated when input goes from low to high or high to low.
     U8 ip_type;                       // The type of the input, which can be Momentary or Toggle.
     U8 ip_function;                   // The function of the input, which can represent various actions or states.
}input_configuration;
// Declare the structure variable of input_configuration to store the input configuration information.
extern input_configuration gb_input[MAX_INPUTS];

/***** Function Declarations / Prototypes *****/
void write_ip_config(U8*);
void read_ip_config(U8*);
void test_ip_config(void);

#endif /* INPUT_CONFIG_H_ */