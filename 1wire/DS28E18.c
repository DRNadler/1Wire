// DS28E18.c - Driver for DS2818E slave one 1-Wire bus
// Cleaned up by Dave Nadler 30-October-2023

// ToDo 1-Wire: Isolate DELAY_MSEC to make porting easier
// ToDo 1-Wire: Use OneWire_ROM_ID_T for ROM ID in DS2485 code
// ToDo 1-Wire: Add DS28E18 status register structure definition and interpretation

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

// Enable debugging here or via command-line macro definition:
//#define DS28E18_ENABLE_PRINTF_DEBUGGING

/* **** Includes **** */
#include <stdio.h> // printf for diagnostics
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "DS28E18.h"

#ifdef USE_MAXIM_DEFINITIONS // Maxim
  #include "mxc_delay.h"
  #include "mxc_sys.h"
  #include "tmr.h"
  #define DELAY_MSEC(msec_) mxc_delay(MXC_DELAY_MSEC(msec_))
#else
  #include "FreeRTOS.h"
  #include "task.h"
  #define DELAY_MSEC(msec_) vTaskDelay(pdMS_TO_TICKS(msec_))
#endif

#ifdef DS28E18_ENABLE_PRINTF_DEBUGGING
  #define PRINTF(...) printf("DS28E18: " __VA_ARGS__)
#else
  #define PRINTF(...) {}
#endif

/* **** Globals **** */
OneWire_ROM_ID_T current_DS28E18_ROM_ID;
/*
 * For example, prototype Temperature probe's DS28E18 ID is set by DS28E18_Init:
 *  current_DS28E18_ROM_ID.ID[0]	uint8_t	0x56 (Hex)
 *  current_DS28E18_ROM_ID.ID[1]	uint8_t	0xf6 (Hex)
 *  current_DS28E18_ROM_ID.ID[2]	uint8_t	0x60 (Hex)
 *  current_DS28E18_ROM_ID.ID[3]	uint8_t	0x12 (Hex)
 *  current_DS28E18_ROM_ID.ID[4]	uint8_t	0x0 (Hex)
 *  current_DS28E18_ROM_ID.ID[5]	uint8_t	0x0 (Hex)
 *  current_DS28E18_ROM_ID.ID[6]	uint8_t	0x0 (Hex)
 *  current_DS28E18_ROM_ID.ID[7]	uint8_t	0x5c (Hex)
 */

/* **** Locals **** */
static DS28E18_one_wire_rom_commands_T current_ROM_command; // (normally MATCH_ROM, SKIP_ROM during device search)
static DS28E18_sequence_T localPacket;
// Eliminates cut-and-paste of memcpy etc:
static inline void appendToSequencerPacket(const uint8_t* sequencerCmds, int length) {
	memcpy(&localPacket.sequenceData[localPacket.sequenceIdx], sequencerCmds, length);
	localPacket.sequenceIdx += length;
};
// Append an array (macro eliminates repeated error-prone sizeof)
#define APPEND_TO_PACKET(s_) { appendToSequencerPacket(s_, sizeof(s_)); }

#define SPU_Delay_tOP_msec		1 // say what? what is this delay?


/* **** Functions **** */

