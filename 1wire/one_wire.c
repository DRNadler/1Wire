/**
 * @file    one_wire.c
 * @brief   General 1-Wire API using the DS2485 1-Wire master.
 */

// Dave Nadler 18-May-2023 Fixed compilation errors (true not TRUE, etc)
// Dave Nadler 18-May-2023 Deleted unused platform-specific #define's

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
#include <string.h>
#include "one_wire.h"
#include "DS2485.h"

/* **** Definitions **** */

/* **** Globals **** */
uint8_t oneWireScript[126];
uint8_t oneWireScript_length = 0;
double oneWireScript_accumulativeOneWireTime = 0;
uint8_t oneWireScript_commandsCount = 0;
uint8_t oneWireScriptResponse[126];
uint8_t oneWireScriptResponse_length = 0;

/* **** Functions **** */
int OneWire_ResetPulse()
{
	int error = 0; //if there is an error, make error == 1
	uint8_t resetResponse_index;
	one_wire_speeds speed;
	uint8_t presence_pulse_detect;
	uint8_t reset_status;

	OneWire_Script_Clear();

	OneWire_Get_OneWireMasterSpeed(&speed);
	OneWire_Script_Add_OW_RESET(&resetResponse_index, speed, false);
	OneWire_Script_Execute();
	reset_status = oneWireScriptResponse[resetResponse_index + 1];
	presence_pulse_detect = reset_status & (1 << 1);
	if (presence_pulse_detect)
	{
		error = 0;
	}
	else
	{
		error = 1;
	}

    return error;
}

int OneWire_WriteByte(unsigned char byte)
{
	int error = 0;
	uint8_t writeByteResponse_index;

	OneWire_Script_Clear();

	OneWire_Script_Add_OW_WRITE_BYTE(&writeByteResponse_index, byte);
	OneWire_Script_Execute();
	uint8_t writeByte_status = oneWireScriptResponse[writeByteResponse_index + 1];
	if (byte == writeByte_status)
	{
		error = 0;
	}
	else
	{
		error = 1;
	}

    return error;
}

int OneWire_WriteBlock(unsigned char *data, int data_length)
{
	int error = 0;
	uint8_t writeBlockResponse_index;

	OneWire_Script_Clear();

	OneWire_Script_Add_OW_WRITE_BLOCK(&writeBlockResponse_index, data, data_length);
	OneWire_Script_Execute();
	uint8_t writeBlock_status = oneWireScriptResponse[writeBlockResponse_index + 1];
	if (writeBlock_status == 0xAA)
	{
		error = 0;
	}
	else
	{
		error = 1;
	}

    return error;
}

unsigned char OneWire_ReadByte()
{
	uint8_t readByteResponse_index;

	OneWire_Script_Clear();

	OneWire_Script_Add_OW_READ_BYTE(&readByteResponse_index);
	OneWire_Script_Execute();

	uint8_t readByte = oneWireScriptResponse[readByteResponse_index + 1];
	return readByte;
}

void OneWire_ReadBlock(unsigned char *data, int data_length)
{
	uint8_t readBlockResponse_index;

	OneWire_Script_Clear();

	OneWire_Script_Add_OW_READ_BLOCK(&readBlockResponse_index, data_length);
	OneWire_Script_Execute();

	uint8_t readBlock_length = oneWireScriptResponse[readBlockResponse_index + 1];
	uint8_t readBlock_index = readBlockResponse_index + 2;
	memcpy(&data[0], &oneWireScriptResponse[readBlock_index], readBlock_length);
}

//--------------------------------------------------------------------------
/// Perform the 1-Wire Search Algorithm on the 1-Wire bus
/// parameter: search_reset: start a new search? (false: continue on to next device)
/// Return 0: no error
/// return parameter: last_device_found - True: no more devices
int OneWire_Search(OneWire_ROM_ID_T *romid, bool search_reset, bool *last_device_found)
{
	return DS2485_OneWireSearch(romid->ID, /*search command code=*/0xF0, /*reset=*/true, /*ignore=*/false, search_reset, last_device_found);
}

int OneWire_WriteBytePower(int send_byte)
{
	int error = 0;
	uint8_t writeByteResponse_index;

	OneWire_Script_Clear();

	OneWire_Script_Add_PRIME_SPU();
	OneWire_Script_Add_OW_WRITE_BYTE(&writeByteResponse_index, send_byte);
	OneWire_Script_Execute();
	uint8_t writeByte_status = oneWireScriptResponse[writeByteResponse_index + 1];
	if (send_byte == writeByte_status)
	{
		error = 0;
	}
	else
	{
		error = 1;
	}

    return error;
}


