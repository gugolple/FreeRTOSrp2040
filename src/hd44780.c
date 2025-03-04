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
const int HD44780_PINS_DATA[] = {1,2,3,4,5,6,7,8};
const int HD44780_PINS_RW = 9;
const int HD44780_PINS_RS = 10;
const int HD44780_PINS_E  = 11;
const int HD44780_PIN_COUNT = ARRAY_SIZE(HD44780_PINS_DATA);

int valPos(int pos, int vals) {
    // Force to be 1 or 0
    return (vals & (1u << pos)) ? 1 : 0;
}

void initialize_pins() {
    gpio_init( HD44780_PINS_RW );
    gpio_init( HD44780_PINS_RS );
    gpio_init( HD44780_PINS_E );
    for(int i=0; i<HD44780_PIN_COUNT; i++) {
        gpio_init( HD44780_PINS_DATA[i] );
    }
}

void set_datapins_output() {
    for(int i=0; i<HD44780_PIN_COUNT; i++) {
        gpio_set_dir( HD44780_PINS_DATA[i], GPIO_OUT );
    }
}
/* 
 * All operations are:
 * - HD44780_PINS_E goes high
 * - HD44780_PINS_DATA gets set
 * - HD44780_PINS_E goes low, committing the instruction
 */

// This will write the value to the data pins
// bit 0 -> D0 ; bit 7 -> D7
void hd44780_set_data_pins(const int v) {
    for(int i=0; i<HD44780_PIN_COUNT; i++){
        gpio_put(HD44780_PINS_DATA[i], v&(1<<i));
    }
}

void clock_low() {
    gpio_put( HD44780_PINS_E, 0 );
}

#define hd44780_INST_CLEAR_DISPLAY_MS            ( 2 / portTICK_PERIOD_MS )
void hd44780_inst_display_clear() {
    const int val = 0x01;
    clock_low();
    hd44780_set_data_pins(val);
    vTaskDelayUntil( &xNextWakeTime, hd44780_INST_CLEAR_DISPLAY_MS );

}

#define hd44780_POWERON_DELAY_MS            ( 50 / portTICK_PERIOD_MS )
void initialize() {
    // Initialize pins
    initialize_pins();
    // Set direction of control pins
    gpio_set_dir( HD44780_PINS_RW, GPIO_OUT );
    gpio_set_dir( HD44780_PINS_RS, GPIO_OUT );
    gpio_set_dir( HD44780_PINS_E , GPIO_OUT );
    // Set direction of pins for default
    set_datapins_output();

    // Set initial values
    hd44780_set_data_pins(0);
    gpio_put( HD44780_PINS_RW, 0 );
    gpio_put( HD44780_PINS_RS, 0 );
    gpio_put( HD44780_PINS_E, 0 );
    vTaskDelayUntil( &xNextWakeTime, hd44780_POWERON_DELAY_MS );
}

void reset_sequence() {
    gpio_put(HD44780_PINS_RW, 0);
    gpio_put(HD44780_PINS_RS, 0);
    gpio_put(HD44780_PINS_E , 0);
    hd44780_inst_display_clear();
}

/*-----------------------------------------------------------*/
/* Logic of operation is to check every 100ms or wait until change event
 * and then run the full logic to realize the task
 */
#define hd44780QUEUE_CHECK_FREQUENCY_MS            ( 100 / portTICK_PERIOD_MS )
void hd44780Task( void *pvParameters )
{
const int MAX_VAL = 1<<HD44780_PIN_COUNT;
// Vars
TickType_t xNextWakeTime;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;
    xNextWakeTime = xTaskGetTickCount();

    // Initialize internal configurations related to HD44780 specifics
    initialize();

    // Realize the reset sequence to initialize the HD44780
    reset_sequence();

    for( ;; )
    {
        for(int current_value=0; current_value<MAX_VAL; current_value++) {
            hd44780_set_data_pins(current_value);
            vTaskDelayUntil( &xNextWakeTime, hd44780QUEUE_CHECK_FREQUENCY_MS );
        }
    }
}
/*-----------------------------------------------------------*/
