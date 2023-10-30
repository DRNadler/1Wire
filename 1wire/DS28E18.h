// DS28E18.h - Driver for DS2818E slave one 1-Wire bus
// Cleaned up by Dave Nadler 30-October-2023

/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/

/**
 * @file    ds28e18.h
 * @brief   General library for the DS28E18.
 */

 /* Define to prevent redundant inclusion */
#ifndef _DS28E18_H_
#define _DS28E18_H_

/* **** Includes **** */
#include <stdint.h>
#include <stdbool.h>
#include "one_wire.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { // DS28E18_device_function_commands_T
	COMMAND_START = 0x66,
	WRITE_SEQUENCER = 0x11,
	READ_SEQUENCER = 0x22,
	RUN_SEQUENCER = 0x33,
	WRITE_CONFIGURATION = 0x55,
	READ_CONFIGURATION = 0x6A,
	WRITE_GPIO_CONFIGURATION = 0x83,
	READ_GPIO_CONFIGURATION = 0x7C,
	DEVICE_STATUS = 0x7A,
} DS28E18_device_function_commands_T;

typedef enum { // DS28E18_one_wire_rom_commands_T
	READ_ROM = 0x33,   ///< can only be used if there is a single slave on the bus
	MATCH_ROM = 0x55,  ///< address a specific slave by ROM ID
	SEARCH_ROM = 0xF0, ///< enumeration of all slaves on bus
	SKIP_ROM = 0xCC,   ///< don't use ROM ID (applicable only when there is only 1 slave on the bus)
	RESUME = 0xA5,     ///
	OVERDRIVE_SKIP = 0x3C,
	OVERDRIVE_MATCH = 0x69, ///< selected (matched ROM ID) slave goes into overdrive
} DS28E18_one_wire_rom_commands_T;

typedef enum { // DS28E18_sequencer_commands_T
	//I2C
	I2C_START = 0x02,
	I2C_STOP = 0x03,
	I2C_WRITE_DATA = 0xE3,
	I2C_READ_DATA = 0xD4,
	I2C_READ_DATA_W_NACK_END = 0xD3,

	// SPI
	SPI_WRITE_READ_BYTE = 0xC0,
	SPI_WRITE_READ_BIT = 0xB0,
	SPI_SS_HIGH = 0x01,
	SPI_SS_LOW = 0x80,

	// Utility
	UTILITY_DELAY = 0xDD,
	UTILITY_SENS_VDD_ON = 0xCC,
	UTILITY_SENS_VDD_OFF = 0xBB,
	UTILITY_GPIO_BUF_WRITE = 0xD1,
	UTILITY_GPIO_BUF_READ = 0x1D,
	UTILITY_GPIO_CNTL_WRITE = 0xE2,
	UTILITY_GPIO_CNTL_READ = 0x2E,
} DS28E18_sequencer_commands_T;

typedef enum { // DS28E18_result_byte_T
	POR_OCCURRED = 0x44,
	EXECUTION_ERROR = 0x55,
	INVALID_PARAMETER = 0x77,
	NACK_OCCURED = 0x88,
	SUCCESS = 0xAA,
} DS28E18_result_byte_T;

typedef enum { // DS28E18_protocol_speed_T
	KHZ_100,
	KHZ_400,
	KHZ_1000,
	KHZ_2300,
} DS28E18_protocol_speed_T;

typedef enum { // DS28E18_ignore_nack_T
	DONT_IGNORE,
	IGNORE,
} DS28E18_ignore_nack_T;

typedef enum { // DS28E18_protocol_T
	I2C,
	SPI,
} DS28E18_protocol_T;

typedef enum { // DS28E18_spi_mode_T
	MODE_0 = 0x00,
	MODE_3 = 0x03,
} DS28E18_spi_mode_T;

typedef enum { // DS28E18_target_configuration_register_T
	CONTROL = 0x0B,
	BUFFER = 0x0C,
} DS28E18_target_configuration_register_T;

/// DS28E18 delay command argument is an exponent: Delay is 2^arg in msec.
/// The actual delay time is from 1ms to 32s respectively
/// Code relies on enum defined as the exponent as below.
typedef enum { // DS28E18_utility_delay_T
	DELAY_1msec     = 0,
	DELAY_2msec     = 1,
	DELAY_4msec     = 2,
	DELAY_8msec     = 3,
	DELAY_16msec    = 4,
	DELAY_32msec    = 5,
	DELAY_64msec    = 6,
	DELAY_128msec   = 7,
	DELAY_256msec   = 8,
	DELAY_512msec   = 9,
	DELAY_1024msec  = 10,
	DELAY_2048msec  = 11,
	DELAY_4096msec  = 12,
	DELAY_8192msec  = 13,
	DELAY_16384msec = 14,
	DELAY_32768msec = 15,
} DS28E18_utility_delay_T;

