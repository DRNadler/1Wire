/*
 * ENS210_Result.cpp - I2C Temperature and Humidity sensor reading results
 *
 * References:
 *   Vendor-provided Arduino driver:
 *     https://github.com/sciosense/ENS210_driver (2020 Apr 06	v3)
 *
 *  Created on: Oct 27, 2023
 *      Author: Dave Nadler
 */

#include "ENS210_Result.hpp"

float ENS210_Result_T::TempKelvin() const
{
	float t = rawTemperature;
	// Return m*K. This equals m*(t/64) = (m*t)/64
	// Note m is the multiplier, K is temperature in Kelvin, t is raw t_data value.
	// Uses K=t/64.
	return t/64; //IDIV(t,64);
}
float ENS210_Result_T::TempCelsius() const
{
	//assert( (1<=multiplier) && (multiplier<=1024) );
	float t= rawTemperature;
	// Return m*C. This equals m*(K-273.15) = m*K - 27315*m/100 = m*t/64 - 27315*m/100
	// Note m is the multiplier, C is temperature in Celsius, K is temperature in Kelvin, t is rawTemperature.
	// Uses C=K-273.15 and K=t/64.
	return t/64 - 273.15F; //IDIV(t,64) - IDIV(27315L,100);
}
float ENS210_Result_T::TempFahrenheit() const
{
	float t= rawTemperature & 0xFFFF;
	// Return m*F. This equals m*(1.8*(K-273.15)+32) = m*(1.8*K-273.15*1.8+32) = 1.8*m*K-459.67*m = 9*m*K/5 - 45967*m/100 = 9*m*t/320 - 45967*m/100
	// Note m is the multiplier, F is temperature in Fahrenheit, K is temperature in Kelvin, t is rawTemperature.
	// Uses F=1.8*(K-273.15)+32 and K=t/64.
	return 9*t/320 - 45967/100.0F; //IDIV(9*t,320) - IDIV(45967L,100);
	// The first multiplication stays below 32 bits (t:16, multiplier:11, 9:4)
	// The second multiplication stays below 32 bits (multiplier:10, 45967:16)
}
float ENS210_Result_T::HumidityPercent() const
{
	float h= rawHumidity;
	// Return m*H. This equals m*(h/512) = (m*h)/512
	// Note m is the multiplier, H is the relative humidity in %RH, h is raw h_data value.
	// Uses H=h/512.
	return h/512; //IDIV(h, 512);
}
float ENS210_Result_T::AbsoluteHumidityPercent() const
{
	const float MOLAR_MASS_OF_WATER = 18.01534F;
	const float UNIVERSAL_GAS_CONSTANT = 8.21447215F;
	//taken from https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
	//precision is about 0.1°C in range -30 to 35°C
	//August-Roche-Magnus   6.1094 exp(17.625 x T)/(T + 243.04)
	//Buck (1981)     6.1121 exp(17.502 x T)/(T + 240.97)
	//reference https://www.eas.ualberta.ca/jdwilson/EAS372_13/Vomel_CIRES_satvpformulae.html    // Use Buck (1981)
	float degreesC = TempCelsius();
	return (6.1121F * std::pow(2.718281828F,(17.67F* degreesC)/(degreesC + 243.5F)) *
                         HumidityPercent() *MOLAR_MASS_OF_WATER)/((273.15F+ degreesC )*UNIVERSAL_GAS_CONSTANT);
}

#include <stdio.h> // diagnostic printf only
void ENS210_Result_T::DiagPrintf() const {
	if(status != Status_OK)
		printf("ENS210: Bogus status =%d\n", status);
	else
		printf("ENS210: Temperature degreesCx10 = %d, Humidityx10=%d\n", TempCelsiusX10(), HumidityPercentX10());
}