/* **** Primitive Commands Functions **** */
void OneWire_Script_Clear(void)
{
	oneWireScript_length = 0;
	oneWireScript_accumulativeOneWireTime = 0;
	oneWireScript_commandsCount = 0;
	oneWireScriptResponse_length = 0;
}
int OneWire_Script_Execute(void)
{
	int error = 0;

	if((error = DS2485_OneWireScript(oneWireScript, oneWireScript_length, oneWireScript_accumulativeOneWireTime, oneWireScript_commandsCount, oneWireScriptResponse, oneWireScriptResponse_length)) != 0)
	{
		return error;
	}

    return error;

}
int OneWire_Script_Add_OW_RESET(uint8_t *response_index, one_wire_speeds spd, bool ignore)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double ow_rst_time;
	double t_rstl;
	double t_rsth;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_RESET;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = ((spd ^ 1) << 7) | (spd << 3) | (ignore << 1);

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	// Fetch timings
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
	}
	//Calculate 1-Wire time
	ow_rst_time = t_rstl + t_rsth;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += ow_rst_time;

	return error;
}

int OneWire_Script_Add_OW_WRITE_BIT(uint8_t *response_index, bool bit_value)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_WRITE_BIT;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = bit_value;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
//	oneWireScript_commandsCount++;			// omitted

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += t_slot;

	return error;
}

int OneWire_Script_Add_OW_READ_BIT(uint8_t *response_index)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_READ_BIT;

	/***** Command parameter *****/
	//No parameter

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
//	oneWireScript_commandsCount++;			// omitted

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += t_slot;

	return error;
}

int OneWire_Script_Add_OW_WRITE_BYTE(uint8_t *response_index, uint8_t txByte)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_WRITE_BYTE;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = txByte;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time;

	return error;
}

int OneWire_Script_Add_OW_READ_BYTE(uint8_t *response_index)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_READ_BYTE;

	/***** Command parameter *****/
	//no parameter

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time;

	return error;
}

int OneWire_Script_Add_OW_TRIPLET(uint8_t *response_index, bool t_value)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_TRIPLET;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = t_value;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
//	oneWireScript_commandsCount++;			// omitted

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += t_slot;

	return error;
}

int OneWire_Script_Add_OV_SKIP(uint8_t *response_index)
{
	int error = 0;

	// Delay variables
	double standard_ow_rst_time;
	double overdrive_ow_rst_time;
	double t_slot;
	double byte_time;

	// Timings
	double standard_t_w0l;
	double standard_t_rec;
	double standard_t_rstl;
	double standard_t_rsth;
	double overdrive_t_rstl;
	double overdrive_t_rsth;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_OV_SKIP;

	/***** Command parameter *****/
	//No parameter

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount += 3; //1-Wire STD reset + Overdrive Skip Command + 1-Wire OV reset

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_tRSTH(&overdrive_t_rsth, OVERDRIVE)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tRSTL(&overdrive_t_rstl, OVERDRIVE)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tW0L(&standard_t_w0l, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tREC(&standard_t_rec, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tRSTH(&standard_t_rsth, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tRSTL(&standard_t_rstl, STANDARD)) != 0)
	{
		return error;
	}

	//Calculate 1-Wire time
	t_slot = standard_t_w0l + standard_t_rec;
	byte_time = 8 * t_slot;
	standard_ow_rst_time = standard_t_rsth + standard_t_rstl;
	overdrive_ow_rst_time = overdrive_t_rsth + overdrive_t_rstl;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += standard_ow_rst_time + byte_time + overdrive_ow_rst_time + 2000;

	return error;
}

int OneWire_Script_Add_SKIP(uint8_t *response_index)
{
	int error = 0;

	// Delay variables
	double ow_rst_time;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;
	double t_rstl;
	double t_rsth;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_SKIP;

	/***** Command parameter *****/
	//No parameter

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount += 2; //1-Wire STD reset + Skip ROM Command

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
	{
		return error;
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;
	ow_rst_time = t_rsth + t_rstl;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += ow_rst_time + byte_time;

	return error;
}

int OneWire_Script_Add_OW_READ_BLOCK(uint8_t *response_index, uint8_t rxBytes)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_READ_BLOCK;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = rxBytes;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += rxBytes + 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount += rxBytes;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time * rxBytes;

	return error;
}

int OneWire_Script_Add_OW_WRITE_BLOCK(uint8_t *response_index, const uint8_t *txData, uint8_t txData_length)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_OW_WRITE_BLOCK;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = txData_length;
	for(int i = 0; i < txData_length; i++)
	{
		oneWireScript[oneWireScript_length++] = txData[i];
	}

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount += txData_length;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time * txData_length;

	return error;
}

void OneWire_Script_Add_DELAY(uint8_t ms)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_DELAY;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = ms;

	/***** Add expected response size to total response length *****/
//	*response_index = oneWireScriptResponse_length; //ommited
//	oneWireScriptResponse_length += 2; //ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	oneWireScript_accumulativeOneWireTime += ms * 1000;
}

void OneWire_Script_Add_PRIME_SPU(void)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_PRIME_SPU;

	/***** Command parameter *****/
	//ommited

	/***** Add expected response size to total response length *****/
	//ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_SPU_OFF(void)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_SPU_OFF;

	/***** Command parameter *****/
	//ommited

	/***** Add expected response size to total response length *****/
	//ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

