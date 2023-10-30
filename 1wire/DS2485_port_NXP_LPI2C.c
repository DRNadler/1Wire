// DS2485_port_NXP_LPI2C.c - Platform-specific interface to drive DS2485 via I2C
// NXP LPI2C and FreeRTOS version by Dave Nadler 28-October-2023
// Note: Provides interface from imxRT1024 to DS2485 1-wire master chip.
// Note: DS2485 is not compatible with older DS2482 (for which lots of driver examples exist).
//
// Customize as needed below: Specific IO pins, LPCI2C peripheral, and DMA channels
//
// DS2485_ExecuteCommand does the following:
// - initialize I2C peripheral (first time only)
// - send DS2485 a command or script to execute as instructed by caller.
//   Note: DS2485 will perform command or execute script (sending data down 1-Wire, waiting, and gathering reply)
// - wait locally for DS2485 to do it's thing - might be a while
// - read the results/response out of DS2485

#define NXP_LPI2C_USE_DMA  // Non-DMA implementation below loops waiting for I2C FIFO to be empty

// ToDo 1-Wire: Time-outs in DS2485_port_NXP_LPI2C in case DS2485 does not reply

#include <stdint.h>
#include <string.h>
#include "assert.h"

#include "pin_mux.h"
#include "clock_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "fsl_iomuxc.h" // I2C IO pin setup
#include "fsl_lpi2c.h"
#include "fsl_lpi2c_edma.h"
#include "fsl_edma.h"
#include "fsl_dmamux.h"

#include "DS2485.h" // DS2485 I2C 1-Wire master

#ifdef NXP_LPI2C_USE_DMA
	#define LPI2C_DMA_MUX (DMAMUX)
	#define LPI2C_DMA     (DMA0)
	#define LPI2C3_TRANSMIT_EDMA_REQUEST_SOURCE kDmaRequestMuxLPI2C3
	#define LPI2C3_RECEIVE_EDMA_REQUEST_SOURCE  kDmaRequestMuxLPI2C3
	#define LPI2C3_TRANSMIT_DMA_CHANNEL 4U
	#define LPI2C3_RECEIVE_DMA_CHANNEL  5U
	#define LPI2C3_DMA_IRQ_PRIORITY \
	(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1) // logically 1 lower than configMAX_SYSCALL_INTERRUPT_PRIORITY
													   // same IRQ priority is used for LPUART and two DMA channels

	static int maxUsedBufferSize = 0; // 64 observed for Dave's application...
	#define I2C_DATA_LENGTH 128 // DS28E18_sequence_T max 512; accommodate maximum used above
	static AT_NONCACHEABLE_SECTION(uint8_t i2c_DMA_buf[I2C_DATA_LENGTH]);
	static AT_NONCACHEABLE_SECTION(lpi2c_master_edma_handle_t g_m_edma_handle);
	static edma_handle_t g_edmaTxHandle;
	static edma_handle_t g_edmaRxHandle;
	static void dmaCompleteCallback(LPI2C_Type *base,
			lpi2c_master_edma_handle_t *handle,
			status_t completionStatus,
			void *userData);
	static volatile bool txPending, rxPending; // waiting for DMA to complete?
	static SemaphoreHandle_t xResponseDataReadySemaphore = NULL;
	static StaticSemaphore_t xResponseDataReadySemaphoreBuffer;
#endif


