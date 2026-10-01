/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: fire_functionality.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: Header file for fire_functionality.c
				  Defines constants and macros for fire_functionality.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NO.
*****************************************************************************/
#ifndef FIRE_FUNCTIONALITY_H_
#define FIRE_FUNCTIONALITY_H_
/* User Includes */
#include "interlock.h"

/* Constant Variables */
enum GRP_IDX
{
	GROUP1,
	GROUP2,
	GROUP3,
	GROUP4,
	GROUP5,
	GROUP6,
	GROUP7,
	GROUP8
};

/* Structure declaration */
typedef struct  
{
	U8 fire_control_f			: 1;
	U8 f_executing_command		: 1;
	U8 chk_op_sts_after_cmd		: 1;
	U8 is_indInput_detect_f		: 1;
	U8 reset_from_aux_ip		: 1;
	U8 chk_fip_after_ipRst_f	: 1;
// 	U8 istatr_slv_pwrUp_rcvd	: 1;
	
	U8 free_acs_active_by[TOTAL_SLAVES];
}FIRE;

typedef struct
{
	U8 fire_req_detect_f : 1;
	U8 acsD_req_detect_f : 1;
	U8 grp_in_free_access : 1;
	U8 grp_in_acs_denied : 1;
}FIREGROUP;

/* Global structure variables */
extern FIRE fireFlags;
extern FIREGROUP fireGroup[NUM_OF_GROUPS];

/* Function Prototypes */
void Execute_Fire_Functionality(void);
U8 Find_Fire_Devices(void);
void Reset_Fire_Control_Flags(void);
U8 Find_All_FA_Group_Devices(void);
U8 Find_All_AD_Group_Devices(void);
U8 Find_All_DRT_Group_Devices(void);
#endif /* FIRE_FUNCTIONALITY_H_ */
