/*
 * Program for multi-sensor interface and display on 1.8 inch TFT with MiniZed
 * using the PL defined Pmod, GPIO and SPI IP blocks
 * More details and full source file at https://github.com/NavadeepGaneshU/Path-to-Programmable-III_MiniZed
 *
 * TFT 	| MiniZed
 * --------------
 * Vcc 	| 5V
 * GND 	| GND
 * CS  	| IO_10
 * RESET| IO_8
 * A0	| IO_9
 * SDA	| IO_11
 * SCK	| IO_13
 * LED	| 3.3V
 *
 * HTU21D pin maps to SDA and SCL of the MiniZed arduino header pins
 * Pmod NAV at Pmod 1
 *
 */

/***************** Includes *****************/
#include <stdio.h>
#include <sleep.h>
#include <time.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "delay/Delay.h"
#include "SPI.h"
#include "htu21d.h"
#include "lcd/LCD_Driver.h"
#include "lcd/LCD_GUI.h"
#include "htu21d.h"

/***************** User Definitions *****************/
#define BACKGROUND  WHITE
#define FOREGROUND BLUE
#define DELAY 1000

/***************** User Peripheral Instances *****************/
extern XGpio gpio1;
extern XSpi  SpiInstance;
extern const unsigned char font[] ;

void htu21d_main_menu(void);

int main()
{
    int Status;
	float temperature;
	float relative_humidity;

	htu21d_status stat;

	char tempbuf[16] = {};
	char humibuf[16] = {};

    init_platform();	    //Initialize the UART

	/* Initialize the GPIO driver */
	Status = XGpio_Initialize(&gpio1, XPAR_AXI_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("GPIO Initialization Failed!\r\n");
		return XST_FAILURE;
	}

	/* Set up the AXI SPI Controller */
	Status = XSpi_Init(&SpiInstance,SPI_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("SPI Initialization Failed!\r\n");
		return XST_FAILURE;
	}

    /* Set the AXI address of the IIC core */
    htu21d_init(XPAR_AXI_IIC_0_BASEADDR);

    /* TFT LCD initialization */
    LCD_SCAN_DIR LCD_ScanDir = SCAN_DIR_DFT;	//SCAN_DIR_DFT = D2U_L2R
	LCD_Init(LCD_ScanDir );

	stat = htu21d_set_resolution(htu21d_resolution_t_14b_rh_12b);		// Set resolution to 12-bit RH and 14-bit Temp

    while(1)
    {
		 stat = htu21d_read_temperature_and_relative_humidity(&temperature, &relative_humidity);
		 if(stat==htu21d_status_ok){
			 //printf("Temperature : %5.2f%cC, \tRelative Humidity : %4.1f%%",temperature,248,relative_humidity);
		 }else if(stat==htu21d_status_i2c_transfer_error){
			 printf("Transfer Error.");
		 }else if(stat==htu21d_status_crc_error){
			printf("CRC Error.");
		 }

		LCD_Clear(GUI_BACKGROUND);

		NavDemo_Initialize();
		NavDemo_Run();
	    NavDemo_Cleanup();

		GUI_DisString_EN(10,10,"Temp:",&Font12,GUI_BACKGROUND,CYAN);
		GUI_DisString_EN(10,25,"Humi:",&Font12,GUI_BACKGROUND,CYAN);

		sprintf(tempbuf, "%2.2f degC ", temperature);
		sprintf(humibuf, "%2.2f %%", relative_humidity);

		GUI_DisString_EN(55,10,tempbuf,&Font12,GUI_BACKGROUND,YELLOW);
		GUI_DisString_EN(55,25,humibuf,&Font12,GUI_BACKGROUND,YELLOW);

		//printf("HTU21D: %2.2f,%2.2f\r\n",temperature,relative_humidity);	//add all sensor values here

		delay_ms(1000);
    }
    return 0;
}