typedef struct { // DS28E18_sequence_T
	uint8_t sequenceData[512];
	int sequenceIdx;
	unsigned int totalSequencerDelayTime; // milliseconds
} DS28E18_sequence_T;


/***** Globals *****/

/// DS28E18 device addressed for current operations (may be one of many on 1-Wire bus)
extern OneWire_ROM_ID_T current_DS28E18_ROM_ID;


/***** API *****/

// High Level Functions
int DS28E18_Init(void);
int DS28E18_SetOnewireSpeed(one_wire_speeds spd);
extern void DS28E18_SetRomCommand(DS28E18_one_wire_rom_commands_T rom_cmd);
extern DS28E18_one_wire_rom_commands_T DS28E18_GetRomCommand(void);

// Device Function Commands
bool DS28E18_WriteSequencer(unsigned short nineBitStartingAddress, const uint8_t *txData, int txDataSize);
bool DS28E18_ReadSequencer(unsigned short nineBitStartingAddress, uint8_t *rxData,  unsigned short readLength);
bool DS28E18_RunSequencer(unsigned short nineBitStartingAddress, unsigned short runLength);
bool DS28E18_WriteConfiguration(DS28E18_protocol_speed_T SPD, DS28E18_ignore_nack_T INACK, DS28E18_protocol_T PROT, DS28E18_spi_mode_T SPI_MODE);
bool DS28E18_ReadConfiguration(uint8_t *rxData);
bool DS28E18_WriteGpioConfiguration(DS28E18_target_configuration_register_T CFG_REG_TARGET, uint8_t GPIO_HI, uint8_t GPIO_LO);
bool DS28E18_ReadGpioConfiguration(uint8_t CFG_REG_TARGET, uint8_t *rxData);
bool DS28E18_DeviceStatus(uint8_t *rxData);

// Utilities to build and use a packet of Sequencer Commands
void DS28E18_BuildPacket_ClearSequencerPacket(void);
uint8_t *DS28E18_BuildPacket_GetSequencerPacket(void);
int DS28E18_BuildPacket_GetSequencerPacketSize(void);
void DS28E18_BuildPacket_I2C_Start(void);
void DS28E18_BuildPacket_I2C_Stop(void);
void DS28E18_BuildPacket_I2C_WriteData(const uint8_t *i2cData, uint8_t i2cDataSize);
unsigned short DS28E18_BuildPacket_I2C_ReadData(int readBytes);
unsigned short DS28E18_BuildPacket_I2C_ReadDataWithNackEnd(int readBytes);
unsigned short DS28E18_BuildPacket_SPI_WriteReadByte(const uint8_t *spiWriteData, uint8_t spiWriteDataSize, int readBytes, bool fullDuplex);
unsigned short DS28E18_BuildPacket_SPI_WriteReadBit(const uint8_t *spiWriteData, uint8_t spiWriteDataSize, int writeBits, int readBits);
void DS28E18_BuildPacket_SPI_SlaveSelectHigh(void);
void DS28E18_BuildPacket_SPI_SlaveSelectLow(void);
void DS28E18_BuildPacket_Utility_Delay(DS28E18_utility_delay_T delayTimeInMsExponent);
void DS28E18_BuildPacket_Utility_SensVddOn(void);
void DS28E18_BuildPacket_Utility_SensVddOff(void);
void DS28E18_BuildPacket_Utility_GpioBufferWrite(uint8_t GPIO_BUF);
unsigned short DS28E18_BuildPacket_Utility_GpioBufferRead(void);
void DS28E18_BuildPacket_Utility_GpioControlWrite(uint8_t GPIO_CRTL_HI, uint8_t GPIO_CRTL_LO);
unsigned short DS28E18_BuildPacket_Utility_GpioControlRead(void);
void DS28E18_BuildPacket_Append(const uint8_t* sequencerCmds, size_t length);
bool DS28E18_BuildPacket_WriteAndRun(void);


#ifdef __cplusplus
}
#endif

#endif /* _DS28E18_H_ */