int OneWire_Script_Add_SPEED(one_wire_speeds spd, bool ignore)
{
	int error = 0;

	// Delay variables
	double ow_rst_time;

	// Timings
	double t_rstl;
	double t_rsth;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_SPEED;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = ((spd ^ 1) << 7) | (spd << 3) | (ignore << 1);

	/***** Add expected response size to total response length *****/
	//ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if(spd != STANDARD)
	{
		if ((error = OneWire_Get_tRSTH(&t_rsth, OVERDRIVE)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTL(&t_rstl, OVERDRIVE)) != 0)
		{
			return error;
		}
	}
	else
	{
		if ((error = OneWire_Get_tRSTH(&t_rsth, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tRSTL(&t_rstl, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	ow_rst_time = t_rstl + t_rsth;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += ow_rst_time;

	return error;
}

int OneWire_Script_Add_VERIFY_TOGGLE(uint8_t *response_index)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_VERIFY_TOGGLE;

	/***** Command parameter *****/
	//ommited

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time;

	return error;
}

int OneWire_Script_Add_VERIFY_BYTE(uint8_t *response_index, uint8_t byte)
{
	int error = 0;

	// Delay variables
	one_wire_speeds master_speed;
	double t_slot;
	double byte_time;

	// Timings
	double t_w0l;
	double t_rec;

	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_VERIFY_BYTE;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = byte;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/
	oneWireScript_commandsCount++;

	/***** Add accumulative 1-Wire time in us *****/
	/***** Fetch timings *****/
	if ((error = OneWire_Get_OneWireMasterSpeed(&master_speed)) != 0)
	{
		return error;
	}

	if(master_speed != STANDARD)
	{
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
		if ((error = OneWire_Get_tW0L(&t_w0l, STANDARD)) != 0)
		{
			return error;
		}
		if ((error = OneWire_Get_tREC(&t_rec, STANDARD)) != 0)
		{
			return error;
		}
	}

	//Calculate 1-Wire time
	t_slot = t_w0l + t_rec;
	byte_time = 8 * t_slot;

	//Add to total 1-Wire time
	oneWireScript_accumulativeOneWireTime += byte_time;

	return error;
}

void OneWire_Script_Add_CRC16_START(void)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_CRC16_START;

	/***** Command parameter *****/
	//ommited

	/***** Add expected response size to total response length *****/
	//ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_VERIFY_CRC16(uint8_t *response_index, unsigned short hex_value)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_VERIFY_CRC16;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = (uint8_t)hex_value; //LSB
	oneWireScript[oneWireScript_length++] = (uint8_t)(hex_value >> 8); //MSB

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_SET_GPIO(uint8_t *response_index, gpio_settings pioac)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_SET_GPIO;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = pioac;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_READ_GPIO(uint8_t *response_index)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_READ_GPIO;

	/***** Command parameter *****/
	//ommited

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_VERIFY_GPIO(uint8_t *response_index, gpio_verify_level_detection pioal)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_VERIFY_GPIO;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = pioal;

	/***** Add expected response size to total response length *****/
	*response_index = oneWireScriptResponse_length;
	oneWireScriptResponse_length += 2;

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

void OneWire_Script_Add_CONFIG_RPUP_BUF(unsigned short hex_value)
{
	/***** Command code *****/
	oneWireScript[oneWireScript_length++] = PC_CONFIG_RPUP_BUF;

	/***** Command parameter *****/
	oneWireScript[oneWireScript_length++] = (uint8_t)hex_value; //LSB
	oneWireScript[oneWireScript_length++] = (uint8_t)(hex_value >> 8); //MSB

	/***** Add expected response size to total response length *****/
	//ommited

	/***** Add 1-Wire actions to total command count (only applies to 1-Wire reset, and 1-Wire bytes) *****/

	/***** Add accumulative 1-Wire time in us *****/
	//ommited
}

/* **** High Level Functions **** */
int OneWire_Enable_APU(bool apu)
{
	int error = 0;
	uint8_t reg_data[2];

	if (apu)
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] |= 0x10;

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}
	}
	else
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] &= ~(0x10);

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}
	}
	return error;
}

int OneWire_Enable_SPU(bool spu)
{
	int error = 0;
	uint8_t reg_data[2];

	if (spu)
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] |= 0x20;

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

	}
	else
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] &= ~(0x20);

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}
	}
	return error;
}

int OneWire_Enable_OneWirePowerDown(bool pdn)
{
	int error = 0;
	uint8_t reg_data[2];

	if (pdn)
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] |= 0x40;

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

	}
	else
	{
		if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}

		reg_data[0] = 0x00;
		reg_data[1] &= ~(0x40);

		if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
		{
			return error;
		}
	}
	return error;
}