/*
On DS28E18 power-up, the ROM ID value is 56000000000000B2. The uniquely programmed factory value for each DS28E18
needs to be loaded from memory. After power-up, issue a Skip ROM command followed by a Write GPIO Configuration
command. This initial command populates (for all DS28E18 on the bus) the unique device ROM ID, including family code, serialization, and CRC-16.
Ignore the command CRC-16 result and the Result byte, as both might be invalid. Next issue a successful Write GPIO
Configuration command to configure the GPIO pullup/down states so that the voltage on the GPIO ports is known.
*/
/// Initialize all DS28E18 on the 1-Wire bus.
/// @return number of DS28E18  found on the bus.
int DS28E18_Init()
{
	int devicesFound = 0;

	PRINTF("-- Populate unique ROM ID of **ALL** devices on 1-Wire line   --\n");
	PRINTF("-- .. using Write GPIO Configuration command (ignore result)  --\n");
	DS28E18_SetOnewireSpeed(STANDARD);	// Set DS2485 master 1-Wire speed, must be standard during search
	DS28E18_SetRomCommand(SKIP_ROM); // Skip trying to match a particular ROM ID
	DS28E18_WriteGpioConfiguration(CONTROL, 0xA5, 0x0F); // populate all DS28E18 ROMID etc. Write is not addressed to a specific DS28E18 (now in SKIP_ROM mode)
	DS28E18_SetRomCommand(MATCH_ROM); // for all subsequent operations...

	PRINTF("-- Search and initialize every device found on the 1-Wire line --\n");
	// Temporary ID tracks last DS28E18 found, may be clobbered if there's a DS2485 error...
	OneWire_ROM_ID_T temp_rom_id = { .ID = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
	bool last_device_found = false;
	while ( !last_device_found )
	{
		// Look for the next DS28E18
		bool startNewSearch = (devicesFound==0);
		int searchError = OneWire_Search(&temp_rom_id, startNewSearch, &last_device_found);
		if(searchError) break;
		devicesFound++;

		PRINTF("Found ROM ID: x");
		#ifdef DS28E18_ENABLE_PRINTF_DEBUGGING
			for(uint8_t i = 0; i < sizeof(temp_rom_id.ID); i++)
			{
				printf("%02X", temp_rom_id.ID[i]);
			}
			printf("\n");
		#endif

		// ToDo 1-Wire: DS28E18_Init() assumes any 1-Wire device found on the bus is a DS28E18
		current_DS28E18_ROM_ID = temp_rom_id; // Set active DS28E18 to device just found

		PRINTF("-- Write GPIO Configuration so the voltage on GPIO ports is known --\n");
		if(!DS28E18_WriteGpioConfiguration(CONTROL, 0xA5, 0x0F))
		{
			return false;
		}

		PRINTF("-- Read Device Status information (clears POR status bit) --\n");
		uint8_t status[4] = {0xFF, 0xFF, 0xFF, 0xFF};
		if(!DS28E18_DeviceStatus(status))
		{
			return false;
		}
		else
		{
			PRINTF("-- Status: ");
			#ifdef DS28E18_ENABLE_PRINTF_DEBUGGING
				for(uint8_t i = 0; i < sizeof(status); i++)
				{
					printf("%02X.", status[i]);
				}
				printf("\n");
			#endif
		}
	};

	DS28E18_BuildPacket_ClearSequencerPacket(); // general initialization (sequencer is not used during Init() above).

	return devicesFound>0;
}

//-----------------------------------------------------------------------------
/// Set desired 1-Wire speed between Standard and Overdrive for both, 1-Wire master and slave.
/// @return
///	0 - At least one device is detected after a 1-Wire reset is performed on new speed.
///	1 - Failure.
int DS28E18_SetOnewireSpeed(one_wire_speeds spd)
{
	int error = 1;
	switch (spd)
	{
		case STANDARD:
			PRINTF("STANDARD*\n");
			//Set host speed to Standard
			OneWire_Set_OneWireMasterSpeed(STANDARD);
			// do a 1-Wire reset in Standard and catch presence result
			error = OneWire_ResetPulse();
			break;
		case OVERDRIVE:
			PRINTF("OVERDRIVE*\n");
			//From Standard speed, do a 1-wire reset + Overdrive Skip ROM to set every device on the line to Overdrive
			OneWire_ResetPulse();
			OneWire_WriteByte(OVERDRIVE_SKIP);
			DELAY_MSEC(40);
			//Set host speed to Overdrive
			OneWire_Set_OneWireMasterSpeed(OVERDRIVE);
			// do a 1-Wire reset in Overdrive and catch presence result
			error = OneWire_ResetPulse();
			break;
		default:
			break;
	}
	return error;
}

/// Set 1-Wire 'current_ROM_command' (normally MATCH_ROM, SKIP_ROM during device search).
/// Does not send anything to lower-level code; just sets the operating mode for this DS28E18 layer.
void DS28E18_SetRomCommand(DS28E18_one_wire_rom_commands_T rom_cmd)
{
	assert(rom_cmd==MATCH_ROM || rom_cmd==SKIP_ROM);
	current_ROM_command = rom_cmd;
}
/// Return the value of 'current_ROM_command'
DS28E18_one_wire_rom_commands_T DS28E18_GetRomCommand()
{
	return current_ROM_command;
}

static unsigned int calculateCrc16Byte(uint8_t data, unsigned int crc)
{
  const uint8_t oddparity[] = {0, 1, 1, 0, 1, 0, 0, 1,
									 1, 0, 0, 1, 0, 1, 1, 0};

  unsigned int data16 = (data ^ crc) & 0xff;
  crc = (crc >> 8) & 0xff;

  if (oddparity[data16 & 0xf] ^ oddparity[data16 >> 4]) {
	crc ^= 0xc001;
  }

  data16 <<= 6;
  crc ^= data16;
  data16 <<= 1;
  crc ^= data16;

  return crc;
}
static unsigned int calculateCrc16Block(uint8_t *data, int dataSize, unsigned int crc)
{
  for (int i = 0; i < dataSize; i++)
  {
	crc = calculateCrc16Byte(data[i], crc);
  }
  return crc;
}

/// Run a DS28E18 command (can be run sequencer), wait for it to complete, and return results.
static int run_command(DS28E18_device_function_commands_T command, uint8_t *parameters, int parameters_size, int delay_msec, uint8_t *result_data)
{
	OneWire_ROM_ID_T ROMID;
	uint8_t tx_packet[3 + parameters_size];
	uint8_t tx_packet_CRC16[2];
	unsigned int expectedCrc = 0;
	uint8_t headerResponse[2];
	int result_data_length;
	uint8_t rx_packet_CRC16[2];

	tx_packet[0] = COMMAND_START;
	tx_packet[1] = 1 + parameters_size;
	tx_packet[2] = command;
	if (parameters_size)
	{
		memcpy(&tx_packet[3], parameters, parameters_size);
	}

	//Reset pulse + presence
	OneWire_ResetPulse();

	//Execute ROM Command currently set
	switch(current_ROM_command)
	{
		case READ_ROM:
			PRINTF("Error: Not appropriate use of Read ROM \n");
			return false;
		case MATCH_ROM:
			ROMID  = current_DS28E18_ROM_ID;;
			OneWire_WriteByte(MATCH_ROM);
			OneWire_WriteBlock(ROMID.ID, 8);
			break;
		case SEARCH_ROM:
			PRINTF("Error: Not appropriate use of Search ROM \n");
			return false;
		case SKIP_ROM:
			OneWire_WriteByte(SKIP_ROM);
			break;
		case RESUME:
			OneWire_WriteByte(RESUME);
			break;
		case OVERDRIVE_SKIP:
			OneWire_WriteByte(OVERDRIVE_SKIP);
			break;
		case OVERDRIVE_MATCH:
			ROMID  = current_DS28E18_ROM_ID;;
			OneWire_WriteByte(OVERDRIVE_MATCH);
			OneWire_WriteBlock(ROMID.ID, 8);
			break;
		default:
			PRINTF("Error: 1-Wire Communication Error\n");
			return false;
	}

	//Write command-specific 1-Wire packet, tx_packet
	OneWire_WriteBlock(tx_packet, sizeof(tx_packet));

	//Read CRC16 of the tx_packet
	OneWire_ReadBlock(tx_packet_CRC16, sizeof(tx_packet_CRC16));

	//Verify CRC16
	expectedCrc = calculateCrc16Block(tx_packet, sizeof(tx_packet), expectedCrc);
	expectedCrc ^= 0xFFFFU;
	if (expectedCrc != (unsigned int)((tx_packet_CRC16[1] << 8) | tx_packet_CRC16[0]))
	{
		PRINTF("Error: Invalid CRC16\n");
		return false;
	}

	//Send Release Byte (0xAA) then enable SPU
	OneWire_WriteBytePower(OneWire_Release_Byte_xAA); // Enables SPU (hence 'Power')

	//Command-specific delay
	DELAY_MSEC(delay_msec);

	// NO! BUG! Some applications require SPU stays on to power peripheral, specifically DS28E18: Disable SPU
    //   OneWire_Enable_SPU(false); // Bug: DS28E18 run_command disabled SPU

	// Read command-specific 1-Wire packet
	OneWire_ReadBlock(headerResponse, sizeof(headerResponse)); //Dummy Byte + Length Byte;
	result_data_length = headerResponse[1];

	if (result_data_length == 0xFF)
	{
		PRINTF("Error: 1-Wire Communication Error\n");
		return false;
	}

	//Read rest of response
	OneWire_ReadBlock(result_data, result_data_length); //Result Byte + Result Data

	//Read CRC16 of the rx_packet
	OneWire_ReadBlock(rx_packet_CRC16, sizeof(rx_packet_CRC16));

	//Verify CRC16
	expectedCrc = 0;
	expectedCrc = calculateCrc16Block(&headerResponse[1], sizeof(headerResponse) - 1, expectedCrc);
	expectedCrc = calculateCrc16Block(result_data, result_data_length, expectedCrc);
	expectedCrc ^= 0xFFFFU;
	if (expectedCrc != (unsigned int)((rx_packet_CRC16[1] << 8) | rx_packet_CRC16[0]))
	{
		PRINTF("Error: Invalid CRC16\n");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
//-------- Device Function Commands -----------------------------------------
//---------------------------------------------------------------------------

static bool returnDeviceResponseResult(DS28E18_result_byte_T r) {
	switch (r) {
	case SUCCESS:
		break;
	case INVALID_PARAMETER:
		PRINTF("Error: Invalid input or parameter\n");
		return false;
	default:
		PRINTF("Error: 1-Wire Communication Error\n");
		return false;
	}
	return true;
}

/// Device Function Command: Write Sequencer (11h) - Write command sequence into DS28E28 sequencer memory over 1Wire
///
/// @param nineBitStartingAddress Target write address
/// @param txData Array of data to be written into the sequencer memory starting from the target write address
/// @param txDataSize Number of elements found in txData array
/// @return
/// true - command successful @n
/// false - command failed
///
/// @note Use Sequencer Commands functions to help build txData array.
bool DS28E18_WriteSequencer(unsigned short nineBitStartingAddress, const uint8_t *txData, int txDataSize)
{
	uint8_t parameters[2 + txDataSize];
	uint8_t response[1];
	uint8_t addressLow = nineBitStartingAddress & 0xFF;
	uint8_t addressHigh = (nineBitStartingAddress >> 8) & 0x01;
	parameters[0] = addressLow;
	parameters[1] = addressHigh;
	memcpy(&parameters[2], &txData[0], txDataSize);

	if (!run_command(WRITE_SEQUENCER, parameters, sizeof(parameters), SPU_Delay_tOP_msec, response))
	{
		return false;
	}
	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Read Sequencer (22h)
///
/// @param nineBitStartingAddress Target read address
/// @param readLength Number of data bytes to be read from the sequencer memory starting from the target read address
/// @param[out] rxData Array of data returned from specified memory address
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_ReadSequencer(unsigned short nineBitStartingAddress, uint8_t *rxData, unsigned short readLength)
{
	uint8_t parameters[2];
	int response_length = 1 + readLength;
	uint8_t response[response_length];
	uint8_t addressLow;
	uint8_t addressHigh;

	if (readLength == 128)
	{
		readLength = 0;
	}

	addressLow = nineBitStartingAddress & 0xFF;
	addressHigh = (nineBitStartingAddress >> 8) & 0x01;

	parameters[0] = addressLow;
	parameters[1] = (readLength << 1) | addressHigh;

	if (!run_command(READ_SEQUENCER, parameters, sizeof(parameters), SPU_Delay_tOP_msec, response))
	{
		return false;
	}

	memcpy(rxData, &response[1], response_length - 1);

	// Parse result byte.
	switch (response[0]) {
	case SUCCESS:
		// Success response.
		break;

	case INVALID_PARAMETER:
		PRINTF("Error: Invalid input or parameter\n");
		return false;

	default:
		PRINTF("Error: 1-Wire Communication Error\n");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
/// Device Function Command: Run Sequencer (33h) - Command DS28E18 over 1wire to run a command sequence already placed in DS28E18 sequence memory
///
/// @param nineBitStartingAddress Target run address
/// @param runLength Number of data bytes to run from the sequencer memory starting from the target run address
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_RunSequencer(unsigned short nineBitStartingAddress, unsigned short runLength)
{
	uint8_t parameters[3];
	int response_length = 3;
	uint8_t response[response_length];
	uint8_t addressLow;
	uint8_t addressHigh;
	uint8_t sequencerLengthLow;
	uint8_t sequencerLengthHigh;
	int totalSequencerCommunicationTime = 0;
	int snackLo;
	int snackHi;
	unsigned short nackOffset;

	if (runLength == 512)
	{
		runLength = 0;
	}

	addressLow = nineBitStartingAddress & 0xFF;
	addressHigh = (nineBitStartingAddress >> 8) & 0x01;
	sequencerLengthLow = ((runLength & 0x7F) << 1);
	sequencerLengthHigh = ((runLength >> 7) & 0x03);

	parameters[0] = addressLow;
	parameters[1] = sequencerLengthLow | addressHigh;
	parameters[2] = sequencerLengthHigh;

	#if 0 // Replace stunningly INSANE looping and floating-point coding
		for (float i = 0; i < (runLength / 10); i++)  //add 1ms to Run Sequencer delay for every 10 sequencer commands
		{
			totalSequencerCommunicationTime += 1;
		}
		localPacket.totalSequencerDelayTime += localPacket.totalSequencerDelayTime * 0.05; // Add ~5% to delay option time for assurance
	#else
		totalSequencerCommunicationTime += (runLength / 10); // add 1ms to Run Sequencer delay for every 10 sequencer commands
		localPacket.totalSequencerDelayTime += localPacket.totalSequencerDelayTime / 20; // Add ~5% to delay option time for assurance
	#endif

	int run_sequencer_delay_msec = SPU_Delay_tOP_msec + localPacket.totalSequencerDelayTime + totalSequencerCommunicationTime;

	if (!run_command(RUN_SEQUENCER, parameters, sizeof(parameters), run_sequencer_delay_msec, response))
	{
		return false;
	}

	// Parse result byte.
	switch (response[0]) {

	case POR_OCCURRED:
		PRINTF("Error: POR occurred resulting in the command sequencer memory being set to zero\n");
		return false;

	case EXECUTION_ERROR:
		PRINTF("Error: Execution Error (Sequencer Command packet or packets incorrectly formed)\n");
		return false;

	case NACK_OCCURED:
		snackLo = response[1];
		snackHi = response[2];
		nackOffset = snackLo + (snackHi << 8);
		if (nackOffset == 0)
		{
			nackOffset = 512;
		}
		PRINTF("Error: RunSequencer NACK occurred at sequencer byte index: %d\n", nackOffset);
		return false;

	default:
		break;
	}

	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Write Configuration (55h)
///
/// @param SPD Desired protocol speed from macros
/// @param INACK Desired INACK configuration from macros
/// @param PROT Desired protocol from macros
/// @param SPI_MODE Desired SPI Mode from macros
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_WriteConfiguration(DS28E18_protocol_speed_T SPD, DS28E18_ignore_nack_T INACK, DS28E18_protocol_T PROT, DS28E18_spi_mode_T SPI_MODE)
{
	uint8_t parameters[1];
	uint8_t response[1];

	parameters[0] = (SPI_MODE << 4) | (PROT << 3) | (INACK << 2) | SPD;

	if (!run_command(WRITE_CONFIGURATION, parameters, sizeof(parameters), SPU_Delay_tOP_msec, response))
	{
		return false;
	}

	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Read Configuration (6Ah)
///
/// @param rxData[out] Array of 2 bytes to be updated with devices' configuration information
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_ReadConfiguration(uint8_t *rxData)
{
	uint8_t parameters[0]; //no parameters
	int response_length = 2;
	uint8_t response[response_length];

	if (!run_command(READ_CONFIGURATION, parameters, 0, SPU_Delay_tOP_msec, response))
	{
		return false;
	}

	memcpy(rxData, &response[1], response_length - 1);

	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Write GPIO Configuration (83h)
///
/// @param CFG_REG_TARGET Desired GPIO Configuration Register to write to
/// @param GPIO_HI Control/Buffer register high byte
/// @param GPIO_LO Control/Buffer register low byte
/// @return
/// true - command successful @n
/// false - command failed
///
/// @note Use GPIO Configuration functions to help build GPIO_HI/GPIO_LO parameter.
bool DS28E18_WriteGpioConfiguration(DS28E18_target_configuration_register_T CFG_REG_TARGET, uint8_t GPIO_HI, uint8_t GPIO_LO)
{
	uint8_t parameters[4];
	uint8_t response[1];

	parameters[0] = CFG_REG_TARGET; // 0Bh: GPIO control register, 0Ch: GPIO buffer register
	parameters[1] = 0x03;
	parameters[2] = GPIO_HI;
	parameters[3] = GPIO_LO;

	if (!run_command(WRITE_GPIO_CONFIGURATION, parameters, sizeof(parameters), SPU_Delay_tOP_msec, response))
	{
		return false;
	}
	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Read GPIO Configuration (7Ch)
///
/// @param CFG_REG_TARGET Desired GPIO Configuration Register from macros to read from
/// @param rxData[out] Array of 2 bytes to be updated with device current GPIO configuration for GPIO_HI and GPIO_LO
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_ReadGpioConfiguration(uint8_t CFG_REG_TARGET, uint8_t *rxData)
{
	uint8_t parameters[2];
	const int response_length = 3;
	uint8_t response[response_length];

	parameters[0] = CFG_REG_TARGET;
	parameters[1] = 0x03;

	if (!run_command(READ_GPIO_CONFIGURATION, parameters, sizeof(parameters), SPU_Delay_tOP_msec, response))
	{
		return false;
	}

	memcpy(rxData, &response[1], response_length - 1);
	return returnDeviceResponseResult(response[0]);
}

//---------------------------------------------------------------------------
/// Device Function Command: Device Status (7Ah)
///
/// @param rxData[out] Array of 4 bytes to receive DS28E18's status information
/// @return
/// true - command successful @n
/// false - command failed
bool DS28E18_DeviceStatus(uint8_t *rxData)
{
	uint8_t parameters[0]; //no parameters
	const int response_length = 5;
	uint8_t response[response_length];

	if (!run_command(DEVICE_STATUS, parameters, 0, SPU_Delay_tOP_msec, response))
	{
		return false;
	}

	memcpy(rxData, &response[1], response_length - 1);
	return returnDeviceResponseResult(response[0]);
}


//---------------------------------------------------------------------------
//------  Utilities to build and use a packet of Sequencer Commands  --------
//---------------------------------------------------------------------------

/// Reset local command sequencer packet under construction
void DS28E18_BuildPacket_ClearSequencerPacket()
{
	memset(localPacket.sequenceData, 0x00, sizeof(localPacket.sequenceData));
	localPacket.sequenceIdx = 0;
	localPacket.totalSequencerDelayTime = 0;
}
/// Get address of locally constructed command sequencer packet's data
uint8_t *DS28E18_BuildPacket_GetSequencerPacket()
{
	return localPacket.sequenceData;
}
/// Get length of locally constructed command sequencer packet
int DS28E18_BuildPacket_GetSequencerPacketSize()
{
	return localPacket.sequenceIdx;
}
/// Append commands to locally constructed command sequencer packet
void DS28E18_BuildPacket_Append(const uint8_t* sequencerCmds, size_t length)
{
	appendToSequencerPacket(sequencerCmds,length);
}
/// Write locally constructed command sequencer packet into DS28E18's
/// sequence memory over 1wire, run it, and wait long enough for completion.
/// Does NOT fetch any response; use DS28E18_ReadSequencer for that.
bool DS28E18_BuildPacket_WriteAndRun()
{
	//printf("\n\n-- Load packet sequence into DS28E18's sequence memory --");
	bool success = DS28E18_WriteSequencer(0x000, localPacket.sequenceData, localPacket.sequenceIdx);
	//printf("\n\n-- Run packet sequence --");
	if(success) success = DS28E18_RunSequencer(0x000, localPacket.sequenceIdx);
	return success;
}

/// Sequencer Command: Start (02h).
///
/// Append an I2C Start command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_I2C_Start()
{
	static const uint8_t i2c_start[1] = { I2C_START };
	APPEND_TO_PACKET(i2c_start);
}

//---------------------------------------------------------------------------
/// Sequencer Command: Stop (03h).
///
/// Append an I2C Stop command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_I2C_Stop()
{
	static const uint8_t i2c_stop[1] = { I2C_STOP };
	APPEND_TO_PACKET(i2c_stop);
}

//---------------------------------------------------------------------------
/// Sequencer Command: Write Data (E3h).
///
/// Append an I2C Write Data command to the locally constructed command sequencer packet.
///
/// @param i2cData Array with data to be transmitted over the I2C bus
/// @param i2cDataSize Number of elements found in i2cData array
void DS28E18_BuildPacket_I2C_WriteData(const uint8_t *i2cData, uint8_t i2cDataSize)
{
	uint8_t i2c_write_data[2 + i2cDataSize];
	i2c_write_data[0] = I2C_WRITE_DATA;
	i2c_write_data[1] = i2cDataSize;
	memcpy(&i2c_write_data[2], i2cData, i2cDataSize);
	APPEND_TO_PACKET(i2c_write_data);
}

//---------------------------------------------------------------------------
/// Sequencer Command: Read Data (D4h).
///
/// Append an I2C Read Data command to the locally constructed command sequencer packet.
///
/// @param readBytes Number of bytes to read from the I2C bus
/// @return
/// readArrayFFhStartingAddress - Address where I2C slave response will reside
unsigned short DS28E18_BuildPacket_I2C_ReadData(int readBytes)
{
	unsigned short readArrayFFhStartingAddress = localPacket.sequenceIdx + 2;
	uint8_t i2c_read_data[2 + readBytes];

	i2c_read_data[0] = I2C_READ_DATA;
	if (readBytes == 256)
	{
		i2c_read_data[1] = 0;
	}
	else
	{
		i2c_read_data[1] = readBytes;
	}
	memset(&i2c_read_data[2], 0xFF, readBytes); // Note: This set read bytes to 0xFF in sequencer memory prior to actual read
	APPEND_TO_PACKET(i2c_read_data);
	return readArrayFFhStartingAddress;
}

//---------------------------------------------------------------------------
/// Sequencer Command: Read Data w/NACK end (D3h).
///
/// Append an I2C Read Data w/NACK end command to the locally constructed command sequencer packet.
///
/// @param readBytes Number of bytes to read from the I2C bus
/// @return
/// readArrayFFhStartingAddress - Address where I2C slave response will reside
unsigned short DS28E18_BuildPacket_I2C_ReadDataWithNackEnd(int readBytes)
{
	unsigned short readArrayFFhStartingAddress = localPacket.sequenceIdx + 2;
	uint8_t i2c_read_data_with_nack_end[2 + readBytes];
	i2c_read_data_with_nack_end[0] = I2C_READ_DATA_W_NACK_END;
	if (readBytes == 256)
	{
		i2c_read_data_with_nack_end[1] = 0;
	}
	else
	{
		i2c_read_data_with_nack_end[1] = readBytes;
	}
	memset(&i2c_read_data_with_nack_end[2], 0xFF, readBytes); // Note: This set read bytes to 0xFF in sequencer memory prior to actual read
	APPEND_TO_PACKET(i2c_read_data_with_nack_end);
	return readArrayFFhStartingAddress;
}

//---------------------------------------------------------------------------
/// Sequencer Command: SPI Write/Read Byte (C0h).
///
/// Append an SPI Write/Read Byte command to the locally constructed command sequencer packet.
///
/// @param spiWriteData Array with data to be transmitted over the SPI bus. Data not important if only reading.
/// @param spiWriteDataSize Number of elements found in spiWriteData array. Set to 0 if only reading.
/// @param readBytes Number of bytes to read from SPI bus. Set to 0 if only writting.
/// @param fullDuplex Set 'true' when interfacing with a full duplex SPI slave. Otherwise, set 'false'
/// @return
/// readArrayFFhStartingAddress - If reading, address where SPI slave response will reside.
unsigned short DS28E18_BuildPacket_SPI_WriteReadByte(const uint8_t *spiWriteData, uint8_t spiWriteDataSize, int readBytes, bool fullDuplex)
{
	unsigned short readArrayFFhStartingAddress = 0;
	uint8_t spi_write_read_data_byte[255];
	int idx = 0;

	//command
	spi_write_read_data_byte[idx++] = SPI_WRITE_READ_BYTE;

	if (spiWriteDataSize != 0 && readBytes != 0)
	{
		//Write Length
		spi_write_read_data_byte[idx++] = spiWriteDataSize;

		//Read Length
		spi_write_read_data_byte[idx] = readBytes;
		if (!fullDuplex)
		{
			spi_write_read_data_byte[idx] += spiWriteDataSize;
		}
		idx++;

		//Write Array
		for (int i = 0; i < spiWriteDataSize; i++)
		{
			spi_write_read_data_byte[idx++] = spiWriteData[i];
		}

		//Read Array
		if (!fullDuplex)
		{
			memset(&spi_write_read_data_byte[idx], 0xFF, spiWriteDataSize);
			idx += spiWriteDataSize;
		}
		readArrayFFhStartingAddress = idx;
		memset(&spi_write_read_data_byte[idx], 0xFF, readBytes);
		idx += readBytes;
	}

	else if(spiWriteDataSize != 0 && readBytes == 0)
	{
		//Write Length
		spi_write_read_data_byte[idx++] = spiWriteDataSize;

		//Read Length
		spi_write_read_data_byte[idx++] = 0;

		//Write Array
		for (int i = 0; i < spiWriteDataSize; i++)
		{
			spi_write_read_data_byte[idx++] = spiWriteData[i];
		}

		//Read Array
		//omitted
	}

	else if(spiWriteDataSize == 0 && readBytes != 0)
	{
		//Write Length
		spi_write_read_data_byte[idx++] = 0;

		//Read Length
		spi_write_read_data_byte[idx++] = readBytes;

		//Write Array
		//omitted

		//Read Array
		readArrayFFhStartingAddress = idx;
		memset(&spi_write_read_data_byte[idx], 0xFF, readBytes);
		idx += readBytes;
	}

	else
	{
		//Write Length
		spi_write_read_data_byte[idx++] = 0;

		//Read Length
		spi_write_read_data_byte[idx++] = 0;

		//Write Array
		//omitted

		//Read Array
		//omitted
	}
	readArrayFFhStartingAddress += localPacket.sequenceIdx;
	appendToSequencerPacket(spi_write_read_data_byte, idx);
	return readArrayFFhStartingAddress;
}

//---------------------------------------------------------------------------
/// Sequencer Command: SPI Write/Read Bit (B0h).
///
/// Append an SPI Write/Read Bit command to the locally constructed command sequencer packet.
///
/// @param spiWriteData Array with data to be transmitted over the SPI bus. Data not important if only reading.
/// @param spiWriteDataSize Number of elements found in spiWriteData array. Set to 0 if only reading.
/// @param writeBits Number of bits to write to SPI bus. Set to 0 if only reading.
/// @param readBits Number of bits to read from SPI bus. Set to 0 if only writting.
/// @return
/// readArrayFFhStartingAddress - If reading, address where SPI slave response will reside.
unsigned short DS28E18_BuildPacket_SPI_WriteReadBit(const uint8_t *spiWriteData, uint8_t spiWriteDataSize, int writeBits, int readBits)
{
	uint8_t readBitsInBytes = 0;
	unsigned short readArrayFFhStartingAddress = 0;
	uint8_t spi_write_read_data_bit[255];
	int idx = 0;

	if (readBits > 0 && readBits < 9)
	{
		readBitsInBytes = 1;
	}
	else if (readBits >= 9 && readBits < 17)
	{
		readBitsInBytes = 2;
	}
	else if (readBits >= 17 && readBits < 25)
	{
		readBitsInBytes = 3;
	}
	else if (readBits >= 25 && readBits < 33)
	{
		readBitsInBytes = 4;
	}
	else if (readBits >= 33 && readBits < 41)
	{
		readBitsInBytes = 5;
	}
	else if (readBits >= 41 && readBits < 49)
	{
		readBitsInBytes = 6;
	}
	else if (readBits >= 49 && readBits < 57)
	{
		readBitsInBytes = 7;
	}
	else if (readBits >= 57 && readBits < 65)
	{
		readBitsInBytes = 8;
	}

	//command
	spi_write_read_data_bit[idx++] = SPI_WRITE_READ_BIT;

	if (writeBits != 0 && readBits != 0)
	{
		//Write Length
		spi_write_read_data_bit[idx++] = writeBits;

		//Read Length
		spi_write_read_data_bit[idx++] = readBits;

		//Write Array
		for (int i = 0; i < spiWriteDataSize; i++)
		{
			spi_write_read_data_bit[idx++] = spiWriteData[i];
		}

		//Read Array
		readArrayFFhStartingAddress = idx;
		memset(&spi_write_read_data_bit[idx], 0xFF, readBitsInBytes);
		idx += readBitsInBytes;
	}

	else if(writeBits != 0 && readBits == 0)
	{
		//Write Length
		spi_write_read_data_bit[idx++] = writeBits;

		//Read Length
		spi_write_read_data_bit[idx++] = 0;

		//Write Array
		for (int i = 0; i < spiWriteDataSize; i++)
		{
			spi_write_read_data_bit[idx++] = spiWriteData[i];
		}

		//Read Array
		//omitted
	}

	else if(writeBits == 0 && readBits != 0)
	{
		//Write Length
		spi_write_read_data_bit[idx++] = 0;

		//Read Length
		spi_write_read_data_bit[idx++] = readBits;

		//Write Array
		//omitted

		//Read Array
		readArrayFFhStartingAddress = idx;
		memset(&spi_write_read_data_bit[idx], 0xFF, readBitsInBytes);
		idx += readBitsInBytes;
	}

	else
	{
		//Write Length
		spi_write_read_data_bit[idx++] = 0;

		//Read Length
		spi_write_read_data_bit[idx++] = 0;

		//Write Array
		//omitted

		//Read Array
		//omitted
	}

	readArrayFFhStartingAddress += localPacket.sequenceIdx;
	appendToSequencerPacket(spi_write_read_data_bit,idx);

	return readArrayFFhStartingAddress;
}

//---------------------------------------------------------------------------
/// Sequencer Command: SPI SS_High (01h).
///
/// Append an SPI SS_High command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_SPI_SlaveSelectHigh()
{
	static const uint8_t spi_slave_select_high[1] = { SPI_SS_HIGH };
	APPEND_TO_PACKET(spi_slave_select_high);
}

//---------------------------------------------------------------------------
/// Sequencer Command: SPI SS_Low (80h).
///
/// Append an SPI SS_Low command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_SPI_SlaveSelectLow()
{
	static const uint8_t spi_slave_select_low[1] = { SPI_SS_LOW };
	APPEND_TO_PACKET(spi_slave_select_low);
}

//---------------------------------------------------------------------------
/// Sequencer Command: Delay (DDh).
///
/// Append a Delay command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_Utility_Delay(DS28E18_utility_delay_T delayTimeInMsExponent)
{
	uint16_t delayTimeInMs = 1U << (uint16_t)delayTimeInMsExponent;
	localPacket.totalSequencerDelayTime += delayTimeInMs;
	uint8_t utility_delay_sequence[2] = { UTILITY_DELAY, delayTimeInMsExponent };
	APPEND_TO_PACKET(utility_delay_sequence);
}

//---------------------------------------------------------------------------
/// Sequencer Command: SENS_VDD On (CCh).
///
/// Append a 'SENS_VDD On' command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_Utility_SensVddOn()
{
	static const uint8_t utility_sens_vdd_on[1] = { UTILITY_SENS_VDD_ON };
	APPEND_TO_PACKET(utility_sens_vdd_on);
}

//---------------------------------------------------------------------------
/// Sequencer Command: SENS_VDD Off (BBh).
///
/// Append a 'SENS_VDD Off' command to the locally constructed command sequencer packet.
void DS28E18_BuildPacket_Utility_SensVddOff()
{
	static const uint8_t utility_sens_vdd_off[1] = { UTILITY_SENS_VDD_OFF };
	APPEND_TO_PACKET(utility_sens_vdd_off);
}

//---------------------------------------------------------------------------
/// Sequencer Command: GPIO_BUF Write (D1h).
///
/// Append a GPIO_BUF Write command to the locally constructed command sequencer packet.
///
/// @param GPIO_BUF Buffer register high byte.
void DS28E18_BuildPacket_Utility_GpioBufferWrite(uint8_t GPIO_BUF)
{
	uint8_t utility_gpio_buff_write[2] = { UTILITY_GPIO_BUF_WRITE, GPIO_BUF };
	APPEND_TO_PACKET(utility_gpio_buff_write);
}

//---------------------------------------------------------------------------
/// Sequencer Command: GPIO_BUF Read (1Dh).
///
/// Append a GPIO_BUF Read command to the locally constructed command sequencer packet.
///
/// @return readArrayFFhStartingAddress - Starting address where configuration data will reside.
unsigned short DS28E18_BuildPacket_Utility_GpioBufferRead()
{
	unsigned short readArrayFFhStartingAddress = localPacket.sequenceIdx + 1;
	static const uint8_t utility_gpio_buff_read[2] = { UTILITY_GPIO_BUF_READ, 0xFF };
	APPEND_TO_PACKET(utility_gpio_buff_read);
	return readArrayFFhStartingAddress;
}

//---------------------------------------------------------------------------
/// Sequencer Command: GPIO_CNTL Write (E2h).
///
/// Append a GPIO_CNTL Write command to the locally constructed command sequencer packet.
///
/// @param GPIO_CRTL_HI Control register high byte.
/// @param GPIO_CRTL_LO Control register low byte.
void DS28E18_BuildPacket_Utility_GpioControlWrite(uint8_t GPIO_CRTL_HI, uint8_t GPIO_CRTL_LO)
{
	uint8_t utility_gpio_cntl_write[3] = { UTILITY_GPIO_CNTL_WRITE, GPIO_CRTL_HI, GPIO_CRTL_LO };
	APPEND_TO_PACKET(utility_gpio_cntl_write);
}

//---------------------------------------------------------------------------
/// Sequencer Command: GPIO_CNTL Read (2Eh).
///
/// Append a GPIO_CNTL Read command to the locally constructed command sequencer packet.
///
/// @return readArrayFFhStartingAddress - Starting address where configuration data will reside.
unsigned short DS28E18_BuildPacket_Utility_GpioControlRead()
{
	unsigned short readArrayFFhStartingAddress = localPacket.sequenceIdx + 1;
	static const uint8_t utility_gpio_cntl_read[3] = { UTILITY_GPIO_CNTL_READ, 0xFF, 0xFF };
	APPEND_TO_PACKET(utility_gpio_cntl_read);
	return readArrayFFhStartingAddress;
}
