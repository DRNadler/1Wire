/*
 * ENS210.cpp - I2C Temperature and Humidity sensor driver
 *
 * References:
 *   Vendor-provided Arduino driver:
 *     https://github.com/sciosense/ENS210_driver (2020 Apr 06 v3)
 *
 * ==========================================================================
 * imxRT1024 -> I2C -> DS2485 ------> 1-Wire ------> DS28E18 -> I2C -> ENS210
 * ==========================================================================
 * This ENS210 driver operates ENS210 via a one-wire bus and DS28E18.
 * Instead of directly commanding the ENS210 over a host I2C interface,
 * this code creates a DS28E18 'command sequence' and runs it on the DS28E18.
 * Layers are:
 * - code below provides the ENS210 API, using
 *   - DS28E18 driver, which sends command sequence to DS28E18 over 1-Wire via
 *     - one_wire command layer, which calls
 *       - DS2485 driver, which calls
 *         - NXP FSL I2C driver (or any I2C driver you substitute)
 *
 * Notes:
 * - I2C hardware initialization takes place in the platform-specific
 *   DS2485_ExecuteCommand, which isolates I2C access to DS2485.
 *
 *  Created on: Oct 27, 2023
 *      Author: Dave Nadler
 */

// ToDo ENS210: Add solderOffset support
// ToDo ENS210: conditional debug printf's
// ToDo ENS210: Could use on-demand measurements rather than continual


#include <assert.h>
#include <stdio.h> // Diagnostic printf

#include "ENS210.hpp" // public interface for this class

// Maxim 1-Wire
#include "1wire/one_wire.h"
#include "1wire/DS28E18.h"


// ==========  Internal ENS210 definitions (not part of public interface)  ==========
enum ENS210_REG : uint8_t { // ENS210 register addresses (not public)
	ENS210_REG_PART_ID      = 0x00,
	ENS210_REG_UID          = 0x04,
	ENS210_REG_SYS_CTRL     = 0x10,
	ENS210_REG_SYS_STAT     = 0x11,
	ENS210_REG_SENS_RUN     = 0x21,
	ENS210_REG_SENS_START   = 0x22,
	ENS210_REG_SENS_STOP    = 0x23,
	ENS210_REG_SENS_STAT    = 0x24,
	ENS210_REG_T_VAL        = 0x30,
	ENS210_REG_H_VAL        = 0x33,
};
static const uint8_t ENS210_I2C_SlaveAddressShifted = 0x43 << 1u; // left-shift accommodates low-order read/!write bit
static const int ENS210_PartID = 0x0210;
static const int ENS210_Boot_Time_MS   = 2;      // Time to boot in ms (also after reset, or going to high power)
static const int ENS210_THConv_Single_MS = 130;  // Conversion time in ms for single shot T/H measurement
static const int ENS210_THConv_Continuous_MS = 238; // Conversion time in ms for continuous T/H measurement

static const uint8_t ENS210_reset[] =
	{ ENS210_I2C_SlaveAddressShifted, ENS210_REG_SYS_CTRL, 0x80};// SYS_CTRL = x80 does device reset, boot time - 1.2ms
static const uint8_t ENS210_setActive[] =
	{ ENS210_I2C_SlaveAddressShifted, ENS210_REG_SYS_CTRL, 0x00};// Disable low-power (device stays active): SYS_CTRL = x00
static const uint8_t ENS210_setContinuousAndStart[] =
	{ ENS210_I2C_SlaveAddressShifted, ENS210_REG_SENS_RUN, 0x03, 0x03};// SENS_RUN=3 SENS_START=3 enables and starts both temperature and humidity