static bool NXP_I2C_initialized;
static void NXP_I2C_init(void) {
	#if 0 // Pin setup and LP2I2C clocks should be initialized at application startup, not here...
		// SensorBox uses LPI2C3: Pin 47 is GPIO_SD_BD_01 SDA, Pin 48 is GPIO_SD_BD_00 SCL
		// ===================  imxRT1024 I2C IO pin setup  ==============================
		// MCUXpresso pin tool can initialize MCU pins for LPI2C3 IO, should generate the following...
		IOMUXC_SetPinMux(
			IOMUXC_GPIO_SD_B0_00_LPI2C3_SCL,  	/* GPIO_SD_B0_00 is configured as LPI2C3 SCL */
			1U);                              	/* ??? Software Input On Field: Force input path of pad GPIO_SD_B0_00 */
		IOMUXC_SetPinConfig(
			IOMUXC_GPIO_SD_B0_00_LPI2C3_SCL,    /* GPIO_SD_B0_00 functional properties : */
			0xD8B0U);                           /*   Slew Rate Field: Slow Slew Rate
													 Drive Strength Field: R0/6
													 Speed Field: 100MHz - 150MHz
													 Open Drain Enable Field: Open Drain Enabled
													 Pull / Keep Enable Field: Pull/Keeper Enabled
													 Pull / Keep Select Field: Keeper
													 Pull Up / Down Config. Field: 22K Ohm Pull Up
													 Hyst. Enable Field: Hysteresis Disabled */
		IOMUXC_SetPinMux(
			IOMUXC_GPIO_SD_B0_01_LPI2C3_SDA,  	/* GPIO_SD_BD_01 is configured as LPI2C3 SDA */
			1U);                              	/* ??? Software Input On Field: Force input path of pad GPIO_SD_B0_00 */
		IOMUXC_SetPinConfig(
			IOMUXC_GPIO_SD_B0_01_LPI2C3_SDA,	/* GPIO_SD_BD_01 PAD functional properties : */
			0xD8B0U);                           /*   Slew Rate Field: Slow Slew Rate
													 Drive Strength Field: R0/6
													 Speed Field: 100MHz - 150MHz
													 Open Drain Enable Field: Open Drain Enabled
													 Pull / Keep Enable Field: Pull/Keeper Enabled
													 Pull / Keep Select Field: Keeper
													 Pull Up / Down Config. Field: 22K Ohm Pull Up
													 Hyst. Enable Field: Hysteresis Disabled */
	#endif

    // Set up I2C as required for communication with DS2485
	lpi2c_master_config_t masterConfig = {0};
	LPI2C_MasterGetDefaultConfig(&masterConfig);
	/*
	 * masterConfig.debugEnable = false;
	 * masterConfig.ignoreAck = false;
	 * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
	 * masterConfig.baudRate_Hz = 100000U; // 100kHz
	 * masterConfig.busIdleTimeout_ns = 0;
	 * masterConfig.pinLowTimeout_ns = 0;
	 * masterConfig.sdaGlitchFilterWidth_ns = 0;
	 * masterConfig.sclGlitchFilterWidth_ns = 0;
	 */
	masterConfig.baudRate_Hz = DS2485_I2C_CLOCKRATE;
	LPI2C_MasterInit(LPI2C3, &masterConfig, BOARD_BOOTCLOCKRUN_LPI2C_CLK_ROOT);

	#ifdef NXP_LPI2C_USE_DMA
		#if 0 // DMA and DMAMUX must be initialized at application startup, not here...
			DMAMUX_Init(LPI2C_DMA_MUX);
			edma_config_t userConfig = {0};
			EDMA_GetDefaultConfig(&userConfig);
			EDMA_Init(LPI2C_DMA, &userConfig);
		#endif
	    /* Create the EDMA channel handles */
	    EDMA_CreateHandle(&g_edmaTxHandle, LPI2C_DMA, LPI2C3_TRANSMIT_DMA_CHANNEL);
	    EDMA_CreateHandle(&g_edmaRxHandle, LPI2C_DMA, LPI2C3_RECEIVE_DMA_CHANNEL);
	    /* Create the LPI2C master DMA driver handle */
	    LPI2C_MasterCreateEDMAHandle(LPI2C3,
	    		&g_m_edma_handle, &g_edmaRxHandle, &g_edmaTxHandle,
	    		dmaCompleteCallback, NULL);
	    // Set DMA TX/RX channels to this LPI2C's DMA functions and enable the DMA channels
	    DMAMUX_SetSource(LPI2C_DMA_MUX, LPI2C3_TRANSMIT_DMA_CHANNEL,
	                     LPI2C3_TRANSMIT_EDMA_REQUEST_SOURCE);
	    DMAMUX_EnableChannel(LPI2C_DMA_MUX, LPI2C3_TRANSMIT_DMA_CHANNEL);
	    DMAMUX_SetSource(LPI2C_DMA_MUX, LPI2C3_RECEIVE_DMA_CHANNEL, LPI2C3_RECEIVE_EDMA_REQUEST_SOURCE);
	    DMAMUX_EnableChannel(LPI2C_DMA_MUX, LPI2C3_RECEIVE_DMA_CHANNEL);
	    // Set interrupt priority for DMA interrupts
	    NVIC_SetPriority((DMA0_DMA16_IRQn + (LPI2C3_TRANSMIT_DMA_CHANNEL%16)), LPI2C3_DMA_IRQ_PRIORITY); // DMA0 shares IRQ with DMA16, etc.
	    NVIC_SetPriority((DMA0_DMA16_IRQn + (LPI2C3_RECEIVE_DMA_CHANNEL %16)), LPI2C3_DMA_IRQ_PRIORITY);
			// created in the 'empty' state; semaphore must first be given before it can subsequently be taken
			xResponseDataReadySemaphore = xSemaphoreCreateBinaryStatic(&xResponseDataReadySemaphoreBuffer );
	#endif // NXP_LPI2C_USE_DMA

	NXP_I2C_initialized = true;
};

