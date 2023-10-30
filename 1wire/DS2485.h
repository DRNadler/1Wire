/*!
 * @file    DS2485.h
 * @brief   General library for the DS2485, supports the higher-level one_wire.c/.h API.
 */

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

 /* Define to prevent redundant inclusion */
#ifndef DS2485_H_INCLUDED
#define DS2485_H_INCLUDED

/* **** Includes **** */
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* **** Definitions **** */
/* I2C Address */
#define DS2485_I2C_7BIT_ADDRESS	 0x40U ///< will be shifted left one bit, then or'd with RW (R=1, W=0) bit on I2C
#define DS2485_I2C_CLOCKRATE  1000000U ///< 1MHz

/* Device Function Commands */
#define DFC_WRITE_MEMORY 				0x96;
#define DFC_READ_MEMORY 				0x44;
#define DFC_READ_STATUS 				0xAA;
#define DFC_SET_I2C_ADDRESS 			0x75;
#define DFC_SET_PAGE_PROTECTION 		0xC3;
#define DFC_READ_ONE_WIRE_PORT_CONFIG 	0x52;
#define DFC_WRITE_ONE_WIRE_PORT_CONFIG  0x99;
#define DFC_MASTER_RESET 				0x62;
#define DFC_ONE_WIRE_SCRIPT				0x88;
#define DFC_ONE_WIRE_BLOCK				0xAB;
#define DFC_ONE_WIRE_READ_BLOCK			0x50;
#define DFC_ONE_WIRE_WRITE_BLOCK 		0x68;
#define DFC_ONE_WIRE_SEARCH				0x11;
#define DFC_FULL_COMMAND_SEQUENCE		0x57;
#define DFC_COMPUTE_CRC16				0xCC;

/* Result Bytes */
#define RB_SUCCESS						0      // No Failure
#define RB_ALREADY_PROTECTED			-100   // Command failed because the protection for the page has already been set
#define RB_INVALID_PARAMETER			-101   // Invalid input or parameter
#define RB_SET_ADDRESS_FAIL				-102   // Command failed because I2C address has already been set
#define RB_MASTER_RESET_FAIL			-103   // Master reset fail
#define RB_COMMS_FAIL					-104   // Communication failure
#define RB_NO_PRESENCE					-105   // 1-Wire presence pulse not detected
#define RB_NO_MATCH_WRITES				-106   // Non-matching 1-Wire writes
#define RB_NOT_DETECTED					-107   // Device not detected in search
#define RB_INCORRECT_CRC				-108   // CRC16 incorrect
#define RB_INVALID_LENGTH				-109   // Invalid length of data (0)
#define RB_LENGTH_MISMATCH				-110   // Length byte does not match actual length of data (Rx Length Byte will be 0)
#define RB_WRITE_PROTECTED				-111   // The command failed because destination page is protected (WP)
#define RB_UNKNOWN				        -112   // Unknown error

/* Operation Times */
#define tOP_USEC	40
#define tSEQ_USEC	20
#define tRM_MSEC	 50
#define tWM_MSEC	100
#define tWS_MSEC	 15

/* Memory Pages*/
typedef enum {
	PAGE_0,
	PAGE_1,
	PAGE_2,
	PAGE_3,
	PAGE_4,
	PAGE_5,
} DS2485_memory_page_T;

/* Device Status Outputs*/
typedef enum {
	PAGE_PROTECTIONS,
	MAN_ID,
	DEVICE_VERSION,
} DS2485_status_outputs_T;

/* Page Protections*/
typedef enum {
	WRITE_PROTECTION = 0x02,
	NONE_PROTECTION = 0x20,
} DS2485_page_protection_T;

/* Write 1-Wire Port Configuration Registers */
typedef enum {
	MASTER_CONFIGURATION,
	STANDARD_SPEED_tRSTL,
	STANDARD_SPEED_tMSI,
	STANDARD_SPEED_tMSP,
	STANDARD_SPEED_tRSTH,
	STANDARD_SPEED_tW0L,
	STANDARD_SPEED_tW1L,
	STANDARD_SPEED_tMSR,
	STANDARD_SPEED_tREC,
	OVERDRIVE_SPEED_tRSTL,
	OVERDRIVE_SPEED_tMSI,
	OVERDRIVE_SPEED_tMSP,
	OVERDRIVE_SPEED_tRSTH,
	OVERDRIVE_SPEED_tW0L,
	OVERDRIVE_SPEED_tW1L,
	OVERDRIVE_SPEED_tMSR,
	OVERDRIVE_SPEED_tREC,
	RPUP_BUF,
	PDSLEW,
	RESERVED,
	ALL,
} DS2485_configuration_register_address_T;