// dataStream first byte is starting register, followed by register value(s)
void ENS210_T::writeRegisters(const uint8_t *dataStream, int len) {
	DS28E18_BuildPacket_I2C_Start();
	//printf("ENS210_T::writeRegisters register %02X command sequence at index %d\n", dataStream[0], DS28E18_BuildPacket_GetSequencerPacketSize());
	DS28E18_BuildPacket_I2C_WriteData(dataStream, len);
	DS28E18_BuildPacket_I2C_Stop();
}
int ENS210_T::readRegisters(uint8_t firstRegister, int len) {
	// Note for ENS210: a repeated start is required before reading
	uint8_t setCAR[2] = { ENS210_I2C_SlaveAddressShifted, firstRegister };
	DS28E18_BuildPacket_I2C_Start();
	//printf("ENS210_T::readRegisters register %02X command sequence at index %d, ", firstRegister, DS28E18_BuildPacket_GetSequencerPacketSize());
	// First write the starting address to DS28E18 (set the CAR Current Address Register)
	DS28E18_BuildPacket_I2C_WriteData(setCAR, 2);
	// Address the Device with read bit set to commence read
	DS28E18_BuildPacket_I2C_Start(); // repeated start before read start
	static const unsigned char ENS210_read[] =
		{ ENS210_I2C_SlaveAddressShifted | 0x01 };  // read start adds 'Read' bit to I2C address...
	DS28E18_BuildPacket_I2C_WriteData(ENS210_read, sizeof(ENS210_read)); // send read to ENS210 address
	uint8_t resultIdx = DS28E18_BuildPacket_I2C_ReadDataWithNackEnd(len); // finally read back 'len' bytes
	DS28E18_BuildPacket_I2C_Stop();
	//printf("result at index %d\n", resultIdx);
	return resultIdx;
}