#ifdef NXP_LPI2C_USE_DMA // This is a non-blocking implementation using DMA for I2C TX and RX

// DMA transfer has completed (same ISR callback used for TX and RX completion)
static void dmaCompleteCallback(LPI2C_Type *base,
		lpi2c_master_edma_handle_t *handle,
		status_t completionStatus,
		void *userData)
{
	(void)base;
	(void)handle;
	(void)completionStatus;
	(void)userData;
	if(txPending) {
		// On completing TX to DS2485, nothing to do, because we must finish waiting for DS2485 (to finish executing 1-Wire script)...
		txPending = false;
	} else {
		assert(rxPending);
		rxPending = false;
		// unblock DS2485_ExecuteCommand, which is waiting for this response data
		xSemaphoreGiveFromISR( xResponseDataReadySemaphore, NULL );
	}
}

int DS2485_ExecuteCommand(const uint8_t *packet, int packetSize, int delay_uSec, uint8_t *response, int responseSize)
{
	// Setup the I2C master
	if(!NXP_I2C_initialized) NXP_I2C_init();
	// ToDo 1-Wire: Every-command re-initialization? Maxim code does I2C shutdown and (re-) initialization EVERY COMMAND.
	// Might be advisable in case I2C bus gets into weird lock-up state, which never happens, right?

	// Copy transmit data into local buffer (need guaranteed non-cacheable memory)
	assert(packetSize<=(int)sizeof(i2c_DMA_buf));
	if(packetSize>(int)sizeof(i2c_DMA_buf)) return 1;
	memcpy(i2c_DMA_buf, packet, packetSize);
	if(maxUsedBufferSize<packetSize) maxUsedBufferSize=packetSize;

	// ====  I2C write to slave DS2485  ====
	lpi2c_master_transfer_t mt_TX = {
		.flags=kLPI2C_TransferDefaultFlag, /*!< Bit mask of options for the transfer. See enumeration #_lpi2c_master_transfer_flags for
												available options. Set to 0 or #kLPI2C_TransferDefaultFlag for normal transfers. */
		.slaveAddress=DS2485_I2C_7BIT_ADDRESS, /*!< The 7-bit slave address. */
		.direction=kLPI2C_Write, /*!< Either #kLPI2C_Read or #kLPI2C_Write. */
		.subaddress=0,		   /*!< Sub address. Transferred MSB first. */
		.subaddressSize=0,	   /*!< Length of sub address to send in bytes. Maximum size is 4 bytes. */
		.data=i2c_DMA_buf,     /*!< Pointer to data to transfer. */
		.dataSize=packetSize,  /*!< Number of bytes to transfer. */
	};
	txPending = true; // cleared asynchronously in DMA completion callback
	status_t reVal = LPI2C_MasterTransferEDMA(LPI2C3, &g_m_edma_handle, &mt_TX);
	assert(reVal == kStatus_Success); // 900 is kStatus_LPI2C_Busy; driver fails to resolve hang w/out powercycle?

	// Wait specified time for command to complete, could be a while...
	TickType_t ticksDelay = pdUS_TO_TICKS(delay_uSec);
	vTaskDelay(ticksDelay);
	assert(txPending == false); // should have been cleared a long time ago now

	// ====  I2C read from slave DS2485  ====
	lpi2c_master_transfer_t mt_RX = {
		.flags=kLPI2C_TransferDefaultFlag, /*!< Bit mask of options for the transfer. See enumeration #_lpi2c_master_transfer_flags for
												available options. Set to 0 or #kLPI2C_TransferDefaultFlag for normal transfers. */
		.slaveAddress=DS2485_I2C_7BIT_ADDRESS, /*!< The 7-bit slave address. */
		.direction=kLPI2C_Read,/*!< Either #kLPI2C_Read or #kLPI2C_Write. */
		.subaddress=0,		   /*!< Sub address. Transferred MSB first. */
		.subaddressSize=0,	   /*!< Length of sub address to send in bytes. Maximum size is 4 bytes. */
		.data=i2c_DMA_buf,     /*!< Pointer to data to transfer. */
		.dataSize=responseSize,/*!< Number of bytes to transfer. */
	};
	rxPending = true; // cleared asynchronously in DMA completion callback
	reVal = LPI2C_MasterTransferEDMA(LPI2C3, &g_m_edma_handle, &mt_RX);
	assert(reVal == kStatus_Success); // 900 is kStatus_LPI2C_Busy; driver fails to resolve hang w/out powercycle?

	// wait for DMA read transfer to complete
	xSemaphoreTake( xResponseDataReadySemaphore, portMAX_DELAY );
	memcpy(response, i2c_DMA_buf, responseSize);

	return 0;
}

