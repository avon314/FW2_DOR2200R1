/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_input.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 05/10/2023.
* Module
* Description	: Header file for digital_input.c
				  Defines constants and macros for digital_input.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 05/10/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef DIGITAL_INPUT_H_
#define DIGITAL_INPUT_H_

/***** Structure variable which holds the flags for digital inputs edge detection *****/
typedef struct
{
	volatile U8 ip1_htl_f : 1;	/* Input 1 High to low edge detection flag */
	volatile U8 ip2_htl_f : 1;	/* Input 2 High to low edge detection flag */
	volatile U8 ip3_htl_f : 1;	/* Input 3 High to low edge detection flag */
	volatile U8 ip4_htl_f : 1;	/* Input 4 High to low edge detection flag */
	volatile U8 ip5_htl_f : 1;	/* Input 5 High to low edge detection flag */
	volatile U8 ip6_htl_f : 1;	/* Input 6 High to low edge detection flag */
	volatile U8 ip7_htl_f : 1;	/* Input 7 High to low edge detection flag */
	volatile U8 ip8_htl_f : 1;	/* Input 8 High to low edge detection flag */
	
	volatile U8 ip1_lth_f : 1;	/* Input 1 Low to high edge detection flag */
	volatile U8 ip2_lth_f : 1;	/* Input 2 Low to high edge detection flag */
	volatile U8 ip3_lth_f : 1;	/* Input 3 Low to high edge detection flag */
	volatile U8 ip4_lth_f : 1;	/* Input 4 Low to high edge detection flag */
	volatile U8 ip5_lth_f : 1;	/* Input 5 Low to high edge detection flag */
	volatile U8 ip6_lth_f : 1;	/* Input 6 Low to high edge detection flag */
	volatile U8 ip7_lth_f : 1;	/* Input 7 Low to high edge detection flag */
	volatile U8 ip8_lth_f : 1;	/* Input 8 Low to high edge detection flag */
}DIG_INPUT;

extern DIG_INPUT digInput;

/***** Function Declarations / Prototypes *****/
void Digital_Input_Driver_Init(void);
void Pin_Edge_Handler(const uint32_t id, const uint32_t index);
void Handle_PowerOn_Input_Flags(void);
#endif /* DIGITAL_INPUT_H_ */