bool ENS210_T::Init() {
	initOK = false;
	do {
		// Initialize Maxim 1-Wire library (beneath the hood, initializes I2C to DS2485 and DS2485)
		int OneWireInitError = OneWire_Init();
		assert(OneWireInitError==0);
		if(OneWireInitError) break;

		// DS28E18 VDD_SENS (DS28E18 power to the sensor) requires 'Strong Pull-Up' 'SPU'
		// on the 1-Wire bus. That's turned on with DS2485 1-Wire Master Configuration
		// (Register 0) Bit 13: Strong Pullup (SPU).
		int SPUerror = OneWire_Enable_SPU(true); // DS2485 must provide strong power to 1-Wire bus
		assert(SPUerror==0);
		if(SPUerror) break;

		// \ToDo ENS210: Assumes there's only one DS28E18 on 1-Wire bus (controlling ENS210); could search in Init()...
		bool ds28e18_init_OK = DS28E18_Init(); // global current_DS28E18_ROM_ID is set to last 1-Wire device found
		assert (ds28e18_init_OK);
		if(!ds28e18_init_OK) break;
		OneWireAddress = current_DS28E18_ROM_ID;

		// For temperature probe, use DS28E18Q+T internal I2C pull-up resistors,
		// which must be enabled **BEFORE** powering up sensor with hard-VDD-pullup
		// Turn on DS28E18 1.2k pull-ups for I2C SDA/SCL lines
		// GPIO_CTRL_HI register, bit position within 4-bit subfield for each output:
		//   3 SDA/MOSI
		//   2 GPIOB/MISO (not connected, don't care)
		//   1 SCL/SCLK
		//   0 GPIOA/SS# (not connected, don't care)
		// For 25k pull-up on all IO, PS=0, PW=F; GPIO_CTRL_HI=x0F
		// For stronger 1.2k pull-up on all IO, PS=0, PW=F; GPIO_CTRL_HI=xF0
		// No pull-down slew, outputs high or release line depending on pull-up selected: GPIO_CTRL_LO=0x0F
		// Stronger pull-ups seem to be required (sometimes got 0 values during reads with weak pull-ups)
		bool configured_GPIO_OK = DS28E18_WriteGpioConfiguration(CONTROL, 0xF0, 0x0F);
		assert(configured_GPIO_OK);
		if(!configured_GPIO_OK) break;

		// ===================================  ENS210 I2C  ==========================================
		// The ENS210 I²C interface supports standard (100kbit/s) and fast (400kbit/s) mode.
		// The device applies all mandatory I²C protocol features for slaves: START, STOP, Acknowledge,
		// 7-bit slave address. ENS210 does not use clock stretching.
		// None of the other optional I²C features (10-bit slave address, General Call, Software reset, or Device ID)
		// are supported, nor are the master features (Synchronization, Arbitration, START byte).
		DS28E18_WriteConfiguration(KHZ_400, DONT_IGNORE, I2C, /* SPI mode ignored for I2C: */MODE_0);

		// Apply power to ENS210 I2C sensor (after setting pull-ups above)
		DS28E18_BuildPacket_ClearSequencerPacket();
		DS28E18_BuildPacket_Utility_SensVddOn();
		DS28E18_BuildPacket_Utility_Delay(DELAY_256msec); // give sensor time to power up 64->256
		// Write the packet into DS28E18 sequencer memory, run it, and wait long enough for it to complete
		bool writeAndRun_PowerUp_OK = DS28E18_BuildPacket_WriteAndRun();
		assert(writeAndRun_PowerUp_OK);
		if(!writeAndRun_PowerUp_OK) break;

		// --- Reset ENS210 by writing 0x80 to SYS_CTRL register at Address 0x10 ---
		DS28E18_BuildPacket_ClearSequencerPacket();
		writeRegisters(ENS210_reset, sizeof(ENS210_reset));
		// --- Wait Tboot=1.2ms for reset to complete ---
		DS28E18_BuildPacket_Utility_Delay(DELAY_8msec); // tBoot can be up to 1.2msec, pad it a bit
		// --- Set ENS210 active (disable low power) by writing 0x00 to SYS_CTRL register at Address 0x10 ---
		writeRegisters(ENS210_setActive, sizeof(ENS210_setActive)); // activate ENS210 sensor
		// --- Delay for boot
		DS28E18_BuildPacket_Utility_Delay(DELAY_8msec); // tBoot can be up to 1.2msec, give it >2msec 16->256
		// --- Read SYS_STAT and PART_ID
		uint8_t SYS_STAT_idx = readRegisters(ENS210_REG_SYS_STAT, 1); // read 1 byte
		uint8_t PART_ID_idx = readRegisters(ENS210_REG_PART_ID, 2+2+8); // read 2 bytes PART_ID, 2 bytes DIE_REV, and 8 bytes UID
		// Write the packet into DS28E18 sequencer memory, run it, and wait long enough for it to complete
		bool writeAndRun_StatusAndPartID_OK = DS28E18_BuildPacket_WriteAndRun();
		assert(writeAndRun_StatusAndPartID_OK);
		if(!writeAndRun_StatusAndPartID_OK) break;

		// read DS28E18 sequencer memory back to host to extract SYS_STAT and PART_ID-DIE_ID-UID values read from sensor
		uint8_t sequencer_memory[DS28E18_BuildPacket_GetSequencerPacketSize()] = {0};
		DS28E18_ReadSequencer(0x00, sequencer_memory, sizeof(sequencer_memory));
		SYS_STAT = sequencer_memory[SYS_STAT_idx];
		PART_ID = sequencer_memory[PART_ID_idx+1]<<8 | sequencer_memory[PART_ID_idx]; // looking for 0x0210 - OK
		printf("ENS210::Init read SYS_STAT=x%02X, PARTID=x%04X\n", SYS_STAT, PART_ID);
		assert(SYS_STAT == 1); // should be in active state - OK
		assert(PART_ID == 0x0210);
		if( !(SYS_STAT==1 && PART_ID==0x0210) ) break;
		uint8_t DIE_REV_idx = PART_ID_idx+2;
		dieRevision = sequencer_memory[DIE_REV_idx+1]<<8 | sequencer_memory[DIE_REV_idx];
		uint8_t UID_idx = DIE_REV_idx+2;
		uniqueDeviceID =
				(uint64_t)sequencer_memory[UID_idx+7]<<56 |
				(uint64_t)sequencer_memory[UID_idx+6]<<48 |
				(uint64_t)sequencer_memory[UID_idx+5]<<40 |
				(uint64_t)sequencer_memory[UID_idx+4]<<32 |
				(uint64_t)sequencer_memory[UID_idx+3]<<24 |
				(uint64_t)sequencer_memory[UID_idx+2]<<16 |
				(uint64_t)sequencer_memory[UID_idx+1]<< 8 |
				(uint64_t)sequencer_memory[UID_idx+0]<< 0 ;
		printf("ENS210::Init read dieRevision=x%02X, uniqueDeviceID=x%016llX\n", dieRevision, uniqueDeviceID);

		// Set continuous mode and start for both temperature and humidity sensor
		DS28E18_BuildPacket_ClearSequencerPacket();
		writeRegisters(ENS210_setContinuousAndStart, sizeof(ENS210_setContinuousAndStart)); // run ENS210 sensors continuously
		// Immediately after starting continuous, wait max conversion time for both temp and humidity = 238ms
		DS28E18_BuildPacket_Utility_Delay(DELAY_256msec);
		// Write the packet into DS28E18 sequencer memory, run it, and wait long enough for it to complete
		bool started_OK = DS28E18_BuildPacket_WriteAndRun();
		if(!started_OK) break;

		initOK = true;
	} while(0);

	return initOK;
}

