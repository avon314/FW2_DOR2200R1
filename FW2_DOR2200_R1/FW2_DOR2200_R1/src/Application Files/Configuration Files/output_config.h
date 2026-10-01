/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: output_config.h
* Created By	: Harshit Agnihotri.
* Created Date	: 04/12/2023.
* Module
* Description	: Header file for output_config.c
		       Defines constants and macros for output_config.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 04/12/2023.
* Changes		: NA.
*****************************************************************************/

#ifndef OUTPUT_CONFIG_H_
#define OUTPUT_CONFIG_H_

/***** Definitions / Macros *****/
#define OP_CONFIG_BYTES (20)
#define MAX_OUTPUTS (4)
#define MAX_OP_LEN (5)
#define DOOR_OPEN (0x01)
#define DOOR_CLOSED (0x02)
#define DOOR_LOCKED (0x03)
#define ACCESS_DENIED_MODE (0x04)
#define EMERGENCY_MODE (0x05)
#define DOOR_INTERLOCKED (0x06)
#define PRIVACY (0x07)
#define NORMAL_STATE (0x08)
#define OP_TYPE_MOMENTARY (0x00)
#define OP_TYPE_TOGGLE (0x01)
#define STATE_NC (0x01)
#define STATE_NO (0x00)
#define TEST_OP_CONFIG (1)

/***** Structure variable that represents the write group configuration of an output, which can be used to store information about the behavior and characteristics of an output *****/
typedef struct
{
     U8 op_en;                    // The number assigned to this output.
     U8 door_number;              // The number of the door associated with this output.
     U8 op_function;              // The function of the output, which can represent various actions or states.
     U8 op_type;                  // The type of output, momentary or toggle.
     U8 op_dflt_state;            // Default output state, whether it is NO or NC.
}output_configuration;
// Declare the structure variable of output_configuration to store the output configuration information.
extern output_configuration gb_output[MAX_OUTPUTS];

/***** Function Declarations / Prototypes *****/
void write_op_config(U8*);
void read_op_config(U8*);
void test_op_config(void);

#endif /* OUTPUT_CONFIG_H_ */