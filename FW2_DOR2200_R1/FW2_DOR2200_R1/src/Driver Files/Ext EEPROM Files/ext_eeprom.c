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

#if (BOARD_HW_VERSION != 5)
/*****************************************************************************
*	V3 storage: I2C EEPROM U4.
*****************************************************************************/

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

/* No buffered state on V3: every write goes to the EEPROM directly. */
void eeprom_flush_state(void)
{
}

#endif /* (BOARD_HW_VERSION != 5) */

#if (BOARD_HW_VERSION == 5)
/*****************************************************************************
*	V5 storage: M95P32 32-Mbit SPI page EEPROM (U16).
*
*	Hardware (schematic LTPL-0823-147-V5):
*		SPI MISO/MOSI/SPCK	: PA12 / PA13 / PA14 (SPI peripheral A)
*		S  (chip select)	: PA16 "CS2"  (GPIO, active low)
*		W  (write protect)	: PA21 "WR_PRT" (held high: writes allowed)
*		AT45DB321E flash U3 shares the bus: FLASH_CS1 PA15 is held high.
*
*	The application keeps using the same logical addresses as on V3:
*	- Configuration (address 0 .. GEN_MEM_BASE_ADD-1) is written to the same
*	  addresses in the M95P32.
*	- The power-on state window (GEN_MEM_BASE_ADD ...) is changed on every door
*	  state change. It is kept in RAM and saved as a 64-byte record in a ring of
*	  STATE_JRNL_PAGES pages, at most once per STATE_FLUSH_MIN_MS, so the same
*	  EEPROM page is not rewritten on every change.
*****************************************************************************/

#include "spi.h"
#include "string.h"

/* lwIP port (sam4e_gmac.c) */
extern void ethernetif_set_mac_address(const uint8_t *puc_mac);

/***** M95P32 instructions (ST datasheet DS12964 Rev 6, Table 12) *****/
#define M95P_CMD_WREN			(0x06)	/* Write enable */
#define M95P_CMD_RDSR			(0x05)	/* Read status register */
#define M95P_CMD_READ			(0x03)	/* Read data, single output (max 50 MHz) */
#define M95P_CMD_PGWR			(0x02)	/* Page write: auto erase + program of 1..512 bytes,
										   other bytes of the page unchanged */
#define M95P_CMD_JEDID			(0x9F)	/* JEDEC identification: 20h, 00h, 16h */
#define M95P_SR_WIP				(0x01)	/* Status: write in progress (also set during power-up) */
#define M95P_PAGE_SIZE			(512u)
#define M95P_WRITE_TIMEOUT_US	(20000u)	/* tPW max 4.5 ms (Table 27) */
#define M95P_SPI_CLOCK_HZ		(8000000u)
#define M95P_JEDEC_MFR			(0x20)	/* ST */
#define M95P_JEDEC_FAMILY		(0x00)	/* SPI family */
#define M95P_JEDEC_DENSITY		(0x16)	/* 32 Mbit */

/***** Pins *****/
#define SPIEE_CS_PORT			(PIOA)
#define SPIEE_CS_PIN			(PIO_PA16)	/* CS2 */
#define SPIEE_WP_PORT			(PIOA)
#define SPIEE_WP_PIN			(PIO_PA21)	/* WR_PRT */
#define SPIFLASH_CS_PORT		(PIOA)
#define SPIFLASH_CS_PIN			(PIO_PA15)	/* FLASH_CS1, AT45DB321E (not used) */
#define SPIEE_SPI_PINS			(PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK)
#define SPIEE_SPI_NPCS			(0)			/* Chip select register used; CS itself is GPIO */

/***** Power-on state journal *****/
#define STATE_WIN_BASE			(GEN_MEM_BASE_ADD)	/* First logical address kept in RAM */
#define STATE_WIN_SIZE			(56u)				/* GEN_MEM_BASE_ADD .. +55 (used up to GEN_MEM_FIRE_STATE) */
#define STATE_JRNL_BASE			(0x010000ul)		/* Away from the configuration area */
#define STATE_JRNL_PAGES		(256u)				/* 128 KB ring */
#define STATE_REC_SIZE			(64u)
#define STATE_JRNL_SLOTS		((STATE_JRNL_PAGES * M95P_PAGE_SIZE) / STATE_REC_SIZE)
#define STATE_REC_MAGIC			(0xA55Au)
#define STATE_FLUSH_MIN_MS		(1000u)