ENS210_Result_T ENS210_T::Measure() {
	ENS210_Result_T result;

	do {
		if(! initOK) Init();
		if(! initOK) break; // arrrggg...

		// Address this ENS210's DS28E18 controller on the 1-wire bus.
		current_DS28E18_ROM_ID = OneWireAddress;

		// Set up and run DS28E18 sequencer (inside temperature probe) to read ENS210 temperature and humidity
		bool readTemperatureAndHumidty_OK;
		// saved information about loaded sequence...
		static bool DS28E18_SequenceLoaded = false;
		static uint8_t T_VAL_idx;
		static unsigned short TH_readSequenceLength;
		if(!DS28E18_SequenceLoaded)		{
			// This sequence takes ~215mSec (including read-back below)
		// Read temperature and humidity: 6 bytes (T_VAL and H_VAL) starting at T_VAL register
		DS28E18_BuildPacket_ClearSequencerPacket();
			T_VAL_idx = readRegisters(ENS210_REG_T_VAL, 6);
			TH_readSequenceLength = DS28E18_GetLastSequenceLength();
		// Write the packet into DS28E18 sequencer memory, run it, and wait long enough for it to complete
			readTemperatureAndHumidty_OK = DS28E18_BuildPacket_WriteAndRun();
			DS28E18_SequenceLoaded = true;
		} else {
			// This sequence takes ~140mSec (including read-back below); saves 75mSec by not reloading DS28E18 sequencer
			readTemperatureAndHumidty_OK = DS28E18_RerunLastSequence(TH_readSequenceLength);
		}
		assert(readTemperatureAndHumidty_OK);
		(void)readTemperatureAndHumidty_OK; // ToDo ENS210: Never observed, but perhaps handle this error?

		// Read DS28E18 sequencer memory back to host to obtain values read from sensor
		uint8_t readback2[DS28E18_BuildPacket_GetSequencerPacketSize()] = {0};
		DS28E18_ReadSequencer(0x00, readback2, sizeof(readback2));
		//printf("ENS210 T_VAL, H_VAL with checksums: x%02X%02X%02X, %02X%02X%02X\n",
		//		readback2[T_VAL_idx],readback2[T_VAL_idx+1],readback2[T_VAL_idx+2],readback2[T_VAL_idx+3],readback2[T_VAL_idx+4],readback2[T_VAL_idx+5]);
		// Lambda function extracts raw returned value and verifies checksum
		auto GetVal = [this](uint8_t *p, uint32_t &val, bool &OK, bool &validCRC) {
			// Note low-order value is first byte, then high-order, then CRC and "OK" bit
			val = p[1]<<8 | p[0];
			OK  = p[2] & 0x01;
			uint8_t recdCRC = (p[2]>>1)&0x7F;
			uint32_t payload = (((uint32_t)OK)<<16) | val;
			uint32_t calcdCRC = crc7(payload);
			validCRC = (recdCRC == calcdCRC);
		};
		uint32_t T_val;
		bool T_OK;
		bool T_validCRC;
		GetVal(&readback2[T_VAL_idx], T_val, T_OK, T_validCRC);
		uint32_t H_val;
		bool H_OK;
		bool H_validCRC;
		GetVal(&readback2[T_VAL_idx+3], H_val, H_OK, H_validCRC);
		// Verify checksums OK
		if(!T_validCRC || !H_validCRC) {
			result.status = ENS210_Result_T::Status_CRC_error;
			break;
		}
		// Verify 'data valid' bits set
		if(!(T_OK && H_OK)) {
			result.status = ENS210_Result_T::Status_Invalid;
			break;
		}
		// Store the valid result!
		result.rawTemperature = T_val  - soldercorrection;
		result.rawHumidity    = H_val;
		result.status = ENS210_Result_T::Status_OK;

	} while(0);

	return result;
}

