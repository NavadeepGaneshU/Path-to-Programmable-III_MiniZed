/**
* Creation Date: 12-07-2023
* @file interrupt_sw.c
*
* This file contains a design example using the Interrupt Controller driver
* (XScuGic) and hardware device. Interfaced is a pushbutton and RED, GREEN LEDs via GPIO pins
* and while the button is pressed, a software interrupt is generated which is then handled by the
* service routine function.
*
/***************************** Include Files *********************************/

#include <stdio.h>
#include <stdlib.h>
#include "xil_io.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xscugic.h"
#include "xgpiops.h"
#include "xplatform_info.h"
#include <xil_printf.h>

/************************** Constant Definitions *****************************/

#define INTC_DEVICE_ID		XPAR_SCUGIC_0_DEVICE_ID
#define INTC_DEVICE_INT_ID	0x0E

#define GPIO_DEVICE_ID		0
#define INTC_DEVICE_ID		0
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR

/* The following constants define the GPIO banks that are used. */
#define GPIO_BANK	XGPIOPS_BANK0  /* Push Button Connected at Bank 0 of the GPIO Device */

#define LED_DELAY		10000000

/************************** Function Prototypes ******************************/
int ScuGicExample(u16 DeviceId);
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr);
void ISRHandle(void *CallbackRef);
void readSwitch(void);

/************************** Variable Definitions *****************************/
static XGpioPs Gpio; 				/* The Instance of the GPIO Driver */

XScuGic InterruptController; 	     /* Instance of the Interrupt Controller */
static XScuGic_Config *GicConfig;    /* The configuration parameters of the
                                       controller */
static u32 Output_Pin_R = 52u,Output_Pin_G = 53u; /* LEDs Pin Positions for Minized Board */
static u32 Input_Pin = 0u;						  /* Switch button */


/*
 * Create a shared variable to be used by the main thread of processing and
 * the interrupt processing
 */
volatile static int InterruptProcessed = FALSE;

