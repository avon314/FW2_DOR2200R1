/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_input.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 05/10/2023.
* Module
* Description	: Driver to configure PORT pins as digital inputs and Implemented
*				  logic to detect a signal based on rising edge and falling edge.
*				  And setting appropriate flags.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 05/10/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "digital_input.h"

/***** Definitions / Macros *****/
/***** Connected On Port D *****/
#define DIG_INPUT_PIN_2			(PIO_PD21)
#define DIG_INPUT_PIN_1			(PIO_PD22)
#define DIG_INPUT_PIN_3			(PIO_PD23)
#define DIG_INPUT_PIN_5			(PIO_PD24)
#define DIG_INPUT_PIN_7			(PIO_PD25)
#define DIG_INPUT_PIN_8			(PIO_PD26)

/****************************************/
#define PORT_PIN_123_5_78		(PIOD)
#define ID_PORT_PIN_123_5_78	(ID_PIOD)
#define PORTD_PIN_MASK			((DIG_INPUT_PIN_1) | (DIG_INPUT_PIN_2) | (DIG_INPUT_PIN_3) |\
								(DIG_INPUT_PIN_5) | (DIG_INPUT_PIN_7) | (DIG_INPUT_PIN_8))
#define PORTD_IRQn				(PIOD_IRQn)
/***** End of section Connected On Port D *****/

/***** Connected On Port A *****/
#define DIG_INPUT_PIN_6			(PIO_PA24)
#define DIG_INPUT_PIN_4			(PIO_PA25)

/****************************************/
#define PORT_PIN_4_6			(PIOA)
#define ID_PORT_PIN_4_6			(ID_PIOA)
#define PORTA_PIN_MASK			((DIG_INPUT_PIN_4) | (DIG_INPUT_PIN_6))
#define PORTA_IRQn				(PIOA_IRQn)
/***** End of section Connected On Port A *****/

/***** Structure Variable *****/
DIG_INPUT digInput;

/*****************************************************************************
* Function name	: void Digital_Input_Driver_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/10/2023.
*
* Description	: Initialization of Port pins as an Input, enable interrupt on
*				  edge detection. Enabled clock for the peripheral
*               :
* Notes			: Pins are bit masked please check the defined macros.
* Global Variables Affected : NA.
*****************************************************************************/						 
void Digital_Input_Driver_Init(void)
{
	/*****
	* Configure Port pins as an Input and set an interrupt on edge detection.
	* for inputs 1,2,3,5,7,8
	*****/
	pmc_enable_periph_clk(ID_PORT_PIN_123_5_78);	/* Enable peripheral clock for defined PORT */
	pio_set_input(PORT_PIN_123_5_78, PORTD_PIN_MASK, PIO_DEFAULT);	/* Set defined Port pins as an input */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_1, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_2, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_3, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_5, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_7, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_8, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_enable_interrupt(PORT_PIN_123_5_78, PORTD_PIN_MASK);	/* Enable interrupt for defined port pins */
	pio_set_debounce_filter(PORT_PIN_123_5_78, PORTD_PIN_MASK, 10);    /* de-bounce period - 1/10 i.e. 100ms */
	NVIC_EnableIRQ(PORTD_IRQn); /* Set an interrupt source */
	
	/*****
	* Configure Port pins as an Input and set an interrupt on edge detection.
	* for inputs 4,6
	*****/
	pmc_enable_periph_clk(ID_PORT_PIN_4_6);	/* Enable peripheral clock for defined PORT */
	pio_set_input(PORT_PIN_4_6, PORTA_PIN_MASK, PIO_DEFAULT);	/* Set defined Port pins as an input */
	pio_handler_set(PORT_PIN_4_6, ID_PORT_PIN_4_6, DIG_INPUT_PIN_4, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_4_6, ID_PORT_PIN_4_6, DIG_INPUT_PIN_6, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_enable_interrupt(PORT_PIN_4_6, PORTA_PIN_MASK);	/* Enable interrupt for defined port pins */
	pio_set_debounce_filter(PORT_PIN_4_6, PORTA_PIN_MASK, 10);    /* de-bounce period - 1/10 i.e. 100ms */
	NVIC_EnableIRQ(PORTA_IRQn); /* Set an interrupt source */
}

/*****************************************************************************
* Function name	: void Pin_Edge_Handler(const uint32_t id, const uint32_t index)
* Returns		: Nothing.
* Arguments    	: const uint32_t id ---> PORT ID.
*				  const uint32_t index ---> Pin Index or Port pin number.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/10/2023.
*
* Description	: Pin edge handler is written to get the value of the pin state
*				  when pin edge changes from high to low or high to low.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Pin_Edge_Handler(const uint32_t id, const uint32_t index)
{
	if (id == ID_PORT_PIN_123_5_78)
	{
		switch (index)
		{
			case DIG_INPUT_PIN_1 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_1))
				{
					digInput.ip1_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip1_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip1_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip1_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_2 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_2))
				{
					digInput.ip2_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip2_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip2_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip2_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_3 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_3))
				{
					digInput.ip3_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip3_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip3_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip3_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_5 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_5))
				{
					digInput.ip5_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip5_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip5_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip5_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_7 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_7))
				{
					digInput.ip7_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip7_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip7_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip7_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_8 :
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_8))
				{
					digInput.ip8_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip8_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip8_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip8_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			default:
				__NOP();
			break;
		}
	}
	else if (id == ID_PORT_PIN_4_6)
	{
		switch (index)
		{
			case DIG_INPUT_PIN_4 :
				if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_4))
				{
					digInput.ip4_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip4_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip4_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip4_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			case DIG_INPUT_PIN_6 :
				if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_6))
				{
					digInput.ip6_lth_f = true;	// Set this flag to say that, low to high edge is detected.
					digInput.ip6_htl_f = false;	// Reset this flag because, low to high edge is detected.
				}
				else
				{
					digInput.ip6_lth_f = false;	// Reset this flag because, high to low edge is detected.
					digInput.ip6_htl_f = true;	// Set this flag to say that, high to low edge is detected.
				}
			break;
			default:
				__NOP();
			break;
		}
	}
	else
	{
		__NOP();
	}
}

/*****************************************************************************
* Function name	: void Handle_PowerOn_Input_Flags(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/07/2023.
* Description	: Set the input flags at power on by the digital input pin state.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Handle_PowerOn_Input_Flags(void)
{
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_1))
	{
		digInput.ip1_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip1_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip1_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip1_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_2))
	{
		digInput.ip2_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip2_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip2_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip2_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_3))
	{
		digInput.ip3_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip3_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip3_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip3_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_5))
	{
		digInput.ip5_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip5_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip5_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip5_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_7))
	{
		digInput.ip7_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip7_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip7_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip7_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_8))
	{
		digInput.ip8_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip8_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip8_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip8_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_4))
	{
		digInput.ip4_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip4_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip4_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip4_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
	
	if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_6))
	{
		digInput.ip6_lth_f = true;	// Set this flag to say that, low to high edge is detected.
		digInput.ip6_htl_f = false;	// Reset this flag because, low to high edge is detected.
	}
	else
	{
		digInput.ip6_lth_f = false;	// Reset this flag because, high to low edge is detected.
		digInput.ip6_htl_f = true;	// Set this flag to say that, high to low edge is detected.
	}
}