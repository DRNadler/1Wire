/*!
 * @file    DS2485.c
 * @brief   General library for the DS2485, supports the higher-level one_wire.c/.h API.
 */
// Modified by DRN for platform-independence 18-May-2023


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

/* **** Includes **** */
#include <stdint.h>
#include <string.h>  // memcpy

#include "DS2485.h"

#include "one_wire.h" // one_wire_speeds...

/* **** Device Function Commands **** */
int DS2485_WriteMemory(DS2485_memory_page_T pgNumber, const uint8_t *pgData)
{
	int error = 0;

	//Command specific variables
	const int txLength = 35;
	const int delay_msec = tWM_MSEC;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_WRITE_MEMORY; 				 // Command
	packet[1] = sizeof(packet) - 2;  			 // Command length byte
	packet[2] = pgNumber; 						 // Parameter
    memcpy(&packet[3], &pgData[0], 32);          // Data

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_msec*1000, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x55:
		error = RB_WRITE_PROTECTED;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_ReadMemory(DS2485_memory_page_T pgNumber, uint8_t *pgData)
{
	int error = 0;

	//Command specific variables
	const int txLength = 3;
	const int delay_msec = tRM_MSEC;
	const int rxLength = 34;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_READ_MEMORY; 				 // Command
	packet[1] = sizeof(packet) - 2;  			 // Command length byte
	packet[2] = pgNumber; 						 // Parameter

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_msec*1000, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch page data from response
    memcpy(&pgData[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_ReadStatus(DS2485_status_outputs_T output, uint8_t *status)
{
	int error = 0;

	//Command specific variables
	const int txLength = 3;
	const int delay_msec = tRM_MSEC;
	int rxLength;

	switch (output){
	case PAGE_PROTECTIONS:
		rxLength = 8;
		break;
	default:
		rxLength = 4;
		break;
	}

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_READ_STATUS; 				 // Command
	packet[1] = sizeof(packet) - 2;  			 // Command length byte
	packet[2] = output; 						 // Parameter

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_msec*1000, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch status data from response
    memcpy(&status[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_SetI2cAddress(uint8_t newAddress)
{
	int error = 0;

	//Command specific variables
	const int txLength = 3;
	const int delay_msec = tWS_MSEC;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_SET_I2C_ADDRESS; 			 // Command
	packet[1] = sizeof(packet) - 2;  			 // Command length byte
	packet[2] = newAddress << 1; 				 // Parameter

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_msec*1000, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x55:
		error = RB_SET_ADDRESS_FAIL;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_SetPageProtection(DS2485_memory_page_T pgNumber, DS2485_page_protection_T protection)
{
	int error = 0;

	//Command specific variables
	const int txLength = 4;
	const int delay_msec = tWS_MSEC;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_SET_PAGE_PROTECTION; 	     // Command
	packet[1] = sizeof(packet) - 2;  			 		 // Command length byte
	packet[2] = pgNumber; 				         // Parameter
	packet[3] = protection; 				 	 // Parameter

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_msec*1000, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x55:
		error = RB_ALREADY_PROTECTED;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_ReadOneWirePortConfig(DS2485_configuration_register_address_T reg, uint8_t *regData)
{
	int error = 0;

	//Command specific variables
	const int txLength = 3;
	const int delay_usec = tOP_USEC;
	int rxLength;

	if (reg > 0x13)
	{
		rxLength = 42;
	}
	else
	{
		rxLength = 4;
	}

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_READ_ONE_WIRE_PORT_CONFIG; 	 // Command
	packet[1] = sizeof(packet) - 2;  			 // Command length byte
	packet[2] = reg; 						     // Parameter

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch status data from response
    memcpy(&regData[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_WriteOneWirePortConfig(DS2485_configuration_register_address_T reg, const uint8_t *regData)
{
	int error = 0;

	//Command specific variables
	const int txLength = 5;
	const int delay_usec = tOP_USEC + 1000;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_WRITE_ONE_WIRE_PORT_CONFIG; 	// Command
	packet[1] = sizeof(packet) - 2;  			 	// Command length byte
	packet[2] = reg; 				 				// Parameter
	packet[3] = regData[0]; 				 		// Data
	packet[4] = regData[1]; 				 		// Data


    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_MasterReset(void)
{
	int error = 0;

	//Command specific variables
	const int txLength = 1;
	const int delay_usec = tOP_USEC;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_MASTER_RESET; 			 // Command

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x22:
		error = RB_MASTER_RESET_FAIL;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_OneWireScript(const uint8_t *script, uint8_t script_length, double accumulativeOneWireTime, uint8_t commandsCount, uint8_t *scriptResponse, uint8_t scriptResponse_length)
{
	int error = 0;

	//Command specific variables
	const int txLength = script_length + 2;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(commandsCount)) + accumulativeOneWireTime + 1000;
	const int rxLength = scriptResponse_length + 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_ONE_WIRE_SCRIPT; 			 					// Command
	packet[1] = sizeof(packet) - 2;  			 					// Command length byte
	memcpy(&packet[2], &script[0], script_length);        			// Primitive commands + data + parameters = script

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch page CRC16 from response
	memcpy(&scriptResponse[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	case 0x22:
		error = RB_COMMS_FAIL;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_OneWireBlock(const uint8_t *blockData, int blockData_Length, uint8_t *ow_data, bool ow_reset, bool ignore, bool spu, bool pe)
{
	int error = 0;
	one_wire_speeds master_speed;

	// Delay variables
	double one_wire_time;
	double ow_rst_time;
	double t_slot;

	// Timings
	double t_rstl;
	double t_rsth;
	double t_w0l;
	double t_rec;

	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//'1-Wire time'
	t_slot = t_w0l + t_rec;					//Time it takes to complete a 1-Wire Write/Read bit time slot
	ow_rst_time = t_rstl + t_rsth;			//Time it takes to complete a 1-Wire Reset slot
	one_wire_time = ((t_slot * 8) * (blockData_Length));
	if(ow_reset)
	{
		one_wire_time += ow_rst_time;
	}

	//Command specific variables
	const int txLength = blockData_Length + 3;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(blockData_Length + ow_reset)) + one_wire_time;
	const int rxLength = blockData_Length + 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_ONE_WIRE_BLOCK; 			 							// Command
	packet[1] = sizeof(packet) - 2; 			 							// Command length byte
	packet[2] = (pe << 3) | (spu << 2) | (ignore << 1) | (ow_reset << 0);   // Parameter byte
	memcpy(&packet[3], &blockData[0], blockData_Length);        			// Data

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch page CRC16 from response
	memcpy(&ow_data[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	case 0x22:
		error = RB_COMMS_FAIL;
		break;

	case 0x33:
		error = RB_NOT_DETECTED;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_OneWireWriteBlock(const uint8_t *writeData, int writeData_Length, bool ow_reset, bool ignore, bool spu)
{
	int error = 0;
	one_wire_speeds master_speed;

	// Delay variables
	double one_wire_time;
	double ow_rst_time;
	double t_slot;

	// Timings
	double t_rstl;
	double t_rsth;
	double t_w0l;
	double t_rec;

	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//'1-Wire time'
	t_slot = t_w0l + t_rec;					//Time it takes to complete a 1-Wire Write/Read bit time slot
	ow_rst_time = t_rstl + t_rsth; 			//Time it takes to complete a 1-Wire Reset slot
	one_wire_time = ((t_slot * 8) * (writeData_Length));
	if(ow_reset)
	{
		one_wire_time += ow_rst_time;
	}

	//Command specific variables
	const int txLength = writeData_Length + 3;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(writeData_Length + ow_reset)) + one_wire_time;
	const int rxLength = 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_ONE_WIRE_WRITE_BLOCK; 			 		 	// Command
	packet[1] = sizeof(packet) - 2;		 					 	// Command length byte
	packet[2] = (spu << 2) | (ignore << 1) | (ow_reset << 0);   // Parameter byte
	memcpy(&packet[3], &writeData[0], writeData_Length);     	// Data

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x22:
		error = RB_COMMS_FAIL;
		break;

	case 0x33:
		error = RB_NO_PRESENCE;
		break;

	case 0x00:
		error = RB_NO_MATCH_WRITES;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_OneWireReadBlock(uint8_t *readData, uint8_t bytes)
{
	int error = 0;
	one_wire_speeds master_speed;

	// Delay variables
	double one_wire_time;
	double t_slot;

	// Timings
	double t_rstl;
	double t_rsth;
	double t_w0l;
	double t_rec;

	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//'1-Wire time'
	t_slot = t_w0l + t_rec;					//Time it takes to complete a 1-Wire Write/Read bit time slot
	one_wire_time = ((t_slot * 8) * (bytes));


	//Command specific variables
	const int txLength = 3;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(bytes)) + one_wire_time;
	const int rxLength = bytes + 2;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_ONE_WIRE_READ_BLOCK; 			 		// Command
	packet[1] = sizeof(packet) - 2; 			            // Command length byte
	packet[2] = bytes;  			 				        // Parameter Byte

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch read data from response
	memcpy(&readData[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x22:
		error = RB_COMMS_FAIL;
		break;

	case 0x77:
		error = RB_INVALID_LENGTH;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_OneWireSearch(uint8_t *romId, uint8_t code, bool ow_reset, bool ignore, bool search_rst, bool *flag)
{
	int error = 0;
	one_wire_speeds master_speed;

	// Delay variables
	double one_wire_time;
	double ow_rst_time;
	double t_slot;

	// Timings
	double t_rstl;
	double t_rsth;
	double t_w0l;
	double t_rec;

	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//'1-Wire time'
	t_slot = t_w0l + t_rec;					//Time it takes to complete a 1-Wire Write/Read bit time slot
	ow_rst_time = t_rstl + t_rsth;			//Time it takes to complete a 1-Wire Reset slot
	one_wire_time = ((t_slot * 8) * (64));
	if(ow_reset)
	{
		one_wire_time += ow_rst_time;
	}

	//Command specific variables
	const int txLength = 4;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(64 + ow_reset)) + one_wire_time;
	const int rxLength = 11;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	// Search command (from datasheet):
	// Table 60. 1-Wire Search Parameter Byte
	// Bit 0: 1-Wire Reset (OW_RST).
	//   (1b) transmit a 1-Wire reset before the block and verify presence pulse—if presence is not detected, and IGNORE = 0b then stop operation;
	//   (0b) no 1-Wire reset.
	// Bit 1: Ignore Presence Pulse (IGNORE).
	//    (1b) ignore the presence pulse result from OW_RST
	//    (0b) do not ignore presence on optional 1-Wire reset.
	// Bit 2: Search Reset (SEARCH_RST).
	//    (1b) reset the search state to find the "first" device ROMID
	//    (0b) do not reset the search state, find the "next" device ROMID.

	//Build command packet
	packet[0] = DFC_ONE_WIRE_SEARCH; 			 		 				// Command
	packet[1] = sizeof(packet) - 2;  			 						// Command length byte
	packet[2] = (search_rst << 2) | (ignore << 1) | (ow_reset << 0);   	// Parameter byte
	packet[3] = code;													// Search command code

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch ROM ID + last device flag from response
	memcpy(&romId[0], &response[2], 8);
	*flag = response[10];

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x00:
		error = RB_NOT_DETECTED;
		break;

	case 0x33:
		error = RB_NO_PRESENCE;
		break;

	case 0x77:
		error = RB_INVALID_PARAMETER;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_FullCommandSequence(const uint8_t *owData, int owData_Length, uint8_t *rom_id, DS2485_full_command_sequence_delays_msecs_T ow_delay_msec, uint8_t *ow_rslt_data, uint8_t ow_rslt_len)
{
	int error = 0;
	one_wire_speeds master_speed;

	// Delay variables
	double one_wire_time;
	double ow_rst_time;
	double t_slot;

	// Timings
	double t_rstl;
	double t_rsth;
	double t_w0l;
	double t_rec;

	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//'1-Wire time'
	t_slot = t_w0l + t_rec;					//Time it takes to complete a 1-Wire Write/Read bit time slot
	ow_rst_time = t_rstl + t_rsth;          //Time it takes to complete a 1-Wire Reset slot
	one_wire_time = ow_rst_time + ((t_slot * 8) * (18 + owData_Length + ow_rslt_len));

	//Command specific variables
	const int txLength = owData_Length + 11;
	const int delay_usec = tOP_USEC + (tSEQ_USEC*(19 + owData_Length + ow_rslt_len)) + one_wire_time + (ow_delay_msec * 2000);
	const int rxLength = ow_rslt_len + 3;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_FULL_COMMAND_SEQUENCE; 			 		 			// Command
	packet[1] = sizeof(packet) - 2;  			 						// Command length byte
	packet[2] = ow_delay_msec;   											// Parameter byte
	memcpy(&packet[3], &rom_id[0], 8);									// ROM ID for Match ROM
	memcpy(&packet[11], &owData[0], owData_Length);						// 1-Wire Data

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch OW_RSLT_DATA from response
	memcpy(&ow_rslt_data[0], &response[3], sizeof(response) - 3);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x00:
		error = RB_INCORRECT_CRC;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}

int DS2485_ComputeCrc16(const uint8_t *crcData, int crcData_Length, uint8_t *crc16)
{
	int error = 0;

	//Command specific variables
	const int txLength = crcData_Length + 2;
	const int delay_usec = tOP_USEC;
	const int rxLength = 4;

	uint8_t packet[txLength];
	uint8_t response[rxLength];

	//Build command packet
	packet[0] = DFC_COMPUTE_CRC16; 			 				// Command
	packet[1] = sizeof(packet) - 2;  			 			// Command length byte
	memcpy(&packet[2], &crcData[0], crcData_Length);        // Data

    //Execute Command
	if ((error = DS2485_ExecuteCommand(packet, sizeof(packet), delay_usec, response, sizeof(response))) != 0)
	{
		return error;
	}

	//Fetch CRC16 from response
	memcpy(&crc16[0], &response[2], sizeof(response) - 2);

	switch (response[1]) {
	case 0xAA:
		error = RB_SUCCESS;
		break;

	case 0x77:
		error = RB_INVALID_LENGTH;
		break;

	case 0xFF:
		error = RB_LENGTH_MISMATCH;
		break;

	default:
		error = RB_UNKNOWN;
		break;
	}

    return error;
}
