/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: ext_eeprom.c
* Created By	: Harshit Agnihotri
* Created Date	: 30/03/2024
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash
*					128 KB		RAM
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 30/03/2024
* Changes		: NA
/*****************************************************************************/

/* System Includes */
#include "asf.h"
#include "user_uart.h"
#include "twi.h"

/* User Includes */
#include "ext_eeprom.h"
#include "definitions.h"

/*****************************************************************************
* Function name: void eeprom_pin_config(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Write Protection Pin is disable to write in specified memory address.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void eeprom_pin_config(void)
{
     // Disable the WP (Write Protection Pin).
     pio_set_output(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN, LOW, DISABLE, ENABLE);
}

/*****************************************************************************
* Function name: void configure_twi(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Initialize TWI driver with I2C bus clock set to 100kHz.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void configure_twi(void)
{
     /* Initialize TWI driver with I2C bus clock set to 100kHz. */
     twi_options_t opt;
     opt.master_clk = sysclk_get_cpu_hz();
     opt.speed = 100000;
     if(twi_master_setup(TWI0, &opt) != TWI_SUCCESS)
     {

          #if DEBUG_ALL || DEBUG_EXT_EEPROM
          Print_Message("\nTWI initialization failed.");
          #endif

     }
}

/*****************************************************************************
* Function name: void eeprom_write_byte(uint16_t addr, uint8_t data)
* Returns		: None
* Arguments	: uint16_t addr, uint8_t data
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Writes 1 byte of data to specified memory address.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void eeprom_write_byte(uint16_t addr, uint8_t data)
{
     // Configure the Packet to write.
     twi_package_t packet_tx =
     {
          .addr[0] = addr >> 8,
          .addr[1] = addr & 0xFF,
          .addr_length = 2,
          .buffer = &data,
          .chip = EEPROM_ADDR,
          .length = 1
     };
     // Write the configured packet.
     pio_clear(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN);
     if(twi_master_write(TWI0, &packet_tx) != TWI_SUCCESS)
     {

          #if DEBUG_ALL || DEBUG_EXT_EEPROM
          Print_Message("\nFailed to write 1 byte of data to eeprom's specified memory address.");
          #endif

     }
     pio_set(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN);
}

/*****************************************************************************
* Function name: uint8_t eeprom_read_byte(uint16_t addr)
* Returns		: uint8_t
* Arguments	: uint16_t addr
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Reads 1 byte of data from specified memory address.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
uint8_t eeprom_read_byte(uint16_t addr)
{
     uint8_t data = 0;
     // Configure the packet to read.
     twi_package_t packet_rx =
     {
          .addr[0] = addr >> 8,
          .addr[1] = addr & 0xFF,
          .addr_length = 2,
          .buffer = &data,
          .chip = EEPROM_ADDR,
          .length = 1
     };
     // Read the configured packet.
     if(twi_master_read(TWI0, &packet_rx) != TWI_SUCCESS)
     {

          #if DEBUG_ALL || DEBUG_EXT_EEPROM
          Print_Message("\nFailed to read 1 byte of data from eeprom's specified memory address.");
          #endif

     }
     return data;
}

/*****************************************************************************
* Function name: void eeprom_write_frame(uint16_t address, uint8_t *data, uint16_t length)
* Returns		: None
* Arguments	: uint16_t address, uint8_t *data, uint16_t length
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Writes the data at specified address location in eeprom.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void eeprom_write_frame(uint16_t address, uint8_t *data, uint16_t length)
{
     uint16_t remaining_bytes = length;
     uint16_t bytes_written = 0;
     uint16_t current_address = address;
     while (remaining_bytes > 0)
     {
          uint16_t bytes_to_write = (remaining_bytes > PAGE_SIZE) ? PAGE_SIZE : remaining_bytes;

          twi_packet_t packet =
          {
               .chip = EEPROM_ADDR,                                             // Chip address.
               .addr[0] = (current_address >> 8) & 0xFF,                        // EEPROM address MSB.
               .addr[1] = current_address & 0xFF,                               // EEPROM address LSB.
               .addr_length = 2,                                                // 2-byte address.
               .buffer = &data[bytes_written],                                  // Data buffer to write.
               .length = bytes_to_write                                         // Number of bytes to write.
          };

          pio_clear(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN);
          if(twi_master_write(TWI0, &packet) != TWI_SUCCESS)
          {

               #if DEBUG_ALL || DEBUG_EXT_EEPROM
               Print_Message("\nFailed to write frame of data to eeprom's specified memory address.");
               #endif

          }
          pio_set(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN);

          remaining_bytes -= bytes_to_write;
          bytes_written += bytes_to_write;
          current_address += bytes_to_write;
          delay_ms(2);
          // Delay or other handling may be necessary depending on EEPROM write time.
     }
}

/*****************************************************************************
* Function name: void eeprom_read_frame(uint16_t address, uint8_t *data, uint16_t length)
* Returns		: None
* Arguments	: uint16_t address, uint8_t *data, uint16_t length
* Created by	: Harshit Agnihotri
* Date created	: 30-03-2024
* Description	: Reads the data from specified address location from eeprom.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void eeprom_read_frame(uint16_t address, uint8_t *data, uint16_t length)
{
     twi_packet_t packet =
     {
          .chip = EEPROM_ADDR,                                                  // Chip address.
          .addr[0] = (address >> 8) & 0xFF,                                     // EEPROM address MSB.
          .addr[1] = address & 0xFF,                                            // EEPROM address LSB.
          .addr_length = 2,                                                     // 2-byte address.
          .buffer = data,                                                       // Data buffer to read into.
          .length = length                                                      // Number of bytes to read.
     };

     if(twi_master_read(TWI0, &packet) != TWI_SUCCESS)
     {

          #if DEBUG_ALL || DEBUG_EXT_EEPROM
          Print_Message("\nFailed to read frame of data from eeprom's specified memory address.");
          #endif

     }
}

/*****************************************************************************
* Function name: void erase_eeprom(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 03-04-2024
* Description	: Erase the complete eeprom by writing 0x00 at all locations.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void erase_eeprom(void)
{
     uint16_t address = 0x0000;
     uint8_t zero_data = 0xFF; // Value to write for erasing.

     // Iterate through each address in the EEPROM and write zero_data.
     while (address < DATA_SIZE)
     {
          eeprom_write_byte(address, zero_data);
          address++;
          delay_ms(2);
          // Delay or other handling may be necessary depending on EEPROM write time.
     }
}

/*****************************************************************************
* Function name: void read_cplt_eeprom(void)
* Returns		: None
* Arguments	: None
* Created by	: Harshit Agnihotri
* Date created	: 04-04-2024
* Description	: Read complete 8192 bytes of EEPROM memory.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void read_cplt_eeprom(void)
{
     uint16_t address = 0x0000;                        // First address of eeprom.
     uint8_t read_data[8192];
     eeprom_read_frame(address, read_data, 8192);      // Read complete memory of 8192 bytes.
     Print_Message("\nData read : ");
     Send_Frame_On_UART(read_data, 8192);
}