int OneWire_Set_OneWireMasterSpeed(one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];

	if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
	{
		return error;
	}

	reg_data[0] = 0x00;
	reg_data[1] = ((reg_data[1] & ~0x80) | (spd << 7));

	if((error = DS2485_WriteOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
	{
		return error;
	}

    return error;
}

int OneWire_Get_OneWireMasterSpeed(one_wire_speeds *spd)
{
	int error = 0;
	uint8_t reg_data[2];

	if((error = DS2485_ReadOneWirePortConfig(MASTER_CONFIGURATION, reg_data)) != 0)
	{
		return error;
	}

	if((reg_data[1] >> 7) != STANDARD)
	{
		*spd = OVERDRIVE;
	}
	else
	{
		*spd = STANDARD;
	}

    return error;
}

int OneWire_Set_Custom_RPUP_BUF(vth_values vth, viapo_values viapo, rwpu_values rwpu)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x80;
	reg_data[0] = (vth << 4) | (viapo << 2) | rwpu;

	if((error = DS2485_WriteOneWirePortConfig(RPUP_BUF, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Get_Custom_RPUP_BUF(vth_values *vth, viapo_values *viapo, rwpu_values *rwpu)
{
	int error = 0;
	uint8_t reg_data[2];

	if((error = DS2485_ReadOneWirePortConfig(RPUP_BUF, reg_data)) != 0)
	{
		return error;
	}

	switch((reg_data[0] >> 4) & 0x03)
	{
	case VTH_LOW:
		*vth = VTH_LOW;
		break;
	case VTH_MEDIUM:
		*vth = VTH_MEDIUM;
		break;
	case VTH_HIGH:
		*vth = VTH_HIGH;
		break;
	case VTH_OFF:
		*vth = VTH_OFF;
		break;
	}

	switch((reg_data[0] >> 2) & 0x03)
	{
	case VIAPO_LOW:
		*viapo = VIAPO_LOW;
		break;
	case VIAPO_MEDIUM:
		*viapo = VIAPO_MEDIUM;
		break;
	case VIAPO_HIGH:
		*viapo = VIAPO_HIGH;
		break;
	case VIAPO_OFF:
		*viapo = VIAPO_OFF;
		break;
	}

	switch(reg_data[0] & 0x03)
	{
	case RWPU_EXTERNAL:
		*rwpu = RWPU_EXTERNAL;
		break;
	case RWPU_500:
		*rwpu = RWPU_500;
		break;
	case RWPU_1000:
		*rwpu = RWPU_1000;
		break;
	case RWPU_333:
		*rwpu = RWPU_333;
		break;
	}

    return error;
}


int OneWire_Set_tRSTL_Standard_Predefined(one_wire_timing_presets trstl)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trstl;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tRSTL, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tRSTL_Overdrive_Predefined(one_wire_timing_presets trstl)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trstl;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tRSTL, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tRSTL_Standard_Custom(double trstl)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trstl > 1020)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trstl *= 1000; //us -> ns
	trstl /= (double)62.5;
	trstl = ((int)trstl | 0x8000);
	reg_data[0] = (uint8_t)trstl;
	reg_data[1] = ((int)trstl >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tRSTL, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tRSTL_Overdrive_Custom(double trstl)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trstl > 126)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trstl *= 1000; //us -> ns
	trstl /= (double)62.5;
	trstl = ((int)trstl | 0x8000);
	reg_data[0] = (uint8_t)trstl;
	reg_data[1] = ((int)trstl >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tRSTL, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tRSTL(double *trstl, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tRSTL, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trstl = (double)value;
			*trstl *= (double)62.5;
			*trstl /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trstl = tRSTL_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*trstl = tRSTL_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*trstl = tRSTL_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*trstl = tRSTL_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*trstl = tRSTL_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*trstl = tRSTL_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*trstl = tRSTL_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*trstl = tRSTL_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*trstl = tRSTL_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*trstl = tRSTL_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*trstl = tRSTL_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*trstl = tRSTL_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*trstl = tRSTL_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*trstl = tRSTL_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*trstl = tRSTL_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*trstl = tRSTL_OVERDRIVE_PRESET_F;
				break;

			default:
				*trstl = tRSTL_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tRSTL, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trstl = (double)value;
			*trstl *= (double)62.5;
			*trstl /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trstl = tRSTL_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*trstl = tRSTL_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*trstl = tRSTL_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*trstl = tRSTL_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*trstl = tRSTL_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*trstl = tRSTL_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*trstl = tRSTL_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*trstl = tRSTL_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*trstl = tRSTL_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*trstl = tRSTL_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*trstl = tRSTL_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*trstl = tRSTL_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*trstl = tRSTL_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*trstl = tRSTL_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*trstl = tRSTL_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*trstl = tRSTL_STANDARD_PRESET_F;
				break;

			default:
				*trstl = tRSTL_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}
int OneWire_Set_tRSTH_Standard_Predefined(one_wire_timing_presets trsth)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trsth;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tRSTH, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tRSTH_Overdrive_Predefined(one_wire_timing_presets trsth)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trsth;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tRSTH, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tRSTH_Standard_Custom(double trsth)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trsth > 1020)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trsth *= 1000; //us -> ns
	trsth /= (double)62.5;
	trsth = ((int)trsth | 0x8000);
	reg_data[0] = (uint8_t)trsth;
	reg_data[1] = ((int)trsth >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tRSTH, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tRSTH_Overdrive_Custom(double trsth)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trsth > 126)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trsth *= 1000; //us -> ns
	trsth /= (double)62.5;
	trsth = ((int)trsth | 0x8000);
	reg_data[0] = (uint8_t)trsth;
	reg_data[1] = ((int)trsth >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tRSTH, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tRSTH(double *trsth, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tRSTH, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trsth = (double)value;
			*trsth *= (double)62.5;
			*trsth /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trsth = tRSTH_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*trsth = tRSTH_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*trsth = tRSTH_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*trsth = tRSTH_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*trsth = tRSTH_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*trsth = tRSTH_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*trsth = tRSTH_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*trsth = tRSTH_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*trsth = tRSTH_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*trsth = tRSTH_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*trsth = tRSTH_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*trsth = tRSTH_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*trsth = tRSTH_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*trsth = tRSTH_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*trsth = tRSTH_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*trsth = tRSTH_OVERDRIVE_PRESET_F;
				break;

			default:
				*trsth = tRSTH_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tRSTH, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trsth = (double)value;
			*trsth *= (double)62.5;
			*trsth /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trsth = tRSTH_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*trsth = tRSTH_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*trsth = tRSTH_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*trsth = tRSTH_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*trsth = tRSTH_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*trsth = tRSTH_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*trsth = tRSTH_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*trsth = tRSTH_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*trsth = tRSTH_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*trsth = tRSTH_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*trsth = tRSTH_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*trsth = tRSTH_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*trsth = tRSTH_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*trsth = tRSTH_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*trsth = tRSTH_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*trsth = tRSTH_STANDARD_PRESET_F;
				break;

			default:
				*trsth = tRSTH_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}
int OneWire_Set_tW0L_Standard_Predefined(one_wire_timing_presets tw0l)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tw0l;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tW0L, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tW0L_Overdrive_Predefined(one_wire_timing_presets tw0l)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tw0l;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tW0L, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tW0L_Standard_Custom(double tw0l)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tw0l > 126)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tw0l *= 1000; //us -> ns
	tw0l /= (double)62.5;
	tw0l = ((int)tw0l | 0x8000);
	reg_data[0] = (uint8_t)tw0l;
	reg_data[1] = ((int)tw0l >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tW0L, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tW0L_Overdrive_Custom(double tw0l)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tw0l > (double)31.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tw0l *= 1000; //us -> ns
	tw0l /= (double)62.5;
	tw0l = ((int)tw0l | 0x8000);
	reg_data[0] = (uint8_t)tw0l;
	reg_data[1] = ((int)tw0l >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tW0L, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tW0L(double *tw0l, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tW0L, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tw0l = (double)value;
			*tw0l *= (double)62.5;
			*tw0l /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tw0l = tW0L_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*tw0l = tW0L_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*tw0l = tW0L_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*tw0l = tW0L_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*tw0l = tW0L_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*tw0l = tW0L_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*tw0l = tW0L_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*tw0l = tW0L_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*tw0l = tW0L_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*tw0l = tW0L_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*tw0l = tW0L_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*tw0l = tW0L_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*tw0l = tW0L_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*tw0l = tW0L_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*tw0l = tW0L_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*tw0l = tW0L_OVERDRIVE_PRESET_F;
				break;

			default:
				*tw0l = tW0L_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tW0L, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tw0l = (double)value;
			*tw0l *= (double)62.5;
			*tw0l /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tw0l = tW0L_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*tw0l = tW0L_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*tw0l = tW0L_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*tw0l = tW0L_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*tw0l = tW0L_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*tw0l = tW0L_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*tw0l = tW0L_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*tw0l = tW0L_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*tw0l = tW0L_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*tw0l = tW0L_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*tw0l = tW0L_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*tw0l = tW0L_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*tw0l = tW0L_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*tw0l = tW0L_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*tw0l = tW0L_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*tw0l = tW0L_STANDARD_PRESET_F;
				break;

			default:
				*tw0l = tW0L_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}
int OneWire_Set_tREC_Standard_Predefined(one_wire_timing_presets trec)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trec;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tREC, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tREC_Overdrive_Predefined(one_wire_timing_presets trec)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = trec;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tREC, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tREC_Standard_Custom(double trec)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trec > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trec *= 1000; //us -> ns
	trec /= (double)62.5;
	trec = ((int)trec | 0x8000);
	reg_data[0] = (uint8_t)trec;
	reg_data[1] = ((int)trec >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tREC, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tREC_Overdrive_Custom(double trec)
{
	int error = 0;
	uint8_t reg_data[2];

	if (trec > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	trec *= 1000; //us -> ns
	trec /= (double)62.5;
	trec = ((int)trec | 0x8000);
	reg_data[0] = (uint8_t)trec;
	reg_data[1] = ((int)trec >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tREC, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tREC(double *trec, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tREC, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trec = (double)value;
			*trec *= (double)62.5;
			*trec /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trec = tREC_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*trec = tREC_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*trec = tREC_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*trec = tREC_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*trec = tREC_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*trec = tREC_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*trec = tREC_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*trec = tREC_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*trec = tREC_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*trec = tREC_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*trec = tREC_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*trec = tREC_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*trec = tREC_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*trec = tREC_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*trec = tREC_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*trec = tREC_OVERDRIVE_PRESET_F;
				break;

			default:
				*trec = tREC_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tREC, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*trec = (double)value;
			*trec *= (double)62.5;
			*trec /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*trec = tREC_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*trec = tREC_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*trec = tREC_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*trec = tREC_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*trec = tREC_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*trec = tREC_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*trec = tREC_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*trec = tREC_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*trec = tREC_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*trec = tREC_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*trec = tREC_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*trec = tREC_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*trec = tREC_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*trec = tREC_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*trec = tREC_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*trec = tREC_STANDARD_PRESET_F;
				break;

			default:
				*trec = tREC_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}

int OneWire_Set_tMSI_Standard_Predefined(one_wire_timing_presets tmsi)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsi;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSI, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSI_Overdrive_Predefined(one_wire_timing_presets tmsi)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsi;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSI, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSI_Standard_Custom(double tmsi)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsi > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsi *= 1000; //us -> ns
	tmsi /= (double)62.5;
	tmsi = ((int)tmsi | 0x8000);
	reg_data[0] = (uint8_t)tmsi;
	reg_data[1] = ((int)tmsi >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSI, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tMSI_Overdrive_Custom(double tmsi)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsi > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsi *= 1000; //us -> ns
	tmsi /= (double)62.5;
	tmsi = ((int)tmsi | 0x8000);
	reg_data[0] = (uint8_t)tmsi;
	reg_data[1] = ((int)tmsi >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSI, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tMSI(double *tmsi, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tMSI, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsi = (double)value;
			*tmsi *= (double)62.5;
			*tmsi /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsi = tMSI_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*tmsi = tMSI_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*tmsi = tMSI_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*tmsi = tMSI_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*tmsi = tMSI_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*tmsi = tMSI_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*tmsi = tMSI_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*tmsi = tMSI_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*tmsi = tMSI_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*tmsi = tMSI_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*tmsi = tMSI_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*tmsi = tMSI_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*tmsi = tMSI_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*tmsi = tMSI_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*tmsi = tMSI_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*tmsi = tMSI_OVERDRIVE_PRESET_F;
				break;

			default:
				*tmsi = tMSI_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tMSI, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsi = (double)value;
			*tmsi *= (double)62.5;
			*tmsi /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsi = tMSI_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*tmsi = tMSI_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*tmsi = tMSI_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*tmsi = tMSI_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*tmsi = tMSI_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*tmsi = tMSI_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*tmsi = tMSI_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*tmsi = tMSI_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*tmsi = tMSI_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*tmsi = tMSI_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*tmsi = tMSI_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*tmsi = tMSI_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*tmsi = tMSI_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*tmsi = tMSI_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*tmsi = tMSI_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*tmsi = tMSI_STANDARD_PRESET_F;
				break;

			default:
				*tmsi = tMSI_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}

int OneWire_Set_tMSP_Standard_Predefined(one_wire_timing_presets tmsp)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsp;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSP, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSP_Overdrive_Predefined(one_wire_timing_presets tmsp)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsp;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSP, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSP_Standard_Custom(double tmsp)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsp > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsp *= 1000; //us -> ns
	tmsp /= (double)62.5;
	tmsp = ((int)tmsp | 0x8000);
	reg_data[0] = (uint8_t)tmsp;
	reg_data[1] = ((int)tmsp >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSP, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tMSP_Overdrive_Custom(double tmsp)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsp > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsp *= 1000; //us -> ns
	tmsp /= (double)62.5;
	tmsp = ((int)tmsp | 0x8000);
	reg_data[0] = (uint8_t)tmsp;
	reg_data[1] = ((int)tmsp >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSP, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tMSP(double *tmsp, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tMSP, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsp = (double)value;
			*tmsp *= (double)62.5;
			*tmsp /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsp = tMSP_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*tmsp = tMSP_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*tmsp = tMSP_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*tmsp = tMSP_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*tmsp = tMSP_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*tmsp = tMSP_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*tmsp = tMSP_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*tmsp = tMSP_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*tmsp = tMSP_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*tmsp = tMSP_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*tmsp = tMSP_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*tmsp = tMSP_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*tmsp = tMSP_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*tmsp = tMSP_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*tmsp = tMSP_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*tmsp = tMSP_OVERDRIVE_PRESET_F;
				break;

			default:
				*tmsp = tMSP_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tMSP, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsp = (double)value;
			*tmsp *= (double)62.5;
			*tmsp /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsp = tMSP_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*tmsp = tMSP_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*tmsp = tMSP_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*tmsp = tMSP_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*tmsp = tMSP_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*tmsp = tMSP_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*tmsp = tMSP_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*tmsp = tMSP_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*tmsp = tMSP_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*tmsp = tMSP_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*tmsp = tMSP_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*tmsp = tMSP_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*tmsp = tMSP_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*tmsp = tMSP_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*tmsp = tMSP_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*tmsp = tMSP_STANDARD_PRESET_F;
				break;

			default:
				*tmsp = tMSP_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}

int OneWire_Set_tW1L_Standard_Predefined(one_wire_timing_presets tw1l)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tw1l;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tW1L, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tW1L_Overdrive_Predefined(one_wire_timing_presets tw1l)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tw1l;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tW1L, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tW1L_Standard_Custom(double tw1l)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tw1l > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tw1l *= 1000; //us -> ns
	tw1l /= (double)62.5;
	tw1l = ((int)tw1l | 0x8000);
	reg_data[0] = (uint8_t)tw1l;
	reg_data[1] = ((int)tw1l >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tW1L, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tW1L_Overdrive_Custom(double tw1l)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tw1l > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tw1l *= 1000; //us -> ns
	tw1l /= (double)62.5;
	tw1l = ((int)tw1l | 0x8000);
	reg_data[0] = (uint8_t)tw1l;
	reg_data[1] = ((int)tw1l >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tW1L, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tW1L(double *tw1l, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tW1L, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tw1l = (double)value;
			*tw1l *= (double)62.5;
			*tw1l /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tw1l = tW1L_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*tw1l = tW1L_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*tw1l = tW1L_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*tw1l = tW1L_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*tw1l = tW1L_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*tw1l = tW1L_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*tw1l = tW1L_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*tw1l = tW1L_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*tw1l = tW1L_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*tw1l = tW1L_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*tw1l = tW1L_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*tw1l = tW1L_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*tw1l = tW1L_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*tw1l = tW1L_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*tw1l = tW1L_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*tw1l = tW1L_OVERDRIVE_PRESET_F;
				break;

			default:
				*tw1l = tW1L_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tW1L, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tw1l = (double)value;
			*tw1l *= (double)62.5;
			*tw1l /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tw1l = tW1L_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*tw1l = tW1L_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*tw1l = tW1L_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*tw1l = tW1L_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*tw1l = tW1L_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*tw1l = tW1L_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*tw1l = tW1L_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*tw1l = tW1L_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*tw1l = tW1L_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*tw1l = tW1L_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*tw1l = tW1L_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*tw1l = tW1L_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*tw1l = tW1L_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*tw1l = tW1L_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*tw1l = tW1L_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*tw1l = tW1L_STANDARD_PRESET_F;
				break;

			default:
				*tw1l = tW1L_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}

int OneWire_Set_tMSR_Standard_Predefined(one_wire_timing_presets tmsr)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsr;

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSR, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSR_Overdrive_Predefined(one_wire_timing_presets tmsr)
{
	int error = 0;
	uint8_t reg_data[2];

	reg_data[1] = 0x00;
	reg_data[0] = tmsr;

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSR, reg_data)) != 0)
	{
		return error;
	}

    return error;
}
int OneWire_Set_tMSR_Standard_Custom(double tmsr)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsr > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsr *= 1000; //us -> ns
	tmsr /= (double)62.5;
	tmsr = ((int)tmsr | 0x8000);
	reg_data[0] = (uint8_t)tmsr;
	reg_data[1] = ((int)tmsr >> 8);

	if((error = DS2485_WriteOneWirePortConfig(STANDARD_SPEED_tMSR, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Set_tMSR_Overdrive_Custom(double tmsr)
{
	int error = 0;
	uint8_t reg_data[2];

	if (tmsr > (double)255.5)
	{
		return error = RB_INVALID_PARAMETER;
	}

	tmsr *= 1000; //us -> ns
	tmsr /= (double)62.5;
	tmsr = ((int)tmsr | 0x8000);
	reg_data[0] = (uint8_t)tmsr;
	reg_data[1] = ((int)tmsr >> 8);

	if((error = DS2485_WriteOneWirePortConfig(OVERDRIVE_SPEED_tMSR, reg_data)) != 0)
	{
		return error;
	}

	return error;
}
int OneWire_Get_tMSR(double *tmsr, one_wire_speeds spd)
{
	int error = 0;
	uint8_t reg_data[2];
	unsigned short value;

	if(spd != STANDARD)  //Overdrive
	{
		if((error = DS2485_ReadOneWirePortConfig(OVERDRIVE_SPEED_tMSR, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //custom value
		{

			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsr = (double)value;
			*tmsr *= (double)62.5;
			*tmsr /= 1000;
		}
		else //predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsr = tMSR_OVERDRIVE_PRESET_0;
				break;

			case PRESET_1:
				*tmsr = tMSR_OVERDRIVE_PRESET_1;
				break;

			case PRESET_2:
				*tmsr = tMSR_OVERDRIVE_PRESET_2;
				break;

			case PRESET_3:
				*tmsr = tMSR_OVERDRIVE_PRESET_3;
				break;

			case PRESET_4:
				*tmsr = tMSR_OVERDRIVE_PRESET_4;
				break;

			case PRESET_5:
				*tmsr = tMSR_OVERDRIVE_PRESET_5;
				break;

			case PRESET_6:
				*tmsr = tMSR_OVERDRIVE_PRESET_6;
				break;

			case PRESET_7:
				*tmsr = tMSR_OVERDRIVE_PRESET_7;
				break;

			case PRESET_8:
				*tmsr = tMSR_OVERDRIVE_PRESET_8;
				break;

			case PRESET_9:
				*tmsr = tMSR_OVERDRIVE_PRESET_9;
				break;

			case PRESET_A:
				*tmsr = tMSR_OVERDRIVE_PRESET_A;
				break;

			case PRESET_B:
				*tmsr = tMSR_OVERDRIVE_PRESET_B;
				break;

			case PRESET_C:
				*tmsr = tMSR_OVERDRIVE_PRESET_C;
				break;

			case PRESET_D:
				*tmsr = tMSR_OVERDRIVE_PRESET_D;
				break;

			case PRESET_E:
				*tmsr = tMSR_OVERDRIVE_PRESET_E;
				break;

			case PRESET_F:
				*tmsr = tMSR_OVERDRIVE_PRESET_F;
				break;

			default:
				*tmsr = tMSR_OVERDRIVE_PRESET_6;
				break;
			}
		}
	}
	else //Standard
	{
		if((error = DS2485_ReadOneWirePortConfig(STANDARD_SPEED_tMSR, reg_data)) != 0)
		{
			return error;
		}

		if((reg_data[1] >> 7) != 0) //Custom Value
		{
			value = ((reg_data[1] & ~(0x80)) << 8) | reg_data[0];
			*tmsr = (double)value;
			*tmsr *= (double)62.5;
			*tmsr /= 1000;
		}
		else //Predefined value
		{
			switch(reg_data[0])
			{
			case PRESET_0:
				*tmsr = tMSR_STANDARD_PRESET_0;
				break;

			case PRESET_1:
				*tmsr = tMSR_STANDARD_PRESET_1;
				break;

			case PRESET_2:
				*tmsr = tMSR_STANDARD_PRESET_2;
				break;

			case PRESET_3:
				*tmsr = tMSR_STANDARD_PRESET_3;
				break;

			case PRESET_4:
				*tmsr = tMSR_STANDARD_PRESET_4;
				break;

			case PRESET_5:
				*tmsr = tMSR_STANDARD_PRESET_5;
				break;

			case PRESET_6:
				*tmsr = tMSR_STANDARD_PRESET_6;
				break;

			case PRESET_7:
				*tmsr = tMSR_STANDARD_PRESET_7;
				break;

			case PRESET_8:
				*tmsr = tMSR_STANDARD_PRESET_8;
				break;

			case PRESET_9:
				*tmsr = tMSR_STANDARD_PRESET_9;
				break;

			case PRESET_A:
				*tmsr = tMSR_STANDARD_PRESET_A;
				break;

			case PRESET_B:
				*tmsr = tMSR_STANDARD_PRESET_B;
				break;

			case PRESET_C:
				*tmsr = tMSR_STANDARD_PRESET_C;
				break;

			case PRESET_D:
				*tmsr = tMSR_STANDARD_PRESET_D;
				break;

			case PRESET_E:
				*tmsr = tMSR_STANDARD_PRESET_E;
				break;

			case PRESET_F:
				*tmsr = tMSR_STANDARD_PRESET_F;
				break;

			default:
				*tmsr = tMSR_STANDARD_PRESET_6;
				break;
			}
		}
	}

    return error;
}

int OneWire_Init(void)
{
	int error = 0;

	//Set standard speed 1-Wire timings
	if ((error = OneWire_Set_tRSTL_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSI_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSP_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tRSTH_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tW0L_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tW1L_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSR_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tREC_Standard_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	//Set overdrive speed 1-Wire timings
	if ((error = OneWire_Set_tRSTL_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSI_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSP_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tRSTH_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tW0L_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tW1L_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tMSR_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}
	if ((error = OneWire_Set_tREC_Overdrive_Predefined(PRESET_6)) != 0)
	{
		return error;
	}

	//Set 1-Wire master speed to Standard
	OneWire_Set_OneWireMasterSpeed(STANDARD);

	//Set RPUP/BUF
	OneWire_Set_Custom_RPUP_BUF(VTH_MEDIUM, VIAPO_LOW, RWPU_1000);

	//Perform a 1-Wire Reset
	OneWire_ResetPulse();

	return error;
}
