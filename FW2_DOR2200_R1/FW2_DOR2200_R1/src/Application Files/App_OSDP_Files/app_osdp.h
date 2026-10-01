/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_osdp.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 07/11/2023.
* Module
* Description	: Header file for app_osdp.c
				  Defines constants and macros for app_osdp.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 07/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef APP_OSDP_H_
#define APP_OSDP_H_

/* System Includes */
#include "asf.h"

/* User Includes */
#include "osdp_protocol_master.h"

/* General macros */
#ifndef FLAG_SET
	#define FLAG_SET	(1)
#endif
#ifndef FLAG_RST
	#define FLAG_RST	(0)
#endif
/* End of General macros */

/***** Definitions for frame transmission and reception timings *****/
#define POLL_DELAY			(100)	// in milli seconds.
#define FRAME_REPLY_DELAY	(50)	// in milli seconds.
#define STOP_TIME			(0)		// in milli seconds.
#define MAX_RETRY_COUNT		(3)		/* Frame retry count */
#define INTER_FRAME_GAP_MS  (10)    // in milli seconds.

#if FRAME_REPLY_DELAY > MAX_REPLY_DELAY
	#define FRAME_REPLY_DELAY	MAX_REPLY_DELAY
#endif
/***** End of Definitions for frame transmission and reception timings *****/

#define DEV_ONLINE_TIME		(2000) /* Time in ms. */

#define OSDP_MAX_DEVICE	(16)
#define TOTAL_SLAVES		(OSDP_MAX_DEVICE)

/***** Index values for inputs *****/
#define IDX_DPS						(0)
#define IDX_LFB						(1)
#define IDX_IP_TYPE_STATE			(2)
#define IDX_IP_TYPE					(3)
#define IDX_STATE_ACTION			(4)
#define IDX_AUX_AUDIO				(5)
/***** End of Index values for inputs *****/

/***** Definition for slave operational state *****/
#define SLV_OP_STATE_NORMAL			(1)
#define SLV_OP_STATE_DOOR_ACTIVE	(2)
#define SLV_OP_STATE_INTERLOCK		(3)
#define SLV_OP_STATE_ACCESS_DENIED	(4)
#define SLV_OP_STATE_FREE_ACCESS	(5)
#define SLV_OP_STATE_EMERGENCY		(6)
#define SLV_OP_STATE_PRIVACY		(7)
/***** End of Definition for slave operational state *****/

/***** Definition for slave Alarm Value *****/
#define SLV_ALRM_NOT_SET			(0)
#define SLV_ALRM_FORCE_DR			(1)
#define SLV_ALRM_NOT_LOCKED			(2)
#define SLV_ALRM_PROPPED_DR			(3)
/***** End of Definition for slave Alarm Value *****/

typedef struct
{
	U8 gb_enable_poll : 1;		/* Flag variable used to enable or disable polling */
	U8 gb_poll_flag : 1;		/* Flag used to check poll flag is set or not */
	U8 gb_frame_not_rcvd_f : 1;	/* Flag used if frame is not received within defined frame receive time. */
	U8 gb_retry_f : 1;			/* Flag used to set retry frame */
	U8 gb_transmit_f : 1;		/* Flag used to send data over the osdp protocol */
	U8 gb_rd_oprt_state_f : 1;	/* Flag used to read operational state value of slaves */
	U8 gb_rd_ip_status_f : 1;	/* Flag used to read the input status of the slaves */
	U8 put_into_free_access : 1;	/* Flag used to put devices into free access state */
	U8 put_into_acs_denied : 1;	/* Flag used to put devices into access denied state */
	U8 put_into_privacy_state : 1;
	U8 chk_free_acs_ack_f : 1;
	U8 chk_singleDr_fa_ack_f : 1;
	U8 chk_singleDr_ad_ack_f : 1;
	U8 chk_acs_dnd_ack_f : 1;
	U8 chk_prv_ack_f : 1;
	U8 free_to_normal_f : 1;
	U8 norReadyIdx : 1;
	U8 gb_retry_count;			/* Variable used as count variable. */
	U8 polling_device;			/* Variable used t fill the current polling device */
	U8 last_dev_address;	/* Variable used to store dev address before transmission. */
	
	U8 istTransIdx;
	U8 istRecIdx;
	U8 oprTransIdx;
	U8 oprRecIdx;
	U8 ilockTransIdx;
	U8 ilockRecIdx;
	U8 nrmlTransIdx;
	U8 nrmlRecIdx;
	U8 emgTransIdx;
	U8 emgAckIdx;
	U8 freeTransIdx;
	U8 freeAckIdx;
	U8 acsTransIdx;
	U8 acsAckIdx;
	U8 prvTransIdx;
	U8 prvAckIdx;
	U8 norTransIdx;
	U8 norAckIdx;
	U8 outTransIdx;
	U8 outAckIdx;
	U16 gb_poll_time;			/* Variable which has poll time in ms. */
	U16 gb_frame_reply_time;	/* Variable which has frame reply time in ms */
}OSDP_APP;

typedef struct
{
	U8 doorState : 1;			/* Variable holds the status of the door. */
	U8 lockState : 1;			/* Variable holds the lock status of the door. */
	U8 ipTypeState : 1;			/* Variable holds the state of the input type (Active or low?). */
	U8 ipType : 1;				/* Variable holds the input type (Push or Toggle?). */
	U8 ipStateAction : 1;		/* Variable holds the value of state action (local or global?) */
	U8 auxAudioSelect : 1; 		/* Variable holds the data of Auxiliary audio selected or not? */
	U8 oprtState;				/* Type of the slave operational state. */
	U8 alarmState;				/* Value of ongoing alarm in the slave. */
	U8 oldOprtState;
}SLAVE_DATA;

typedef struct
{
	U8 slv_addr;
}TOTALDEV;


/***** Extern Variables *****/
U8 gb_power_on_flag;

//extern U16 gb_online_time[TOTAL_SLAVES];
extern U8 inSystem_dev_len;
extern U8 polling_dev_len;
extern OSDP_APP osdp_app;
extern OSDP_APP copyosdp_app;
extern SLAVE_DATA slv_data[TOTAL_SLAVES];
extern SLAVE_DATA slvTemp_data[TOTAL_SLAVES];
extern TOTALDEV inSysDeviceList[TOTAL_SLAVES];
extern TOTALDEV PollDevice[TOTAL_SLAVES];

extern bool gb_slave_aux_audio;

/***** Function declaration / prototypes *****/
void OSDP_Transmit_Task(void *pvParameters);
void OSDP_Poll_Delay(void);
void OSDP_Frame_Response_Time(void);
void Send_OSDP_Frame_To_Slave(U8 *bufdata, U8 buflen);
void Select_Command_transmission(void);
void Check_Door_Active_State(U8 dvIdx);
U8 Map_Operation_State_To_MBReg(U8 opStateValue);
U8 Find_Device_Index_InSystem(U8 dev_address);
void Reset_Flags_Error_After_Retry(U8 rstDevAdd);
void Match_Operation_Status_For_Output(void);
void Find_Polling_Devices(void);
void Reset_devNoResponse_flags(void);
#endif /* APP_OSDP_H_ */