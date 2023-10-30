/*
 * ENS210.hpp - I2C Temperature and Humidity sensor driver
 *
 *  Created on: Oct 27, 2023
 *      Author: Dave Nadler
 */

#ifndef ENS210_HPP_
#define ENS210_HPP_

#include <stdint.h>

#include "ENS210_Result.hpp"
#include "1wire/one_wire_address.h"

class ENS210_T {
	bool initOK = false;
	uint8_t soldercorrection = 0; // Correction due to soldering (in 1/64K); subtracted from rawTemperature by measure function.
	uint16_t dieRevision = 0;
	uint64_t uniqueDeviceID = 0;
	uint32_t crc7( uint32_t val ); // calculate ENS210 checksum for a raw temperature or humidity value
	// *** Following members are specific to the DS28E18 controlling this ENS210 on a 1-Wire bus ***
	OneWire_ROM_ID_T OneWireAddress; ///< Address of the DS28E18 controlling this ENS210 on the 1-Wire bus.
	// Append a write to the command sequence under construction
	// dataStream first byte is starting register, followed by register value(s)
	void writeRegisters(const uint8_t *dataStream, int len);
	// Append a read to the command sequence under construction
	// Return value is the index of the result in the readback command sequence
	int readRegisters(uint8_t firstRegister, int len);
public:
	ENS210_T() : OneWireAddress() {}; // ctor does NOT do device initialization; permits static allocation...
	bool Init();
	static void QwikTest();
	ENS210_Result_T Measure();
};

#endif /* ENS210_HPP_ */