typedef struct
{
	U32 seq;
	U16 magic;
	U8  data[STATE_WIN_SIZE];
	U16 crc;
} STATE_REC;

/* The record must fill exactly one slot (no padding). */
typedef char state_rec_size_check[(sizeof(STATE_REC) == STATE_REC_SIZE) ? 1 : -1];

static U8  stateWin[STATE_WIN_SIZE];	/* RAM copy of the power-on state window */
static U8  stateDirty = 0;
static U32 stateSeq = 0;				/* Sequence number of the last saved record */
static U16 stateSlot = 0;				/* Slot of the last saved record */
static U32 stateLastFlushTick = 0;
static U8  spiee_ready = 0;

static void spiee_init(void);
static U8   spiee_lock(void);
static void spiee_unlock(U8 locked);
static U8   spiee_xfer(U8 byte);
static void spiee_cmd_addr(U8 cmd, U32 addr);
static U8   spiee_wait_ready(void);
static void spiee_read(U32 addr, U8 *data, U32 len);
static U8   spiee_write(U32 addr, const U8 *data, U32 len);
static U16  state_crc16(const U8 *data, U16 len);
static void state_journal_load(void);

#define SPIEE_SELECT()		pio_clear(SPIEE_CS_PORT, SPIEE_CS_PIN)
#define SPIEE_DESELECT()	pio_set(SPIEE_CS_PORT, SPIEE_CS_PIN)

/*****************************************************************************
* Function name: static void spiee_init(void)
* Description	: Configure the SPI peripheral and the memory control pins.
*****************************************************************************/
static void spiee_init(void)
{
	pmc_enable_periph_clk(ID_PIOA);
	
	/* Deselect both SPI memories and allow writes before the bus is enabled. */
	pio_set_output(SPIEE_CS_PORT, SPIEE_CS_PIN, HIGH, DISABLE, ENABLE);
	pio_set_output(SPIFLASH_CS_PORT, SPIFLASH_CS_PIN, HIGH, DISABLE, ENABLE);
	pio_set_output(SPIEE_WP_PORT, SPIEE_WP_PIN, HIGH, DISABLE, ENABLE);
	pio_configure(PIOA, PIO_PERIPH_A, SPIEE_SPI_PINS, PIO_DEFAULT);
	
	spi_enable_clock(SPI);
	spi_disable(SPI);
	spi_reset(SPI);
	spi_set_master_mode(SPI);
	spi_disable_mode_fault_detect(SPI);
	spi_disable_peripheral_select_decode(SPI);
	spi_set_fixed_peripheral_select(SPI);
	spi_set_peripheral_chip_select_value(SPI, spi_get_pcs(SPIEE_SPI_NPCS));
	spi_set_clock_polarity(SPI, SPIEE_SPI_NPCS, 0);		/* SPI mode 0 */
	spi_set_clock_phase(SPI, SPIEE_SPI_NPCS, 1);
	spi_set_bits_per_transfer(SPI, SPIEE_SPI_NPCS, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI, SPIEE_SPI_NPCS,
						 spi_calc_baudrate_div(M95P_SPI_CLOCK_HZ, sysclk_get_peripheral_hz()));
	spi_configure_cs_behavior(SPI, SPIEE_SPI_NPCS, SPI_CS_KEEP_LOW);
	spi_enable(SPI);
	
	delay_ms(1);	/* tVSL: 30 us min from VCC(min) to S low (Table 15) */
	
	/*	WIP is 1 while the device completes its power-up. If power-up failed (PUF)
		the status register reads FFh and this times out. */
	U8 powered_up = spiee_wait_ready();
	
	SPIEE_SELECT();
	spiee_xfer(M95P_CMD_JEDID);
	U8 id0 = spiee_xfer(0xFF);
	U8 id1 = spiee_xfer(0xFF);
	U8 id2 = spiee_xfer(0xFF);
	SPIEE_DESELECT();
	
	#if DEBUG_ALL || DEBUG_EXT_EEPROM
	Print_Message("\nSPI EEPROM JEDEC ID : ");
	Print_Number(id0);
	Print_Message(",");
	Print_Number(id1);
	Print_Message(",");
	Print_Number(id2);
	#endif
	
	/*	Use the memory only if it answers as an M95P32. Otherwise reads return
		FFh (configuration CRC fails, defaults are used) and writes are skipped. */
	if ((powered_up == 1) && (id0 == M95P_JEDEC_MFR) && (id1 == M95P_JEDEC_FAMILY) && (id2 == M95P_JEDEC_DENSITY))
	{
		spiee_ready = 1;
	}
	else
	{
		spiee_ready = 0;
		
		#if DEBUG_ALL || DEBUG_EXT_EEPROM
		Print_Message("\nSPI EEPROM not found, configuration and state are not stored.");
		#endif
	}
}

