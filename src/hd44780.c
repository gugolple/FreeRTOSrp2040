#include "hd44780.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Library includes. */
#include <stdio.h>
#include "hardware/gpio.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Pins assigned for the HD44780 interface
const int HD44780_PINS[] = {1,2,3,4,5,6,7,8};
const int HD44780_PIN_COUNT = ARRAY_SIZE(HD44780_PINS);
const int MAX_PIN = 29;

/*-----------------------------------------------------------*/
#define hd44780QUEUE_CHECK_FREQUENCY_MS            ( 400 / portTICK_PERIOD_MS )
void hd44780Task( void *pvParameters )
{
// Vars
//int current_pin = 0;
TickType_t xNextWakeTime;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;
    xNextWakeTime = xTaskGetTickCount();

    for(int i=0; i<HD44780_PIN_COUNT; i++) {
        gpio_init( HD44780_PINS[i] );
        gpio_set_dir( HD44780_PINS[i], GPIO_OUT );
    }

    for( ;; )
    {
        /* Logic of operation is to check every 100ms or wait until change event
         * and then run the full logic to realize the task
         */
        for(int i=0; i<HD44780_PIN_COUNT; i++) {
            gpio_put(HD44780_PINS[i], 1);
        }
        vTaskDelayUntil( &xNextWakeTime, hd44780QUEUE_CHECK_FREQUENCY_MS );
        for(int i=0; i<HD44780_PIN_COUNT; i++) {
            gpio_put(HD44780_PINS[i], 0);
        }
        vTaskDelayUntil( &xNextWakeTime, hd44780QUEUE_CHECK_FREQUENCY_MS );
    }
}
/*-----------------------------------------------------------*/
