/*
 * ENS210_Result.hpp - I2C Temperature and Humidity sensor reading results
 *
 * References:
 *   Vendor-provided Arduino driver:
 *     https://github.com/sciosense/ENS210_driver (2020 Apr 06	v3)
 *
 *  Created on: Oct 27, 2023
 *      Author: Dave Nadler
 */

#ifndef ENS210_RESULT_HPP_INCLUDED
#define ENS210_RESULT_HPP_INCLUDED

#include <stdint.h>
#include <cmath> // pow

/// Measurement result from ENS210
struct ENS210_Result_T {
	enum ENS210_Result_status_T : uint8_t {
		Status_NA = 0, ///< nothing here yet...
		Status_OK = 1, ///< The value was read, the CRC matches, and data is valid.
		Status_Invalid = 2, ///< The value was read, the CRC matches, but the data is invalid (e.g. the measurement was not yet finished).
		Status_CRC_error = 3, ///< The value was read, but the CRC over the payload (valid and data) does not match.
		Status_I2C_error = 4, ///< There was an I2C communication error attempting to read the value.
	};
	ENS210_Result_status_T status;
	// Note: raw values have been stripped of checksum; just the interesting 16-bit data here.
	uint16_t rawTemperature; ///< temperature in 1/64 Kelvin, corrected for solder offset
	uint16_t rawHumidity; ///< relative humidity in 1/512%RH (ie a value of 51200 means 100% relative humidity)
	ENS210_Result_T() : status(Status_NA), rawTemperature(0), rawHumidity(0) {};
	float TempKelvin() const;       // Convert to Kelvin
	int   TempKelvinx10() const { return (10*((unsigned int)rawTemperature))/64; };
	float TempCelsius() const;      // Convert to Celsius
	int   TempCelsiusX10() const { return TempKelvinx10() - (int)(273.15*10); };
	float TempFahrenheit() const;   // Convert to Fahrenheit
	int   TempFahrenheitx10() const;// Convert to Fahrenheit times 10, integer
	float HumidityPercent() const;  ///< Fetch relative humidity 0% to 100.0%
	int HumidityPercentX10() const {  return (10*((int)rawHumidity))/512; }; ///< Fetch relative humidity % x10, ie 395 means 39.5% relative humidity
	float AbsoluteHumidityPercent() const; // Convert to %aH
	void DiagPrintf() const;
};

#endif /* ENS210_RESULT_HPP_INCLUDED */