// ToDo ENS210: Move QwikTest and statically allocated ENS210_T instance out of driver...
ENS210_T ENS210; // global scope object
void ENS210_T::QwikTest() {
	ENS210_Result_T r = Measure(); // does Init() if not yet completed
	// report results
	static bool initSummaryPrinted;
	if(!initSummaryPrinted && initOK) {
		printf("ENS210::Init read SYS_STAT=x%02X, PARTID=x%04X\n", SYS_STAT, PART_ID);
		printf("ENS210::Init read dieRevision=x%02X, uniqueDeviceID=x%016llX\n", dieRevision, uniqueDeviceID);
		initSummaryPrinted = true;
	}
		r.DiagPrintf();
}

// Compute the CRC-7 of 'val' (should only have 17 bits)
// https://en.wikipedia.org/wiki/Cyclic_redundancy_check#Computation
//               7654 3210
// Polynomial 0b 1000 1001 ~ x^7+x^3+x^0
//            0x    8    9
#define CRC7WIDTH  7    					// A 7 bits CRC has polynomial of 7th order, which has 8 terms
#define CRC7POLY   0x89 					// The 8 coefficients of the polynomial
#define CRC7IVEC   0x7F 					// Initial vector has all 7 bits high
// Payload data
#define DATA7WIDTH 17
#define DATA7MASK  ((1UL<<DATA7WIDTH)-1) 	// 0b 0 1111 1111 1111 1111
#define DATA7MSB   (1UL<<(DATA7WIDTH-1)) 	// 0b 1 0000 0000 0000 0000
uint32_t ENS210_T::crc7( uint32_t val )
{
	// Setup polynomial
	uint32_t pol= CRC7POLY;
	// Align polynomial with data
	pol = pol << (DATA7WIDTH-CRC7WIDTH-1);
	// Loop variable (indicates which bit to test, start with highest)
	uint32_t bit = DATA7MSB;
	// Make room for CRC value
	val = val << CRC7WIDTH;
	bit = bit << CRC7WIDTH;
	pol = pol << CRC7WIDTH;
	// Insert initial vector
	val |= CRC7IVEC;
	// Apply division until all bits done
	while( bit & (DATA7MASK<<CRC7WIDTH) ) {
		if( bit & val ) val ^= pol;
		bit >>= 1;
		pol >>= 1;
	}
	return val;
}