/*****************************************************************************
* Function name: static U8 spiee_lock(void) / static void spiee_unlock(U8 locked)
* Description	: The EEPROM is used from several tasks (configuration save in the
*				  CONFIG_MODE task, power-on state and default IP key in
*				  General_Task). A task switch in the middle of an SPI command
*				  sequence would interleave two commands on the bus, so task
*				  switching is suspended for each EEPROM operation. Interrupts
*				  stay enabled. Before the scheduler starts nothing is needed.
*****************************************************************************/
static U8 spiee_lock(void)
{
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		vTaskSuspendAll();
		return 1;
	}
	return 0;
}

static void spiee_unlock(U8 locked)
{
	if (locked)
	{
		xTaskResumeAll();
	}
}

/*****************************************************************************
* Function name: static U8 spiee_xfer(U8 byte)
* Description	: Send one byte on SPI and return the byte received.
*****************************************************************************/
static U8 spiee_xfer(U8 byte)
{
	while ((SPI->SPI_SR & SPI_SR_TDRE) == 0);
	SPI->SPI_TDR = byte;
	while ((SPI->SPI_SR & SPI_SR_RDRF) == 0);
	return (U8)(SPI->SPI_RDR & 0xFF);
}

/*****************************************************************************
* Function name: static void spiee_cmd_addr(U8 cmd, U32 addr)
* Description	: Send an instruction followed by a 24-bit address.
*****************************************************************************/
static void spiee_cmd_addr(U8 cmd, U32 addr)
{
	spiee_xfer(cmd);
	spiee_xfer((U8)(addr >> 16));
	spiee_xfer((U8)(addr >> 8));
	spiee_xfer((U8)addr);
}

/*****************************************************************************
* Function name: static U8 spiee_wait_ready(void)
* Returns		: 1 when the memory is ready, 0 on timeout.
* Description	: Poll the status register until the write cycle is complete.
*****************************************************************************/
static U8 spiee_wait_ready(void)
{
	for (U32 waited = 0; waited < M95P_WRITE_TIMEOUT_US; waited += 20)
	{
		SPIEE_SELECT();
		spiee_xfer(M95P_CMD_RDSR);
		U8 status = spiee_xfer(0xFF);
		SPIEE_DESELECT();
		
		if ((status & M95P_SR_WIP) == 0)
		{
			return 1;
		}
		delay_us(20);
	}
	
	#if DEBUG_ALL || DEBUG_EXT_EEPROM
	Print_Message("\nSPI EEPROM write timeout.");
	#endif
	return 0;
}

/*****************************************************************************
* Function name: static void spiee_read(U32 addr, U8 *data, U32 len)
* Description	: Read len bytes from the SPI EEPROM.
*****************************************************************************/
static void spiee_read(U32 addr, U8 *data, U32 len)
{
	if (spiee_ready == 0)
	{
		memset(data, 0xFF, len);
		return;
	}
	
	SPIEE_SELECT();
	spiee_cmd_addr(M95P_CMD_READ, addr);
	for (U32 idx = 0; idx < len; idx++)
	{
		data[idx] = spiee_xfer(0xFF);
	}
	SPIEE_DESELECT();
}

