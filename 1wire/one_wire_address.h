/*!
 * @file    one_wire_address.h
 * @brief   Address of a device on a 1-Wire bus. Separate file to allow inclusion by high-level functions without namespace pollution from DS2485 etc.
 */

#ifndef ONE_WIRE_ADDRESS_H_INCLUDED
#define ONE_WIRE_ADDRESS_H_INCLUDED
#ifdef __cplusplus
  extern "C" {
#endif

#include <stdint.h>

/// OneWire_ROM_ID_T type stores a 1-Wire address
typedef struct {
	uint8_t ID[8];
} OneWire_ROM_ID_T;


#ifdef __cplusplus
}
#endif
#endif /* ONE_WIRE_ADDRESS_H_INCLUDED */
