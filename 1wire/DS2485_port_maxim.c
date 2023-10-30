// DS2485_port_maxim.c - Platform-specific interface to drive DS2485 via I2C
// Platform-dependent code isolated here by Dave Nadler 18-May-2023
// Not used in SensorBox/Vario application.


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
#include <string.h>  // memcpy...

#if 1 // Maxim-specific
  #include "i2c.h"
  #include "mxc_delay.h"
  #include "mxc_sys.h"
  #include "tmr.h"
  #include "mxc_config.h"
#endif

#include "DS2485.h"

/* **** Definitions **** */
#define I2C_MASTER	    MXC_I2C0
#define I2C_MASTER_IDX	0
#define I2C_SLAVE_ADDR	(DS2485_I2C_7BIT_ADDRESS << 1)

/* **** Globals **** */

/* **** Functions **** */
int DS2485_ExecuteCommand(const uint8_t *packet, int packetSize, int delay_uSec, uint8_t *response, int responseSize)
{
    int error = 0;
    const sys_cfg_i2c_t sys_i2c_cfg = NULL;

    //Setup the I2C Master
    I2C_Shutdown(I2C_MASTER);
    if((error = I2C_Init(I2C_MASTER, I2C_STD_MODE, &sys_i2c_cfg)) != E_NO_ERROR) {
        return error;
    }

    if((error = I2C_MasterWrite(MXC_I2C0, I2C_SLAVE_ADDR, packet, packetSize, 0)) != packetSize) {
        return error;
    }

    mxc_delay(MXC_DELAY_MSEC(delay_uSec));

    //Read out Length Byte
    if((error = I2C_MasterRead(MXC_I2C0, I2C_SLAVE_ADDR, response, responseSize, 0)) != responseSize) {
        return error;
    }

    return 0;
}