/*****************************************************************************
* Function name: static U8 spiee_write(U32 addr, const U8 *data, U32 len)
* Returns		: 1 when all data was written and read back correctly.
* Description	: Write len bytes, split at 512-byte page boundaries. Each part
*				  is read back and compared, and written once more on mismatch.
*****************************************************************************/
static U8 spiee_write(U32 addr, const U8 *data, U32 len)
{
	U8 ok = 1;
	
	if (spiee_ready == 0)
	{
		return 0;
	}
	
	while (len > 0)
	{
		U32 room = M95P_PAGE_SIZE - (addr % M95P_PAGE_SIZE);
		U32 chunk = (len < room) ? len : room;
		U8 chunk_ok = 0;
		
		for (U8 attempt = 0; (attempt < 2) && (chunk_ok == 0); attempt++)
		{
			SPIEE_SELECT();
			spiee_xfer(M95P_CMD_WREN);
			SPIEE_DESELECT();
			
			SPIEE_SELECT();
			spiee_cmd_addr(M95P_CMD_PGWR, addr);
			for (U32 idx = 0; idx < chunk; idx++)
			{
				spiee_xfer(data[idx]);
			}
			SPIEE_DESELECT();
			
			if (spiee_wait_ready() == 0)
			{
				continue;
			}
			
			/* Verify */
			chunk_ok = 1;
			SPIEE_SELECT();
			spiee_cmd_addr(M95P_CMD_READ, addr);
			for (U32 idx = 0; idx < chunk; idx++)
			{
				if (spiee_xfer(0xFF) != data[idx])
				{
					chunk_ok = 0;
				}
			}
			SPIEE_DESELECT();
		}
		
		if (chunk_ok == 0)
		{
			ok = 0;
			
			#if DEBUG_ALL || DEBUG_EXT_EEPROM
			Print_Message("\nSPI EEPROM write/verify failed at address : ");
			Print_Number(addr);
			#endif
		}
		
		addr += chunk;
		data += chunk;
		len -= chunk;
	}
	
	return ok;
}

/*****************************************************************************
* Function name: static U16 state_crc16(const U8 *data, U16 len)
* Description	: CRC-16/CCITT of a state record.
*****************************************************************************/
static U16 state_crc16(const U8 *data, U16 len)
{
	U16 crc = 0xFFFF;
	for (U16 idx = 0; idx < len; idx++)
	{
		crc ^= (U16)data[idx] << 8;
		for (U8 bit = 0; bit < 8; bit++)
		{
			crc = (crc & 0x8000) ? (U16)((crc << 1) ^ 0x1021) : (U16)(crc << 1);
		}
	}
	return crc;
}

/*****************************************************************************
* Function name: static void state_journal_load(void)
* Description	: Find the newest valid state record and load it into RAM. With
*				  no valid record (new board) the window reads 0xFF, as a blank
*				  EEPROM did on V3.
*****************************************************************************/
static void state_journal_load(void)
{
	STATE_REC rec;
	U8 found = 0;
	
	memset(stateWin, 0xFF, sizeof(stateWin));
	stateSeq = 0;
	stateSlot = STATE_JRNL_SLOTS - 1;	/* First save goes to slot 0 */
	
	for (U16 slot = 0; slot < STATE_JRNL_SLOTS; slot++)
	{
		spiee_read(STATE_JRNL_BASE + ((U32)slot * STATE_REC_SIZE), (U8 *)&rec, sizeof(rec));
		
		if ((rec.magic == STATE_REC_MAGIC)
		&& (rec.crc == state_crc16((const U8 *)&rec, sizeof(rec) - sizeof(rec.crc)))
		&& ((found == 0) || (rec.seq > stateSeq)))
		{
			found = 1;
			stateSeq = rec.seq;
			stateSlot = slot;
			memcpy(stateWin, rec.data, sizeof(stateWin));
		}
	}
	
	#if DEBUG_ALL || DEBUG_EXT_EEPROM
	Print_Message("\nPower-on state record : ");
	Print_Number(found ? stateSeq : 0);
	#endif
}

