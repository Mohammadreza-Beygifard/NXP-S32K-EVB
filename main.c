/********************************************************************************************************
*
*
*										Filename:  main.c
*									    Project:   OSES_Final_assignment
*										A.Y:       2019-2020
*
*										Student:   Stefania Gabutto, 265481
*											       Mohammadreza Beygifard, 257645
*
********************************************************************************************************/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>
#include  <bsp_os.h>
#include  <bsp_clk.h>
#include  <bsp_int.h>
#include "S32K144.h"

#include  "os_app_hooks.h"
#include  "../app_cfg.h"

/************************************ Project Specific Includes *****************************************/
#include  <bsp_flextimerled.h>
#include  <bsp_adc.h>
#include  <bsp_switch.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB   StartupTaskTCB;
static  CPU_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];

/************************************ Project Specific Variables *****************************************/

extern volatile LEDCOLOR_VAR LED_COLOR;	    // storing current LED color
extern volatile CPU_INT16U ADC0_conv; 		// ADC conversion result, which is given by the interrupt handler
extern OS_SEM ADC0sem;						// ADC semaphore

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask(void* p_arg);


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

int  main(void)
{
	OS_ERR  os_err;


	BSP_ClkInit();                             /* Initialize the main clock.                           */
	BSP_IntInit();
	BSP_OS_TickInit();                         /* Initialize kernel tick timer                         */

	Mem_Init();                                /* Initialize Memory Managment Module                   */
	CPU_IntDis();                              /* Disable all Interrupts.                              */
	CPU_Init();                                /* Initialize the uC/CPU services                       */

	OSInit(&os_err);                           /* Initialize uC/OS-III                                 */
	if (os_err != OS_ERR_NONE) {
		while (1);
	}

	App_OS_SetAllHooks();                      /* Set all applications hooks                           */

	OSTaskCreate(&StartupTaskTCB,              /* Create the startup task                              */
		"Startup Task",
		StartupTask,
		0u,
		APP_CFG_STARTUP_TASK_PRIO,
		&StartupTaskStk[0u],
		StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE / 10u],
		APP_CFG_STARTUP_TASK_STK_SIZE,
		0u,
		0u,
		0u,
		(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		&os_err);
	if (os_err != OS_ERR_NONE) {
		while (1);
	}

	OSStart(&os_err);                          /* Start multitasking (i.e. give control to uC/OS-III)  */

	while (DEF_ON) {                           /* Should Never Get Here.                               */
		;
	}
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*********************************************************************************************************
*/

static  void  StartupTask(void* p_arg)
{
	OS_ERR      os_err;
	CPU_INT16U comparator = 0;

	(void)p_arg;

	OS_TRACE_INIT();                           	/* Initialize Trace recorder                            */

	BSP_OS_TickEnable();                       	/* Enable the tick timer and interrupt                  */

#if OS_CFG_STAT_TASK_EN > 0u
	OSStatTaskCPUUsageInit(&os_err);           	/* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
	CPU_IntDisMeasMaxCurReset();
#endif

	

	OSSemCreate(&ADC0sem, "ADC0 Sempahore", 0u, &os_err);	//  semaphore declration

	BSP_FTM0_PWM_Init(); 						// Initialize FTM0 for PTD15,16,0
	BSP_ADC0_Init();							// Initialize ADC0
	BSP_Switch_Init();							// Initialize SWitches of PTC12 and 13

	while (DEF_TRUE) { // Main loop
		/* Start conversion of ADC0 channel 12 (potentiometer) */
		BSP_ADC0_convertAdcChan_interrupt(12);
		OSSemPend(&ADC0sem, 0u, OS_OPT_PEND_BLOCKING, 0u, &os_err); // Pend on ADC0sem

		comparator = ADC0_conv;

		BSP_FTM0_ChangeDutyCycle(comparator);
	}
}
