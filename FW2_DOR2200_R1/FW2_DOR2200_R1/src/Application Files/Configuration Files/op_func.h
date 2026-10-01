/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: op_func.h
* Created By	: Harshit Agnihotri.
* Created Date	: 16/03/2024.
* Module
* Description	: Header file for op_func.c
				  Defines constants and macros for op_func.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 16/03/2024.
* Changes		: NA.
*****************************************************************************/


#ifndef OP_FUNC_H_
#define OP_FUNC_H_

#ifndef MAX_DOORS
#define MAX_DOORS (16)
#endif

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "definitions.h"

// Macros for defining output indices.
#define OP_1_IDX         (0)
#define OP_2_IDX         (1)
#define OP_3_IDX         (2)
#define OP_4_IDX         (3)

// Time duration for reset.
#define OP_RST_TIME      (500)

// External declarations for door and operational states.
extern volatile U8 door_state[MAX_DOORS];
extern volatile U8 operational_state[MAX_DOORS];

// External declarations for momentary and toggle set/reset flags.
extern volatile U8 gb_op_mom_set_reset_f;
extern volatile U8 gb_op_tog_set_reset_f;

// External declarations for momentary timer running flag and timer array.
extern volatile U8 gb_op_mom_timer_running;
extern volatile U16 gb_op_mom_timer[4];

// External declaration for momentary pulse flag.
extern volatile U8 gb_op_mom_pulse;

// External declarations for test index, frame received flag, and test frame buffer.
extern volatile U8 gb_op_test_idx;
extern volatile U8 gb_op_test_frame_received_f;
extern volatile U8 gb_op_test_frame_buff[2];

/***** Function Declarations / Prototypes *****/
void Output_1_Functionality(void);
void operate_relay_1_mom(void);
void Output_2_Functionality(void);
void operate_relay_2_mom(void);
void Output_3_Functionality(void);
void operate_relay_3_mom(void);
void Output_4_Functionality(void);
void operate_relay_4_mom(void);
void update_op_test_buff(void);
void Set_Default_State_NO_NC (void);

#endif /* OP_FUNC_H_ */