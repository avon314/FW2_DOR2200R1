/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_ip_app.h
* Created By	: Harshit Agnihotri.
* Created Date	: 11/10/2023.
* Module
* Description	: Header file for digital_ip_app.c
		       Defines constants and macros for digital_ip_app.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 11/10/2023.
* Changes		: NA.
*****************************************************************************/

/***** User Includes *****/
#include "digital_input.h"

#ifndef DIGITAL_IP_APP_H_
#define DIGITAL_IP_APP_H_

/***** Definitions / Macros *****/
#define FIRE_IP						(6)
#define EMERGENCY_IP				(7)
#define TEST_IP_FIRE_DETECT			(1)
#define TEST_GRP_FIRE_DETECT		(1)
#define TEST_IP_EMERGENCY_DETECT	(1)
#define TEST_GRP_EMERGENCY_DETECT	(1)


// Macros for input index.
#define IP_1_IDX         (0)
#define IP_2_IDX         (1)
#define IP_3_IDX         (2)
#define IP_4_IDX         (3)
#define IP_5_IDX         (4)
#define IP_6_IDX         (5)

// Macros for different functional states of input.
#define IP_ILOCK         (1)       // Input for interlock.
#define IP_ILOCK_N       (2)       // Input for interlock to normal.

#define TOG_DOR_AD       (3)       // Toggle door access denied.
#define TOG_GRP_AD       (4)       // Toggle group access denied.
#define TOG_ALLDOR_AD    (5)       // Toggle all door access denied.

#define TOG_DOR_DR       (6)       // Toggle door door release.
#define TOG_GRP_DR       (7)       // Toggle group door release.
#define TOG_ALLDOR_DR    (8)       // Toggle all door door release.

#define TOG_DOR_DRT      (9)       // Toggle door door release for drt.
#define TOG_GRP_DRT      (10)      // Toggle group door release for drt.
#define TOG_ALLDOR_DRT   (11)      // Toggle all door door release for drt.

#define TOG_DOR_AD_N     (12)      // Toggle door access denied to normal.
#define TOG_GRP_AD_N     (13)      // Toggle group access denied to normal.
#define TOG_ALLDOR_AD_N  (14)      // Toggle all door access denied to normal.

#define TOG_DOR_DR_N     (15)      // Toggle door door release to normal.
#define TOG_GRP_DR_N     (16)      // Toggle group door release to normal.
#define TOG_ALLDOR_DR_N  (17)      // Toggle all door door release to normal.

#define TOG_DOR_DRT_N    (18)      // Toggle door door release for drt to normal.
#define TOG_GRP_DRT_N    (19)      // Toggle group door release for drt to normal.
#define TOG_ALLDOR_DRT_N (20)      // Toggle all door door release for drt to normal.

#define MOM_DOR_AD       (21)      // Momentary door access denied.
#define MOM_GRP_AD       (22)      // Momentary group access denied.
#define MOM_ALLDOR_AD    (23)      // Momentary all door access denied.

#define MOM_DOR_DR       (24)      // Momentary door door release.
#define MOM_GRP_DR       (25)      // Momentary group door release.
#define MOM_ALLDOR_DR    (26)      // Momentary all door door release.

#define MOM_DOR_DRT      (27)      // Momentary door door release for drt.
#define MOM_GRP_DRT      (28)      // Momentary group door release for drt.
#define MOM_ALLDOR_DRT   (29)      // Momentary all door door release for drt.

#define MOM_DOR_AD_N     (30)      // Momentary door access denied to normal.
#define MOM_GRP_AD_N     (31)      // Momentary group access denied to normal.
#define MOM_ALLDOR_AD_N  (32)      // Momentary all door access denied to normal.

#define MOM_DOR_DR_N     (33)      // Momentary door door release to normal.
#define MOM_GRP_DR_N     (34)      // Momentary group door release to normal.
#define MOM_ALLDOR_DR_N  (35)      // Momentary all door door release to normal.

#define MOM_DOR_DRT_N    (36)      // Momentary door door release for drt to normal.
#define MOM_GRP_DRT_N    (37)      // Momentary group door release for drt to normal.
#define MOM_ALLDOR_DRT_N (38)      // Momentary all door door release for drt to normal.

#define IP_EMG_RST_TIME	(5000)

typedef struct
{
	U8 func_control_f : 1;
}IP_APP;

extern IP_APP ipAplktion;

extern volatile bool ip_fire_detect_f;            // Flag for Fire.
extern volatile bool control_emg_pin_detect;       // Flag for Emergency.
extern volatile bool emg_ip_timer_start_f;               // Flag for Emergency-Timer status.
extern volatile bool gb_ip_emg_detected_f;
extern volatile bool gb_ip_emg_reset_f;

extern U8 grp_fire_state;                         // Status of fire for each group bitwise.
extern U8 grp_emergency_state;                    // Status of emergency for each group bitwise.
extern volatile U16 emg_ip_time_count;                  // Timer for Emergency.

// Variables to store function of different inputs.
extern volatile U8 gb_ip1_func;
extern volatile U8 gb_ip2_func;
extern volatile U8 gb_ip3_func;
extern volatile U8 gb_ip4_func;
extern volatile U8 gb_ip5_func;
extern volatile U8 gb_ip6_func;

// Variable to store interlocking groups of different inputs.
extern volatile U8 gb_ip1_ilock_grps;
extern volatile U8 gb_ip2_ilock_grps;
extern volatile U8 gb_ip3_ilock_grps;
extern volatile U8 gb_ip4_ilock_grps;
extern volatile U8 gb_ip5_ilock_grps;
extern volatile U8 gb_ip6_ilock_grps;

/***** Function Declarations / Prototypes *****/
void Check_Fire_Input_detection(void);
void test_ip_fire_detect(void);
void grp_fire_detect(void);
void test_grp_fire_detect(void);
void Check_EMG_Input_Detection(void);
void test_ip_emergency_detect(void);
void Is_EMG_Input_Enabled_InGroup(void);
void test_grp_emergency_detect(void);
void Is_Fire_Input_Enabled_InGroup(void);
void Input_1_Functionality(void);
void Input_2_Functionality(void);
void Input_3_Functionality(void);
void Input_4_Functionality(void);
void Input_5_Functionality(void);
void Input_6_Functionality(void);
#endif /* DIGITAL_IP_APP_H_ */