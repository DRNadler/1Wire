/*!
 * @file    one_wire.h
 * @brief   General 1-Wire API using the DS2485 1-Wire master.
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
#ifndef ONE_WIRE_H_INCLUDED
#define ONE_WIRE_H_INCLUDED

/* **** Includes **** */
#include <stdint.h>
#include <stdbool.h>
#include "one_wire_address.h" // OneWire_ROM_ID_T type stores a 1-Wire address

#ifdef __cplusplus
  extern "C" {
#endif

#define OneWire_Release_Byte_xAA	0xAA

/* **** Definitions **** */

/* Primitive Commands for use in DS2485 Scripts*/
#define PC_OW_RESET						0x00
#define PC_OW_WRITE_BIT	 				0x01
#define PC_OW_READ_BIT					0x02
#define PC_OW_WRITE_BYTE				0x03
#define PC_OW_READ_BYTE					0x04
#define PC_OW_TRIPLET					0x05
#define PC_OW_OV_SKIP					0x06
#define PC_OW_SKIP						0x07
#define PC_OW_READ_BLOCK				0x08
#define PC_OW_WRITE_BLOCK				0x09
#define PC_DELAY						0x0A
#define PC_PRIME_SPU					0x0B
#define PC_SPU_OFF						0x0C
#define PC_SPEED						0x0D
#define PC_VERIFY_TOGGLE				0x0E
#define PC_VERIFY_BYTE					0x0F
#define PC_CRC16_START					0x10
#define PC_VERIFY_CRC16					0x11
#define PC_SET_GPIO						0x12
#define PC_READ_GPIO					0x13
#define PC_VERIFY_GPIO					0x14
#define PC_CONFIG_RPUP_BUF				0x15

/* 1-Wire Preset Timings [us] */
//tRSTL - Standard Speed
#define tRSTL_STANDARD_PRESET_0			440
#define tRSTL_STANDARD_PRESET_1			460
#define tRSTL_STANDARD_PRESET_2			480
#define tRSTL_STANDARD_PRESET_3			500
#define tRSTL_STANDARD_PRESET_4			520
#define tRSTL_STANDARD_PRESET_5			540
#define tRSTL_STANDARD_PRESET_6			560
#define tRSTL_STANDARD_PRESET_7			580
#define tRSTL_STANDARD_PRESET_8			600
#define tRSTL_STANDARD_PRESET_9			620
#define tRSTL_STANDARD_PRESET_A			640
#define tRSTL_STANDARD_PRESET_B			660
#define tRSTL_STANDARD_PRESET_C			680
#define tRSTL_STANDARD_PRESET_D			720
#define tRSTL_STANDARD_PRESET_E			800
#define tRSTL_STANDARD_PRESET_F			960

//tRSTL - Overdrive Speed
#define tRSTL_OVERDRIVE_PRESET_0		44
#define tRSTL_OVERDRIVE_PRESET_1		46
#define tRSTL_OVERDRIVE_PRESET_2		48
#define tRSTL_OVERDRIVE_PRESET_3		50
#define tRSTL_OVERDRIVE_PRESET_4		52
#define tRSTL_OVERDRIVE_PRESET_5		54
#define tRSTL_OVERDRIVE_PRESET_6		56
#define tRSTL_OVERDRIVE_PRESET_7		58
#define tRSTL_OVERDRIVE_PRESET_8		60
#define tRSTL_OVERDRIVE_PRESET_9		62
#define tRSTL_OVERDRIVE_PRESET_A		64
#define tRSTL_OVERDRIVE_PRESET_B		66
#define tRSTL_OVERDRIVE_PRESET_C		68
#define tRSTL_OVERDRIVE_PRESET_D		72
#define tRSTL_OVERDRIVE_PRESET_E		74
#define tRSTL_OVERDRIVE_PRESET_F		80

//tRSTH - Standard Speed
#define tRSTH_STANDARD_PRESET_0			440
#define tRSTH_STANDARD_PRESET_1			460
#define tRSTH_STANDARD_PRESET_2			480
#define tRSTH_STANDARD_PRESET_3			500
#define tRSTH_STANDARD_PRESET_4			520
#define tRSTH_STANDARD_PRESET_5			540
#define tRSTH_STANDARD_PRESET_6			560
#define tRSTH_STANDARD_PRESET_7			580
#define tRSTH_STANDARD_PRESET_8			600
#define tRSTH_STANDARD_PRESET_9			620
#define tRSTH_STANDARD_PRESET_A			640
#define tRSTH_STANDARD_PRESET_B			660
#define tRSTH_STANDARD_PRESET_C			680
#define tRSTH_STANDARD_PRESET_D			700
#define tRSTH_STANDARD_PRESET_E			720
#define tRSTH_STANDARD_PRESET_F			740

//tRSTH - Overdrive Speed
#define tRSTH_OVERDRIVE_PRESET_0		44
#define tRSTH_OVERDRIVE_PRESET_1		46
#define tRSTH_OVERDRIVE_PRESET_2		48
#define tRSTH_OVERDRIVE_PRESET_3		50
#define tRSTH_OVERDRIVE_PRESET_4		52
#define tRSTH_OVERDRIVE_PRESET_5		54
#define tRSTH_OVERDRIVE_PRESET_6		56
#define tRSTH_OVERDRIVE_PRESET_7		58
#define tRSTH_OVERDRIVE_PRESET_8		60
#define tRSTH_OVERDRIVE_PRESET_9		62
#define tRSTH_OVERDRIVE_PRESET_A		64
#define tRSTH_OVERDRIVE_PRESET_B		66
#define tRSTH_OVERDRIVE_PRESET_C		68
#define tRSTH_OVERDRIVE_PRESET_D		70
#define tRSTH_OVERDRIVE_PRESET_E		72
#define tRSTH_OVERDRIVE_PRESET_F		74

//tW0L - Standard Speed
#define tW0L_STANDARD_PRESET_0			52
#define tW0L_STANDARD_PRESET_1			56
#define tW0L_STANDARD_PRESET_2			60
#define tW0L_STANDARD_PRESET_3			62
#define tW0L_STANDARD_PRESET_4			64
#define tW0L_STANDARD_PRESET_5			66
#define tW0L_STANDARD_PRESET_6			68
#define tW0L_STANDARD_PRESET_7			70
#define tW0L_STANDARD_PRESET_8			72
#define tW0L_STANDARD_PRESET_9			74
#define tW0L_STANDARD_PRESET_A			76
#define tW0L_STANDARD_PRESET_B			80
#define tW0L_STANDARD_PRESET_C			90
#define tW0L_STANDARD_PRESET_D			100
#define tW0L_STANDARD_PRESET_E			110
#define tW0L_STANDARD_PRESET_F			120

//tW0L - Overdrive Speed
#define tW0L_OVERDRIVE_PRESET_0			5
#define tW0L_OVERDRIVE_PRESET_1			5.5
#define tW0L_OVERDRIVE_PRESET_2			6
#define tW0L_OVERDRIVE_PRESET_3			6.5
#define tW0L_OVERDRIVE_PRESET_4			7
#define tW0L_OVERDRIVE_PRESET_5			7.5
#define tW0L_OVERDRIVE_PRESET_6			8
#define tW0L_OVERDRIVE_PRESET_7			8.5
#define tW0L_OVERDRIVE_PRESET_8			9
#define tW0L_OVERDRIVE_PRESET_9			9.5
#define tW0L_OVERDRIVE_PRESET_A			10
#define tW0L_OVERDRIVE_PRESET_B			11
#define tW0L_OVERDRIVE_PRESET_C			12
#define tW0L_OVERDRIVE_PRESET_D			13
#define tW0L_OVERDRIVE_PRESET_E			14
#define tW0L_OVERDRIVE_PRESET_F			15.5

//tREC - Standard Speed
#define tREC_STANDARD_PRESET_0			0.5
#define tREC_STANDARD_PRESET_1			1.5
#define tREC_STANDARD_PRESET_2			2
#define tREC_STANDARD_PRESET_3			3
#define tREC_STANDARD_PRESET_4			4
#define tREC_STANDARD_PRESET_5			5
#define tREC_STANDARD_PRESET_6			6
#define tREC_STANDARD_PRESET_7			7.5
#define tREC_STANDARD_PRESET_8			12
#define tREC_STANDARD_PRESET_9			17.5
#define tREC_STANDARD_PRESET_A			28.5
#define tREC_STANDARD_PRESET_B			34
#define tREC_STANDARD_PRESET_C			45
#define tREC_STANDARD_PRESET_D			56.5
#define tREC_STANDARD_PRESET_E			112
#define tREC_STANDARD_PRESET_F			223

//tREC - Overdrive Speed
#define tREC_OVERDRIVE_PRESET_0			0.5
#define tREC_OVERDRIVE_PRESET_1			1.5
#define tREC_OVERDRIVE_PRESET_2			2
#define tREC_OVERDRIVE_PRESET_3			3
#define tREC_OVERDRIVE_PRESET_4			4
#define tREC_OVERDRIVE_PRESET_5			5
#define tREC_OVERDRIVE_PRESET_6			6
#define tREC_OVERDRIVE_PRESET_7			7.5
#define tREC_OVERDRIVE_PRESET_8			12
#define tREC_OVERDRIVE_PRESET_9			17.5
#define tREC_OVERDRIVE_PRESET_A			28.5
#define tREC_OVERDRIVE_PRESET_B			34
#define tREC_OVERDRIVE_PRESET_C			45
#define tREC_OVERDRIVE_PRESET_D			56.5
#define tREC_OVERDRIVE_PRESET_E			112
#define tREC_OVERDRIVE_PRESET_F			223

//tMSI - Standard Speed
#define tMSI_STANDARD_PRESET_0			3
#define tMSI_STANDARD_PRESET_1		    3
#define tMSI_STANDARD_PRESET_2			3
#define tMSI_STANDARD_PRESET_3			5
#define tMSI_STANDARD_PRESET_4			6
#define tMSI_STANDARD_PRESET_5			7
#define tMSI_STANDARD_PRESET_6			7.5
#define tMSI_STANDARD_PRESET_7			8
#define tMSI_STANDARD_PRESET_8			8.5
#define tMSI_STANDARD_PRESET_9			9
#define tMSI_STANDARD_PRESET_A			10
#define tMSI_STANDARD_PRESET_B			11
#define tMSI_STANDARD_PRESET_C			12
#define tMSI_STANDARD_PRESET_D			13
#define tMSI_STANDARD_PRESET_E			14
#define tMSI_STANDARD_PRESET_F			15

//tMSI - Overdrive Speed
#define tMSI_OVERDRIVE_PRESET_0			0.75
#define tMSI_OVERDRIVE_PRESET_1			0.75
#define tMSI_OVERDRIVE_PRESET_2			0.75
#define tMSI_OVERDRIVE_PRESET_3			0.75
#define tMSI_OVERDRIVE_PRESET_4			1
#define tMSI_OVERDRIVE_PRESET_5			1.25
#define tMSI_OVERDRIVE_PRESET_6			1.5
#define tMSI_OVERDRIVE_PRESET_7			1.625
#define tMSI_OVERDRIVE_PRESET_8			1.75
#define tMSI_OVERDRIVE_PRESET_9			1.875
#define tMSI_OVERDRIVE_PRESET_A			2
#define tMSI_OVERDRIVE_PRESET_B			2.125
#define tMSI_OVERDRIVE_PRESET_C			2.25
#define tMSI_OVERDRIVE_PRESET_D			2.375
#define tMSI_OVERDRIVE_PRESET_E			2.5
#define tMSI_OVERDRIVE_PRESET_F			2.625

//tMSP - Standard Speed
#define tMSP_STANDARD_PRESET_0			58
#define tMSP_STANDARD_PRESET_1		    60
#define tMSP_STANDARD_PRESET_2			62
#define tMSP_STANDARD_PRESET_3			64
#define tMSP_STANDARD_PRESET_4			66
#define tMSP_STANDARD_PRESET_5			67
#define tMSP_STANDARD_PRESET_6			68
#define tMSP_STANDARD_PRESET_7			69
#define tMSP_STANDARD_PRESET_8			70
#define tMSP_STANDARD_PRESET_9			71
#define tMSP_STANDARD_PRESET_A			72
#define tMSP_STANDARD_PRESET_B			74
#define tMSP_STANDARD_PRESET_C			76
#define tMSP_STANDARD_PRESET_D			78
#define tMSP_STANDARD_PRESET_E			80
#define tMSP_STANDARD_PRESET_F			82

//tMSP - Overdrive Speed
#define tMSP_OVERDRIVE_PRESET_0			5
#define tMSP_OVERDRIVE_PRESET_1			5.5
#define tMSP_OVERDRIVE_PRESET_2			6
#define tMSP_OVERDRIVE_PRESET_3			6.5
#define tMSP_OVERDRIVE_PRESET_4			7
#define tMSP_OVERDRIVE_PRESET_5			7.5
#define tMSP_OVERDRIVE_PRESET_6			8
#define tMSP_OVERDRIVE_PRESET_7			8.5
#define tMSP_OVERDRIVE_PRESET_8			9
#define tMSP_OVERDRIVE_PRESET_9			9.5
#define tMSP_OVERDRIVE_PRESET_A			10
#define tMSP_OVERDRIVE_PRESET_B			10.5
#define tMSP_OVERDRIVE_PRESET_C			11
#define tMSP_OVERDRIVE_PRESET_D		    12
#define tMSP_OVERDRIVE_PRESET_E			13
#define tMSP_OVERDRIVE_PRESET_F			14

//tW1L - Standard Speed
#define tW1L_STANDARD_PRESET_0			1
#define tW1L_STANDARD_PRESET_1		    3
#define tW1L_STANDARD_PRESET_2			5
#define tW1L_STANDARD_PRESET_3			6.5
#define tW1L_STANDARD_PRESET_4			7
#define tW1L_STANDARD_PRESET_5			7.5
#define tW1L_STANDARD_PRESET_6			8
#define tW1L_STANDARD_PRESET_7			9
#define tW1L_STANDARD_PRESET_8			10
#define tW1L_STANDARD_PRESET_9			11
#define tW1L_STANDARD_PRESET_A			12
#define tW1L_STANDARD_PRESET_B			13
#define tW1L_STANDARD_PRESET_C			14
#define tW1L_STANDARD_PRESET_D			15
#define tW1L_STANDARD_PRESET_E			15.5
#define tW1L_STANDARD_PRESET_F			16

//tW1L - Overdrive Speed
#define tW1L_OVERDRIVE_PRESET_0			0.0625
#define tW1L_OVERDRIVE_PRESET_1			0.125
#define tW1L_OVERDRIVE_PRESET_2			0.25
#define tW1L_OVERDRIVE_PRESET_3			0.375
#define tW1L_OVERDRIVE_PRESET_4			0.5
#define tW1L_OVERDRIVE_PRESET_5			0.625
#define tW1L_OVERDRIVE_PRESET_6			0.75
#define tW1L_OVERDRIVE_PRESET_7			0.875
#define tW1L_OVERDRIVE_PRESET_8			1
#define tW1L_OVERDRIVE_PRESET_9			1.125
#define tW1L_OVERDRIVE_PRESET_A			1.25
#define tW1L_OVERDRIVE_PRESET_B			1.375
#define tW1L_OVERDRIVE_PRESET_C			1.5
#define tW1L_OVERDRIVE_PRESET_D		    1.625
#define tW1L_OVERDRIVE_PRESET_E			1.75
#define tW1L_OVERDRIVE_PRESET_F			1.875

//tMSR - Standard Speed
#define tMSR_STANDARD_PRESET_0			5
#define tMSR_STANDARD_PRESET_1		    7
#define tMSR_STANDARD_PRESET_2			9
#define tMSR_STANDARD_PRESET_3			10.5
#define tMSR_STANDARD_PRESET_4			11
#define tMSR_STANDARD_PRESET_5			11.5
#define tMSR_STANDARD_PRESET_6			12
#define tMSR_STANDARD_PRESET_7			13
#define tMSR_STANDARD_PRESET_8			14
#define tMSR_STANDARD_PRESET_9			15
#define tMSR_STANDARD_PRESET_A			16
#define tMSR_STANDARD_PRESET_B			17
#define tMSR_STANDARD_PRESET_C			18
#define tMSR_STANDARD_PRESET_D			19
#define tMSR_STANDARD_PRESET_E			19.5
#define tMSR_STANDARD_PRESET_F			20

//tMSR - Overdrive Speed
#define tMSR_OVERDRIVE_PRESET_0			1
#define tMSR_OVERDRIVE_PRESET_1			1.125
#define tMSR_OVERDRIVE_PRESET_2			1.25
#define tMSR_OVERDRIVE_PRESET_3			1.375
#define tMSR_OVERDRIVE_PRESET_4			1.5
#define tMSR_OVERDRIVE_PRESET_5			1.625
#define tMSR_OVERDRIVE_PRESET_6			1.75
#define tMSR_OVERDRIVE_PRESET_7			1.875
#define tMSR_OVERDRIVE_PRESET_8			2
#define tMSR_OVERDRIVE_PRESET_9			2.125
#define tMSR_OVERDRIVE_PRESET_A			2.25
#define tMSR_OVERDRIVE_PRESET_B			2.375
#define tMSR_OVERDRIVE_PRESET_C			2.5
#define tMSR_OVERDRIVE_PRESET_D		    2.625
#define tMSR_OVERDRIVE_PRESET_E			2.75
#define tMSR_OVERDRIVE_PRESET_F			2.875

/* 1-Wire Speeds */
typedef enum {
	STANDARD,
	OVERDRIVE,
} one_wire_speeds;

/* One Wire Timing Presets */
typedef enum {
	PRESET_0,
	PRESET_1,
	PRESET_2,
	PRESET_3,
	PRESET_4,
	PRESET_5,
	PRESET_6,
	PRESET_7,
	PRESET_8,
	PRESET_9,
	PRESET_A,
	PRESET_B,
	PRESET_C,
	PRESET_D,
	PRESET_E,
	PRESET_F,
} one_wire_timing_presets;

/* VTH Values */
typedef enum {
	VTH_LOW,
	VTH_MEDIUM,
	VTH_HIGH,
	VTH_OFF
} vth_values;

/* VIAPO Values */
typedef enum {
	VIAPO_LOW,
	VIAPO_MEDIUM,
	VIAPO_HIGH,
	VIAPO_OFF
} viapo_values;

/* RWPU Values */
typedef enum {
	RWPU_EXTERNAL,
	RWPU_500,
	RWPU_1000,
	RWPU_333,
} rwpu_values;

/* GPIO Settings */
typedef enum {
	CONDUCTING = 0xAA,
	NON_CONDUCTING = 0x55,
	CONDUCTING_NO_LEVEL_READ = 0xA5,
	NON_CONDUCTING_NO_LEVEL_READ = 0x5A,
} gpio_settings;

/* GPIO Verify Level Detection */
typedef enum {
	LOW = 0xAA,
	HIGH = 0x55,
} gpio_verify_level_detection;


/* **** Globals **** */

extern uint8_t oneWireScript[126];
extern uint8_t oneWireScript_length;
extern double oneWireScript_accumulativeOneWireTime;
extern uint8_t oneWireScript_commandsCount;
extern uint8_t oneWireScriptResponse[126];
extern uint8_t oneWireScriptResponse_length;

/***** Low Level Functions *****/
int OneWire_ResetPulse(void);
int OneWire_WriteByte(uint8_t byte);
int OneWire_WriteBlock(uint8_t *data, int data_length);
uint8_t OneWire_ReadByte(void);
void OneWire_ReadBlock(uint8_t *data, int data_length);
int OneWire_Search(OneWire_ROM_ID_T *romid, bool search_reset, bool *last_device_found);
int OneWire_WriteBytePower(int send_byte);

/***** High Level Functions *****/
int OneWire_Enable_APU(bool apu);
int OneWire_Enable_SPU(bool spu);
int OneWire_Enable_OneWirePowerDown(bool pdn);
int OneWire_Set_OneWireMasterSpeed(one_wire_speeds spd);
int OneWire_Get_OneWireMasterSpeed(one_wire_speeds *spd);

//RPUP/BUF
int OneWire_Set_Custom_RPUP_BUF(vth_values vth, viapo_values viapo, rwpu_values rwpu);
int OneWire_Get_Custom_RPUP_BUF(vth_values *vth, viapo_values *viapo, rwpu_values *rwpu); //overwrites vth, viapo, and rwpu with value read from DS2485

//tRSTL
int OneWire_Set_tRSTL_Standard_Predefined(one_wire_timing_presets trstl);
int OneWire_Set_tRSTL_Overdrive_Predefined(one_wire_timing_presets trstl);
int OneWire_Set_tRSTL_Standard_Custom(double trstl); // Max = 1020 us
int OneWire_Set_tRSTL_Overdrive_Custom(double trstl); // Max = 126 us
int OneWire_Get_tRSTL(double *trstl, one_wire_speeds spd); //overwrites trstl in us

//tRSTH
int OneWire_Set_tRSTH_Standard_Predefined(one_wire_timing_presets trsth);
int OneWire_Set_tRSTH_Overdrive_Predefined(one_wire_timing_presets trsth);
int OneWire_Set_tRSTH_Standard_Custom(double trsth); // Max = 1020 us
int OneWire_Set_tRSTH_Overdrive_Custom(double trsth); // Max = 126 us
int OneWire_Get_tRSTH(double *trsth, one_wire_speeds spd); //overwrites trsth in us

//tRSTH
int OneWire_Set_tW0L_Standard_Predefined(one_wire_timing_presets tw0l);
int OneWire_Set_tW0L_Overdrive_Predefined(one_wire_timing_presets tw0l);
int OneWire_Set_tW0L_Standard_Custom(double tw0l); // Max = 126 us
int OneWire_Set_tW0L_Overdrive_Custom(double tw0l); // Max = 31.5 us
int OneWire_Get_tW0L(double *tw0l, one_wire_speeds spd); //overwrites tw0l in us

//tREC
int OneWire_Set_tREC_Standard_Predefined(one_wire_timing_presets trec);
int OneWire_Set_tREC_Overdrive_Predefined(one_wire_timing_presets trec);
int OneWire_Set_tREC_Standard_Custom(double trec); // Max = 255.5 us
int OneWire_Set_tREC_Overdrive_Custom(double trec); // Max = 255.5 us
int OneWire_Get_tREC(double *trec, one_wire_speeds spd); //overwrites trec in us

//tMSI
int OneWire_Set_tMSI_Standard_Predefined(one_wire_timing_presets tmsi);
int OneWire_Set_tMSI_Overdrive_Predefined(one_wire_timing_presets tmsi);
int OneWire_Set_tMSI_Standard_Custom(double tmsi); // Max = 15.5 us
int OneWire_Set_tMSI_Overdrive_Custom(double tmsi); // Max = 3.875 us
int OneWire_Get_tMSI(double *tmsi, one_wire_speeds spd); //overwrites tmsi in us

//tMSP
int OneWire_Set_tMSP_Standard_Predefined(one_wire_timing_presets tmsp);
int OneWire_Set_tMSP_Overdrive_Predefined(one_wire_timing_presets tmsp);
int OneWire_Set_tMSP_Standard_Custom(double tmsp); // Max = 127 us
int OneWire_Set_tMSP_Overdrive_Custom(double tmsp); // Max = 15.5 us
int OneWire_Get_tMSP(double *tmsp, one_wire_speeds spd); //overwrites tmsp in us

//tW1L
int OneWire_Set_tW1L_Standard_Predefined(one_wire_timing_presets tw1l);
int OneWire_Set_tW1L_Overdrive_Predefined(one_wire_timing_presets tw1l);
int OneWire_Set_tW1L_Standard_Custom(double tw1l); // Max = 31.5 us
int OneWire_Set_tW1L_Overdrive_Custom(double tw1l); // Max = 1.9375 us
int OneWire_Get_tW1L(double *tw1l, one_wire_speeds spd); //overwrites tw1l in us

//tMSR
int OneWire_Set_tMSR_Standard_Predefined(one_wire_timing_presets tmsr);
int OneWire_Set_tMSR_Overdrive_Predefined(one_wire_timing_presets tmsr);
int OneWire_Set_tMSR_Standard_Custom(double tmsr); // Max = 31.5 us
int OneWire_Set_tMSR_Overdrive_Custom(double tmsr); // Max = 3.875 us
int OneWire_Get_tMSR(double *tmsr, one_wire_speeds spd); //overwrites tmsr in us

/* Primitive Script Commands */
extern void OneWire_Script_Clear(void);
extern int OneWire_Script_Execute(void);
extern int OneWire_Script_Add_OW_RESET(uint8_t *response_index, one_wire_speeds spd, bool ignore);
extern int OneWire_Script_Add_OW_WRITE_BIT(uint8_t *response_index, bool bit_value);
extern int OneWire_Script_Add_OW_READ_BIT(uint8_t *response_index);
extern int OneWire_Script_Add_OW_WRITE_BYTE(uint8_t *response_index, uint8_t txByte);
extern int OneWire_Script_Add_OW_READ_BYTE(uint8_t *response_index);
extern int OneWire_Script_Add_OW_TRIPLET(uint8_t *response_index, bool t_value);
extern int OneWire_Script_Add_OV_SKIP(uint8_t *response_index);
extern int OneWire_Script_Add_SKIP(uint8_t *response_index);
extern int OneWire_Script_Add_OW_READ_BLOCK(uint8_t *response_index, uint8_t rxBytes);
extern int OneWire_Script_Add_OW_WRITE_BLOCK(uint8_t *response_index, const uint8_t *txData, uint8_t txData_length);
extern void OneWire_Script_Add_DELAY(uint8_t ms);
extern void OneWire_Script_Add_PRIME_SPU(void);
extern void OneWire_Script_Add_SPU_OFF(void);
extern int OneWire_Script_Add_SPEED(one_wire_speeds spd, bool ignore);
extern int OneWire_Script_Add_VERIFY_TOGGLE(uint8_t *response_index);
extern int OneWire_Script_Add_VERIFY_BYTE(uint8_t *response_index, uint8_t byte);
extern void OneWire_Script_Add_CRC16_START(void);
extern void OneWire_Script_Add_VERIFY_CRC16(uint8_t *response_index, unsigned short hex_value);
extern void OneWire_Script_Add_SET_GPIO(uint8_t *response_index, gpio_settings pioac);
extern void OneWire_Script_Add_READ_GPIO(uint8_t *response_index);
extern void OneWire_Script_Add_VERIFY_GPIO(uint8_t *response_index, gpio_verify_level_detection pioal);
extern void OneWire_Script_Add_CONFIG_RPUP_BUF(unsigned short hex_value);

int OneWire_Init(void);


#ifdef __cplusplus
}
#endif

#endif /* _ONE_WIRE_TEST_H_ */