/*****************************************************************************
* Function name: void eeprom_flush_state(void)
* Description	: Save the power-on state window if it changed, at most once per
*				  STATE_FLUSH_MIN_MS. Call periodically from the application.
*****************************************************************************/
void eeprom_flush_state(void)
{
	if ((stateDirty == 0) || (spiee_ready == 0))
	{
		return;
	}
	
	U32 now = xTaskGetTickCount();
	if ((U32)(now - stateLastFlushTick) < (STATE_FLUSH_MIN_MS / portTICK_RATE_MS))
	{
		return;
	}
	
	STATE_REC rec;
	U16 slot = (U16)((stateSlot + 1) % STATE_JRNL_SLOTS);
	
	rec.magic = STATE_REC_MAGIC;
	rec.seq = stateSeq + 1;
	memcpy(rec.data, stateWin, sizeof(rec.data));
	rec.crc = state_crc16((const U8 *)&rec, sizeof(rec) - sizeof(rec.crc));
	
	U8 locked = spiee_lock();
	spiee_write(STATE_JRNL_BASE + ((U32)slot * STATE_REC_SIZE), (const U8 *)&rec, sizeof(rec));
	spiee_unlock(locked);
	
	/*	Advance even if the write failed: the next save uses a fresh slot and the
		newest valid record is still found at power on. */
	stateSlot = slot;
	stateSeq = rec.seq;
	stateDirty = 0;
	stateLastFlushTick = now;
}

/*****************************************************************************
* Function name: void eeprom_pin_config(void)
* Description	: V5: initialise the SPI EEPROM and load the power-on state.
*				  The I2C EEPROM U4 is only read (MAC address): keep its WP high.
*****************************************************************************/
void eeprom_pin_config(void)
{
	pio_set_output(WRITE_PROTECT_PORT, WRITE_PROTECT_PIN, HIGH, DISABLE, ENABLE);
	
	spiee_init();
	state_journal_load();
}

/*****************************************************************************
* Function name: void eeprom_write_frame(uint16_t address, uint8_t *data, uint16_t length)
* Description	: Write data at a logical address. Bytes in the power-on state
*				  window go to RAM (saved by eeprom_flush_state()); all other
*				  bytes are written to the SPI EEPROM at the same address.
*****************************************************************************/
void eeprom_write_frame(uint16_t address, uint8_t *data, uint16_t length)
{
	U8 locked = spiee_lock();
	
	while (length > 0)
	{
		if ((address >= STATE_WIN_BASE) && (address < (STATE_WIN_BASE + STATE_WIN_SIZE)))
		{
			U16 off = address - STATE_WIN_BASE;
			U16 n = STATE_WIN_SIZE - off;
			n = (length < n) ? length : n;
			
			if (memcmp(&stateWin[off], data, n) != 0)
			{
				memcpy(&stateWin[off], data, n);
				stateDirty = 1;
			}
			
			address += n;
			data += n;
			length -= n;
		}
		else
		{
			U16 n = length;
			if ((address < STATE_WIN_BASE) && ((U32)address + length > STATE_WIN_BASE))
			{
				n = STATE_WIN_BASE - address;
			}
			
			spiee_write(address, data, n);
			
			address += n;
			data += n;
			length -= n;
		}
	}
	
	spiee_unlock(locked);
}

/*****************************************************************************
* Function name: void eeprom_read_frame(uint16_t address, uint8_t *data, uint16_t length)
* Description	: Read data from a logical address (see eeprom_write_frame()).
*****************************************************************************/
void eeprom_read_frame(uint16_t address, uint8_t *data, uint16_t length)
{
	U8 locked = spiee_lock();
	
	while (length > 0)
	{
		if ((address >= STATE_WIN_BASE) && (address < (STATE_WIN_BASE + STATE_WIN_SIZE)))
		{
			U16 off = address - STATE_WIN_BASE;
			U16 n = STATE_WIN_SIZE - off;
			n = (length < n) ? length : n;
			
			memcpy(data, &stateWin[off], n);
			
			address += n;
			data += n;
			length -= n;
		}
		else
		{
			U16 n = length;
			if ((address < STATE_WIN_BASE) && ((U32)address + length > STATE_WIN_BASE))
			{
				n = STATE_WIN_BASE - address;
			}
			
			spiee_read(address, data, n);
			
			address += n;
			data += n;
			length -= n;
		}
	}
	
	spiee_unlock(locked);
}