#else // This is a blocking implementation with polling.
// LPI2C_MasterTransferBlocking sits in a loop polling I2C FIFO to push out data, as does LPI2C_MasterReceive
// CPU pig! Other tasks could be getting work done!
int DS2485_ExecuteCommand(const uint8_t *packet, int packetSize, int delay_uSec, uint8_t *response, int responseSize)
{
	// Setup the I2C master
	if(!NXP_I2C_initialized) NXP_I2C_init();
	// ToDo 1-Wire: Every-command re-initialization? Maxim code does I2C shutdown and (re-) initialization EVERY COMMAND.
	// Might be advisable in case I2C bus gets into weird lock-up state, which never happens, right?

	// ====  I2C write to slave DS2485  ====
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wcast-qual" // yick, see comment below...
	lpi2c_master_transfer_t mt = {
		.flags=kLPI2C_TransferDefaultFlag, /*!< Bit mask of options for the transfer. See enumeration #_lpi2c_master_transfer_flags for
												available options. Set to 0 or #kLPI2C_TransferDefaultFlag for normal transfers. */
		.slaveAddress=DS2485_I2C_7BIT_ADDRESS, /*!< The 7-bit slave address. */
		.direction=kLPI2C_Write, /*!< Either #kLPI2C_Read or #kLPI2C_Write. */
		.subaddress=0,		   /*!< Sub address. Transferred MSB first. */
		.subaddressSize=0,	   /*!< Length of sub address to send in bytes. Maximum size is 4 bytes. */
		.data=(void *)packet,  /*!< Pointer to data to transfer. Need to cast away const because FSL API uses this structure for read as well (then .data points to writable memory). */
		.dataSize=packetSize,  /*!< Number of bytes to transfer. */
	};
  #pragma GCC diagnostic pop
	status_t reVal = LPI2C_MasterTransferBlocking(LPI2C3, &mt); // blocks waiting for send completion...
	assert(reVal == kStatus_Success); // 900 is kStatus_LPI2C_Busy; driver fails to resolve hang w/out powercycle?

	// Wait specified time for command to complete, could be a long time...
	TickType_t ticksDelay = pdUS_TO_TICKS(delay_uSec);
	vTaskDelay(ticksDelay);

	// ====  I2C read from slave DS2485  ====
	// Read out Length Byte
	reVal = LPI2C_MasterStart(LPI2C3, DS2485_I2C_7BIT_ADDRESS, kLPI2C_Read);
	assert(reVal == kStatus_Success);
	reVal = LPI2C_MasterReceive(LPI2C3, response, responseSize);
	assert(reVal == kStatus_Success);

	return 0;
}
#endif
