/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: ext_eeprom.h
* Created By	: Harshit Agnihotri
* Created Date	: 30/03/2024
* Module
* Description	: Header file for ext_eeprom.c
				  Defines constants and macros for ext_eeprom.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 30/03/2024
* Changes		: NA
*****************************************************************************/

#ifndef EXT_EEPROM_H_
#define EXT_EEPROM_H_

#define WRITE_PROTECT_PORT    (PIOA)                                            // Port of pin used for write protect in eeprom.
#define WRITE_PROTECT_PIN     (PIO_PA5)                                         // Pin used for write protect in eeprom.


/*	Configuration layout granularity used by app_eeprom.c. It is kept for both
	hardware versions so the configuration addresses stay the same. */
#define EEPROM_ADDR           (0x50)                                            // I2C address of U4 (AT24MAC402) EEPROM array.
#define PAGE_SIZE             (32)                                              // Page size in bytes.
#define MAX_FRAME_SIZE        (256)                                             // Maximum frame size to write or read.
#define DATA_SIZE             (PAGE_SIZE * MAX_FRAME_SIZE)                      // Total data size to write or read.

// Function prototypes
void configure_twi(void);
void eeprom_pin_config(void);
void eeprom_write_byte(uint16_t addr, uint8_t data);
uint8_t eeprom_read_byte(uint16_t addr);
void eeprom_write_frame(uint16_t address, uint8_t *data, uint16_t length);
void eeprom_read_frame(uint16_t address, uint8_t *data, uint16_t length);
void erase_eeprom(void);
void read_cplt_eeprom(void);
void eeprom_flush_state(void);
void eeprom_load_mac_address(void);

#endif /* EXT_EEPROM_H_ */