void eeprom_write_byte(uint16_t addr, uint8_t data)
{
	eeprom_write_frame(addr, &data, 1);
}

uint8_t eeprom_read_byte(uint16_t addr)
{
	uint8_t data = 0xFF;
	eeprom_read_frame(addr, &data, 1);
	return data;
}

/*****************************************************************************
* Function name: void erase_eeprom(void)
* Description	: Erase the configuration area and the saved power-on state.
*****************************************************************************/
void erase_eeprom(void)
{
	U8 blank[64];
	memset(blank, 0xFF, sizeof(blank));
	U8 locked = spiee_lock();
	
	for (U32 addr = 0; addr < STATE_WIN_BASE; addr += sizeof(blank))
	{
		U32 n = ((STATE_WIN_BASE - addr) < sizeof(blank)) ? (STATE_WIN_BASE - addr) : sizeof(blank);
		spiee_write(addr, blank, n);
	}
	for (U32 slot = 0; slot < STATE_JRNL_SLOTS; slot++)
	{
		spiee_write(STATE_JRNL_BASE + (slot * STATE_REC_SIZE), blank, STATE_REC_SIZE);
	}
	state_journal_load();
	spiee_unlock(locked);
}

/*****************************************************************************
* Function name: void read_cplt_eeprom(void)
* Description	: Debug: dump the configuration area and power-on state.
*****************************************************************************/
void read_cplt_eeprom(void)
{
	U8 buf[64];
	
	Print_Message("\nData read : ");
	for (U32 addr = 0; addr < (STATE_WIN_BASE + STATE_WIN_SIZE); addr += sizeof(buf))
	{
		eeprom_read_frame((uint16_t)addr, buf, sizeof(buf));
		Send_Frame_On_UART(buf, sizeof(buf));
	}
}

/*****************************************************************************
* Function name: void eeprom_load_mac_address(void)
* Description	: Read the factory EUI-48 from the AT24MAC402 (U4) and use it as
*				  the Ethernet MAC address. If it can not be read, the default
*				  address from conf_eth.h stays in use.
* Notes			: Call after configure_twi() and before init_ethernet().
*****************************************************************************/
#define AT24MAC_EUI_CHIP		(0x58)	/* Extended memory, device address 1011 A2 A1 A0 (A2..A0 = 0) */
#define AT24MAC_EUI48_ADDR		(0x9A)	/* EUI-48 at 9Ah..9Fh */

void eeprom_load_mac_address(void)
{
	U8 mac[6] = {0};
	U8 all_ff = 1;
	U8 all_00 = 1;
	
	twi_package_t packet_rx =
	{
		.addr[0] = AT24MAC_EUI48_ADDR,
		.addr_length = 1,
		.buffer = mac,
		.chip = AT24MAC_EUI_CHIP,
		.length = sizeof(mac)
	};
	
	if (twi_master_read(TWI0, &packet_rx) != TWI_SUCCESS)
	{
		#if DEBUG_ALL || DEBUG_EXT_EEPROM
		Print_Message("\nMAC address read failed, using default.");
		#endif
		return;
	}
	
	for (U8 idx = 0; idx < sizeof(mac); idx++)
	{
		all_ff &= (mac[idx] == 0xFF);
		all_00 &= (mac[idx] == 0x00);
	}
	
	if (all_ff || all_00 || (mac[0] & 0x01))
	{
		#if DEBUG_ALL || DEBUG_EXT_EEPROM
		Print_Message("\nMAC address invalid, using default.");
		#endif
		return;
	}
	
	ethernetif_set_mac_address(mac);
	
	#if DEBUG_ALL || DEBUG_EXT_EEPROM
	Print_Message("\nMAC address : ");
	for (U8 idx = 0; idx < sizeof(mac); idx++)
	{
		Print_Number(mac[idx]);
		Print_Message(" ");
	}
	#endif
}

#endif /* (BOARD_HW_VERSION == 5) */