/* 1-Wire Delays*/
typedef enum {
	ms_0,
	ms_2,
	ms_4,
	ms_6,
	ms_8,
	ms_10,
	ms_12,
	ms_14,
	ms_16,
	ms_18,
	ms_20,
	ms_22,
	ms_24,
	ms_26,
	ms_28,
	ms_30,
	ms_32,
	ms_34,
	ms_36,
	ms_38,
	ms_40,
	ms_42,
	ms_44,
	ms_46,
	ms_48,
	ms_50,
	ms_52,
	ms_54,
	ms_56,
	ms_58,
	ms_60,
	ms_62,
	ms_64,
	ms_66,
	ms_68,
	ms_70,
	ms_72,
	ms_74,
	ms_76,
	ms_78,
	ms_80,
	ms_82,
	ms_84,
	ms_86,
	ms_88,
	ms_90,
	ms_92,
	ms_94,
	ms_96,
	ms_98,
	ms_100,
	ms_102,
	ms_104,
	ms_106,
	ms_108,
	ms_110,
	ms_112,
	ms_114,
	ms_116,
	ms_118,
	ms_120,
	ms_122,
	ms_124,
	ms_126,
	ms_128,
	ms_130,
	ms_132,
	ms_134,
	ms_136,
	ms_138,
	ms_140,
	ms_142,
	ms_144,
	ms_146,
	ms_148,
	ms_150,
	ms_152,
	ms_154,
	ms_156,
	ms_158,
	ms_160,
	ms_162,
	ms_164,
	ms_166,
	ms_168,
	ms_170,
	ms_172,
	ms_174,
	ms_176,
	ms_178,
	ms_180,
	ms_182,
	ms_184,
	ms_186,
	ms_188,
	ms_190,
	ms_192,
	ms_194,
	ms_196,
	ms_198,
	ms_200,
	ms_202,
	ms_204,
	ms_206,
	ms_208,
	ms_210,
	ms_212,
	ms_214,
	ms_216,
	ms_218,
	ms_220,
	ms_222,
	ms_224,
	ms_226,
	ms_228,
	ms_230,
	ms_232,
	ms_234,
	ms_236,
	ms_238,
	ms_240,
	ms_242,
	ms_244,
	ms_246,
	ms_248,
	ms_250,
	ms_252,
	ms_254,
	ms_256,
	ms_258,
	ms_260,
	ms_262,
	ms_264,
	ms_266,
	ms_268,
	ms_270,
	ms_272,
	ms_274,
	ms_276,
	ms_278,
	ms_280,
	ms_282,
	ms_284,
	ms_286,
	ms_288,
	ms_290,
	ms_292,
	ms_294,
	ms_296,
	ms_298,
	ms_300,
	ms_302,
	ms_304,
	ms_306,
	ms_308,
	ms_310,
	ms_312,
	ms_314,
	ms_316,
	ms_318,
	ms_320,
	ms_322,
	ms_324,
	ms_326,
	ms_328,
	ms_330,
	ms_332,
	ms_334,
	ms_336,
	ms_338,
	ms_340,
	ms_342,
	ms_344,
	ms_346,
	ms_348,
	ms_350,
	ms_352,
	ms_354,
	ms_356,
	ms_358,
	ms_360,
	ms_362,
	ms_364,
	ms_366,
	ms_368,
	ms_370,
	ms_372,
	ms_374,
	ms_376,
	ms_378,
	ms_380,
	ms_382,
	ms_384,
	ms_386,
	ms_388,
	ms_390,
	ms_392,
	ms_394,
	ms_396,
	ms_398,
	ms_400,
	ms_402,
	ms_404,
	ms_406,
	ms_408,
	ms_410,
	ms_412,
	ms_414,
	ms_416,
	ms_418,
	ms_420,
	ms_422,
	ms_424,
	ms_426,
	ms_428,
	ms_430,
	ms_432,
	ms_434,
	ms_436,
	ms_438,
	ms_440,
	ms_442,
	ms_444,
	ms_446,
	ms_448,
	ms_450,
	ms_452,
	ms_454,
	ms_456,
	ms_458,
	ms_460,
	ms_462,
	ms_464,
	ms_466,
	ms_468,
	ms_470,
	ms_472,
	ms_474,
	ms_476,
	ms_478,
	ms_480,
	ms_482,
	ms_484,
	ms_486,
	ms_488,
	ms_490,
	ms_492,
	ms_494,
	ms_496,
	ms_498,
	ms_500,
	ms_502,
	ms_504,
	ms_506,
	ms_508,
	ms_510,
} DS2485_full_command_sequence_delays_msecs_T;

/* Device Function Commands */
int DS2485_WriteMemory(DS2485_memory_page_T pgNumber, const uint8_t *pgData);
int DS2485_ReadMemory(DS2485_memory_page_T pgNumber, uint8_t *pgData);
int DS2485_ReadStatus(DS2485_status_outputs_T output, uint8_t *status);
int DS2485_SetI2cAddress(uint8_t newAddress);
int DS2485_SetPageProtection(DS2485_memory_page_T pgNumber, DS2485_page_protection_T protection);
int DS2485_ReadOneWirePortConfig(DS2485_configuration_register_address_T reg, uint8_t *regData);
int DS2485_WriteOneWirePortConfig(DS2485_configuration_register_address_T reg, const uint8_t *regData);
int DS2485_MasterReset(void);
int DS2485_OneWireScript(const uint8_t *script, uint8_t script_length, double accumulativeOneWireTime, uint8_t commandsCount, uint8_t *scriptResponse, uint8_t scriptResponse_length);
int DS2485_OneWireBlock(const uint8_t *blockData, int blockData_Length, uint8_t *ow_data, bool ow_reset, bool ignore, bool spu, bool pe); // blockData should include read bytes as FFh
int DS2485_OneWireWriteBlock(const uint8_t *writeData, int writeData_Length, bool ow_reset, bool ignore, bool spu);
int DS2485_OneWireReadBlock(uint8_t *readData, uint8_t bytes);
int DS2485_OneWireSearch(uint8_t *romId, uint8_t code, bool ow_reset, bool ignore, bool search_rst, bool *flag);
int DS2485_FullCommandSequence(const uint8_t *owData, int owData_Length, uint8_t *rom_id, DS2485_full_command_sequence_delays_msecs_T ow_delay_msecs, uint8_t *ow_rslt_data, uint8_t ow_rslt_len);
int DS2485_ComputeCrc16(const uint8_t *crcData, int crcData_Length, uint8_t *crc16);

/* Platform-specific I2C command interface implemented in DS2485_port_xxxx.c */
int DS2485_ExecuteCommand(const uint8_t *packet, int packetSize, int delay_uSec, uint8_t *response, int responseSize);

#ifdef __cplusplus
}
#endif

#endif /* DS2485_H_INCLUDED */
