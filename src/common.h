#ifndef COMMON_H
#define COMMON_H
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

/*
 * Configure the hardware as necessary to run this demo.
 */
static void prvSetupHardware( void );
/*
 * main_blinky() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1.
 * main_full() is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
extern void main_blinky( void );

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationTickHook( void );

#endif