/*****************************************************************************/
/**
*
* This is the main function for the Interrupt Controller example.
*
* @param	None.
*
* @return	XST_SUCCESS to indicate success, otherwise XST_FAILURE.
*
* @note		None.
*
****************************************************************************/
int main(void)
{
	int Status;

	xil_printf("/**********GIC Software Interrupt Example Test**********/\r\n");

	/*
	 *  Run the GIC software example, specify the Device ID
	 */
	Status = ScuGicExample(INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("GIC Example Test Failed\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is an example of how to use the interrupt controller driver
* (XScuGic) and the hardware device. It also initializes the GPIO peripherals
*
* @param	DeviceId is Device ID of the Interrupt Controller Device,
*		typically XPAR_<INTC_instance>_DEVICE_ID value from
*		xparameters.h
*
* @return	XST_SUCCESS to indicate success, otherwise XST_FAILURE
*
* @note		None.
*
******************************************************************************/
int ScuGicExample(u16 DeviceId)
{
	int Status;
	XGpioPs_Config *ConfigPtr;	//stores the Configuration Settings of PS GPIO

	ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);			  //Load the Configuration settings required for PS GPIO_0

	XGpioPs_CfgInitialize(&Gpio, ConfigPtr, ConfigPtr->BaseAddr); //Create PS GPIO device using settings from "ConfigPtr" and

	/* Set the direction for the PS Push Button pin to be input */
	XGpioPs_SetDirectionPin(&Gpio, Input_Pin, 0x0);				  //0x0 for input direction

	/* Set the direction for the Red LED pin to be output. */
	XGpioPs_SetDirectionPin(&Gpio, Output_Pin_R, 1u);
	XGpioPs_SetOutputEnablePin(&Gpio, Output_Pin_R, 1u);
	XGpioPs_WritePin(&Gpio, Output_Pin_R, 0x0);

	/* Set the direction for the Green LED pin to be output. */
	XGpioPs_SetDirectionPin(&Gpio, Output_Pin_G, 1u);
	XGpioPs_SetOutputEnablePin(&Gpio, Output_Pin_G, 1u);
	XGpioPs_WritePin(&Gpio, Output_Pin_G, 0x0);

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	GicConfig = XScuGic_LookupConfig(DeviceId);
	if (NULL == GicConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
					GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built
	 * correctly
	 */
	Status = XScuGic_SelfTest(&InterruptController);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Setup the Interrupt System
	 */
	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the ISR handle performs
	 * the specific interrupt processing for the device
	 */
	Status = XScuGic_Connect(&InterruptController, INTC_DEVICE_INT_ID,
			   (Xil_ExceptionHandler)ISRHandle,
			   (void *)&InterruptController);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupt for the device and then cause (simulate) an
	 * interrupt so the handlers will be called
	 */
	XScuGic_Enable(&InterruptController, INTC_DEVICE_INT_ID);

	/*This function polls the GPIO pin state connected to a pushbutton
	 * for raising a software interrupt*/
	void readSwitch(void)
	{
		if(1 == XGpioPs_ReadPin(&Gpio, Input_Pin))
		{
			xil_printf("Button interrupt detected!\r\n");

			/*Simulate the Interrupt*/
			Status = XScuGic_SoftwareIntr(&InterruptController,
							INTC_DEVICE_INT_ID,
							XSCUGIC_SPI_CPU0_MASK);

			if (Status != XST_SUCCESS) {
				return XST_FAILURE;
			}
		}
	}

	/*
	 * Wait for the interrupt to be processed, if the interrupt does not
	 * occur, this loop will be running forever
	 */
	u32 delay;
	u32 i;
	delay = LED_DELAY;

	xil_printf("Core task running...\r\n");

	while (1)
	{
		XGpioPs_WritePin(&Gpio, Output_Pin_G, 0x0);
		readSwitch();		//poll for the button status

		for (i = 0 ; i < delay; i++);
		XGpioPs_WritePin(&Gpio, Output_Pin_R, 0x0);
		for (i = 0 ; i < delay; i++);
		XGpioPs_WritePin(&Gpio, Output_Pin_R, 0x1);

		/*
		 * If the interrupt occurred which is indicated by the global
		 * variable which is set in the ISR handle, then
		 * the service routine runs and let this loop continue(or break whichever)
		 */
		if (InterruptProcessed) {
			continue;
		}
	}

	return XST_SUCCESS;
}

/******************************************************************************/
/**
*
* This function connects the interrupt handler of the interrupt controller to
* the processor.
*
* @param	XScuGicInstancePtr is the instance of the interrupt controller
*		that needs to be worked on.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the ARM processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler,
			XScuGicInstancePtr);

	/*
	 * Enable interrupts in the ARM
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/******************************************************************************/
/**
*
* This function is designed to look like an interrupt handler in a device
* driver. This is typically a 2nd level handler that is called from the
* interrupt controller interrupt handler.  This handler would typically
* perform device specific processing such as reading and writing the registers
* of the device to clear the interrupt condition and pass any data to an
* application using the device driver.
*
* @param	CallbackRef is passed back to the device driver's interrupt
*		handler by the XScuGic driver.  It was given to the XScuGic
*		driver in the XScuGic_Connect() function call.  It is typically
*		a pointer to the device driver instance variable.
*		In this example, we do not care about the callback
*		reference, so we passed it a 0 when connecting the handler to
*		the XScuGic driver and we make no use of it here.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void ISRHandle(void *CallbackRef)
{
	XGpioPs_WritePin(&Gpio, Output_Pin_G, 0x1);
//	sleep(1);
//	XGpioPs_WritePin(&Gpio, Output_Pin_G, 0x0);

	/*
	 * Indicate the interrupt has been processed using a shared variable
	 */
	InterruptProcessed = TRUE;
	xil_printf("Successfully cleared the interrupt\r\n\n");
}

