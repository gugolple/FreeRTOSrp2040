#include "hd44780.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Library includes. */
#include <stdio.h>
#include "hardware/gpio.h"

/* RP2040 Specifics */
#include "hardware/timer.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

// Hardware configuration of unit
#define HD44780_CONFIG_DL_DATA_LENGTH   0 //0 - 4 | 1 - 8 //Bits for comm
#define HD44780_CONFIG_N_DISPLAY_LINES  1 //0 - 1 | 1 - 2
#define HD44780_CONFIG_F_CHARACTER_FONT 0 
#define HD44780_CONFIG_C_CURSOR         0 // Cursor visible
#define HD44780_CONFIG_B_CURSOR_BLINK   0 // Cursor blinking

// Macro definition checks
// HD44780_CONFIG_N_DISPLAY_LINES
#if HD44780_CONFIG_N_DISPLAY_LINES != 0 && HD44780_CONFIG_N_DISPLAY_LINES != 1
#error
#error INVALID HD44780_CONFIG_N_DISPLAY_LINES MUST BE EITHER 0 or 1
#endif

// HD44780_CONFIG_F_CHARACTER_FONT
#if HD44780_CONFIG_F_CHARACTER_FONT != 0 && HD44780_CONFIG_F_CHARACTER_FONT != 1
#error
#error INVALID HD44780_CONFIG_F_CHARACTER_FONT MUST BE EITHER 0 or 1
#endif

// HD44780_CONFIG_C_CURSOR
#if HD44780_CONFIG_C_CURSOR != 0 && HD44780_CONFIG_C_CURSOR != 1
#error
#error INVALID HD44780_CONFIG_C_CURSOR MUST BE EITHER 0 or 1
#endif

// HD44780_CONFIG_B_CURSOR_BLINK
#if HD44780_CONFIG_B_CURSOR_BLINK != 0 && HD44780_CONFIG_B_CURSOR_BLINK != 1
#error
#error INVALID HD44780_CONFIG_B_CURSOR_BLINK MUST BE EITHER 0 or 1
#endif

// Hardware restrictions of official spec
#define MAX_HD44780_FREQ              ( 250000 ) //Herth
#define MIN_HD44780_PERIOD_US         ( 1000000/MAX_HD44780_FREQ )
#define hd44780_POWERON_DELAY_MS      ( 100 / portTICK_PERIOD_MS )
#define hd44780_INST_CLEAR_DISPLAY_MS ( 10 / portTICK_PERIOD_MS )
#define hd44780_INST_DELAY_US         80

#define HD44780_START_ADD_L1          (0x00) 
#define HD44780_START_ADD_L2          (0x40)
#define HD44780_START_ADD_L3          (0x10) 
#define HD44780_START_ADD_L4          (0x50)
const int HD44780_LINE_START_LOC[] = {
    HD44780_START_ADD_L1,
    HD44780_START_ADD_L2,
    HD44780_START_ADD_L3,
    HD44780_START_ADD_L4
};

// Pins assigned for the HD44780 interface
// Only 4 high used if set to 4 bit mode at HD44780_MODE
#if HD44780_CONFIG_DL_DATA_LENGTH == 1
#define HD44780_MODE 8
const int HD44780_PINS_DATA[HD44780_MODE] = {1,2,3,4,5,6,7,8};
#elif HD44780_CONFIG_DL_DATA_LENGTH == 0
#define HD44780_MODE 4
const int HD44780_PINS_DATA[HD44780_MODE] = {5,6,7,8};
#else
#error INVALID HD44780_CONFIG_DL_DATA_LENGTH MUST BE EITHER 0 or 1
#endif
const int HD44780_PINS_RW   = 9;
const int HD44780_PINS_RS   = 10;
const int HD44780_PINS_E    = 11;
const int HD44780_PINS_DBG  = 12;
const int HD44780_PIN_COUNT = HD44780_MODE;

#define NROW 4
#define ROWLEN 17
#define ROWLENCP (ROWLEN-1)
char hd44780_display_data[NROW][ROWLEN] = {
    "",
    "",
    "",
    "",
};

int get_high_4bits(const int v) {
    return (v & 0xF0) >> 4;
}

int get_low_4bits(const int v) {
    return v & 0x0F;
}

int valPos(int pos, int vals) {
    // Force to be 1 or 0
    return (vals & (1u << pos)) ? 1 : 0;
}

void initialize_pins() {
    gpio_init( HD44780_PINS_RW );
    gpio_init( HD44780_PINS_RS );
    gpio_init( HD44780_PINS_E );
    gpio_init( HD44780_PINS_DBG );
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
void hd44780_inst_set_data_pins(const int v) {
    for(int i=0; i<HD44780_PIN_COUNT; i++){
        gpio_put(HD44780_PINS_DATA[i], v&(1<<i));
    }
}

void hd44780_send_data(const int v) {
    gpio_put( HD44780_PINS_E, 1 );
    hd44780_inst_set_data_pins(v);
    busy_wait_us(hd44780_INST_DELAY_US);
    gpio_put( HD44780_PINS_E, 0 );
    busy_wait_us(hd44780_INST_DELAY_US);
}

void hd44780_send_payload(const int v) {
#if HD44780_CONFIG_DL_DATA_LENGTH == 1
    hd44780_send_data(v);
#elif HD44780_CONFIG_DL_DATA_LENGTH == 0
    hd44780_send_data(get_high_4bits(v));
    hd44780_send_data(get_low_4bits(v));
#endif
}

void hd44780_send_instruction(const int v) {
    gpio_put( HD44780_PINS_RS, 0 );
    hd44780_send_payload(v);
}

void hd44780_send_data_payload(const int v) {
    gpio_put( HD44780_PINS_RS, 1 );
    hd44780_send_payload(v);
}

void hd44780_inst_display_clear(TickType_t *xNextWakeTime) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 0
    // - DB4: 0
    // - DB3: 0
    // - DB2: 0
    // - DB1: 0
    // - DB0: 1
    const int val = 0x01;
    hd44780_send_instruction(val);
    vTaskDelayUntil( xNextWakeTime, hd44780_INST_CLEAR_DISPLAY_MS );
}

void hd44780_inst_return_home(TickType_t *xNextWakeTime) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 0
    // - DB4: 0
    // - DB3: 0
    // - DB2: 0
    // - DB1: 1
    // - DB0: Ignored
    const int val = 0x02;
    hd44780_send_instruction(val);
    vTaskDelayUntil( xNextWakeTime, hd44780_INST_CLEAR_DISPLAY_MS );
}

#define HD44780_CONFIG_ID_INCREMENT_DIRECTION 1 // 1 - Right | 0 - Left
#define HD44780_CONFIG_SHIFT_CURSOR 1 // 0 off | 1 on 
void hd44780_inst_entry_mode_set(const int id, const int s) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 0
    // - DB4: 0
    // - DB3: 0
    // - DB2: 1
    // - DB1: ID // Cursor direction // 1 - Right | 0 - Left
    // - DB0: S  // Shift after read/write // 0 off | 1 on
    const int val = 0x04 |
        (id & 0x01) << 1 |
        (s & 0x01)
    ;
    hd44780_send_instruction(val);
}

void hd44780_inst_display_control(const int d, const int c, const int b) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 0
    // - DB4: 0
    // - DB3: 1
    // - DB2: D // Display on/off
    // - DB1: C // Cursor on/off
    // - DB0: B // Blink cursor on/off
    const int val = 0x08 |
        (d &0x01) << 2 |
        (c &0x01) << 1 |
        (b &0x01) 
    ;
    hd44780_send_instruction(val);
}

void hd44780_inst_cursor_display_shift(const int sc, const int rl) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 0
    // - DB4: 1
    // - DB3: SC
    // - DB2: RL
    // - DB1: Ignored 
    // - DB0: Ignored
    // SC - RL Table:
    //  0    0 Shift cursor to the left (AC-1)
    //  0    1 Shift cursor to the right (AC+1)
    //  1    0 Shift display to the left, cursor follows
    //  1    1 Shift display to the right, cursor follows
    
    // Start with the instruction value
    const int val = 0x10 |
        sc << 3 | 
        rl << 2
    ;
    hd44780_send_instruction(val);
}

// This specific call is to transition from 8 bit to 4 bit mode
void hd44780_inst_function_set_half() {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 1
    // - DB4: DL
    // - DB3: N
    // - DB2: F
    // - DB1: Ignored
    // - DB0: Ignored
    
    // Start with the instruction value
    const int val = 0x20 |
        HD44780_CONFIG_DL_DATA_LENGTH << 4 | 
        HD44780_CONFIG_N_DISPLAY_LINES << 3 |
        HD44780_CONFIG_F_CHARACTER_FONT << 2
    ;
    hd44780_send_data(get_high_4bits(val));
}

void hd44780_inst_function_set() {
    // Per instructions:
    // - DB7: 0
    // - DB6: 0
    // - DB5: 1
    // - DB4: DL
    // - DB3: N
    // - DB2: F
    // - DB1: Ignored
    // - DB0: Ignored
    
    // Start with the instruction value
    const int val = 0x20 |
        HD44780_CONFIG_DL_DATA_LENGTH << 4 | 
        HD44780_CONFIG_N_DISPLAY_LINES << 3 |
        HD44780_CONFIG_F_CHARACTER_FONT << 2
    ;
    hd44780_send_instruction(val);
}

void hd44780_inst_set_cgram_address(const int address) {
    // Per instructions:
    // - DB7: 0
    // - DB6: 1
    // - DB5: Add
    // - DB4: Add
    // - DB3: Add
    // - DB2: Add
    // - DB1: Add
    // - DB0: Add
    
    // Start with the instruction value
    const int val = 0x40 |
        (address & 0x3F)
    ;
    hd44780_send_instruction(val);
}

void hd44780_inst_set_ddram_address(const int address) {
    // Per instructions:
    // - DB7: 1
    // - DB6: Add
    // - DB5: Add
    // - DB4: Add
    // - DB3: Add
    // - DB2: Add
    // - DB1: Add
    // - DB0: Add
    
    // Start with the instruction value
    const int val = 0x80 |
        (address & 0x7F)
    ;
    hd44780_send_instruction(val);
}

void initialize() {
    // Initialize pins
    initialize_pins();
    // Set direction of control pins
    gpio_set_dir( HD44780_PINS_RW, GPIO_OUT );
    gpio_set_dir( HD44780_PINS_RS, GPIO_OUT );
    gpio_set_dir( HD44780_PINS_E , GPIO_OUT );
    gpio_set_dir( HD44780_PINS_DBG , GPIO_OUT );
    // Set direction of pins for default
    set_datapins_output();

    // Set initial values
    hd44780_inst_set_data_pins(0);
    gpio_put( HD44780_PINS_RW, 0 );
    gpio_put( HD44780_PINS_RS, 0 );
    gpio_put( HD44780_PINS_E, 0 );
    gpio_put( HD44780_PINS_DBG, 1 );
}

void reset_sequence(TickType_t *xNextWakeTime) {
    // Fully wait until initialization is compleated
    vTaskDelayUntil( xNextWakeTime, hd44780_POWERON_DELAY_MS );
    // Instruction to archieve the correct initialization
#if HD44780_CONFIG_DL_DATA_LENGTH == 0
    hd44780_inst_function_set_half();
    vTaskDelayUntil( xNextWakeTime, hd44780_INST_CLEAR_DISPLAY_MS );
#endif
    hd44780_inst_function_set();
    vTaskDelayUntil( xNextWakeTime, hd44780_INST_CLEAR_DISPLAY_MS );
    hd44780_inst_display_clear(xNextWakeTime);
    hd44780_inst_display_control(1, 1, 0);
    hd44780_inst_entry_mode_set(1,0);

    // Set start point
    hd44780_inst_set_ddram_address(HD44780_START_ADD_L4);
}

// This only exists to confirm that the loop is never stuck
static int blk = 0;
void blink_dbg() {
    gpio_put(HD44780_PINS_DBG, blk);
    blk = !blk;
}

int check_line_not_reachable(int const l) {
    if(l < 0 || l > NROW) { return 1; }
    return 0;
}

void write_string(char const * const rstr) {
    char const * str = rstr;
    for(int i=0 ; i<ROWLENCP && str[i] != '\0' ; i++) {
        hd44780_send_data_payload(str[i]);
    }
}

void set_line(int line, char* str) {
    // Verify reachable line
    if(check_line_not_reachable(line)) { return; }
    char* ddl = hd44780_display_data[line];
    // Only copy allowed range of data
    int i;
    for(i=0 ; i<ROWLENCP && str[i] != '\0' ; i++) {
        ddl[i] = str[i];
    }
    // ddl[i] will always be in range due to checks in
    // the for loop
    ddl[i] = '\0';
}

void display_line(int line) {
    if(check_line_not_reachable(line)) { return; }
    hd44780_inst_set_ddram_address(HD44780_LINE_START_LOC[line]);
    write_string(hd44780_display_data[line]);
}

/*-----------------------------------------------------------*/
/* Logic of operation is to check every 100ms or wait until change event
 * and then run the full logic to realize the task
 */
#define hd44780_CHECK_FREQUENCY_MS            ( 1000 / portTICK_PERIOD_MS )
void hd44780Task( void *pvParameters )
{
    // Vars
    TickType_t xNextWakeTime;

    /* Remove compiler warning about unused parameter. */
    ( void ) pvParameters;
    xNextWakeTime = xTaskGetTickCount();

    // Initialize internal configurations related to HD44780 specifics
    initialize();

    // Realize the reset sequence to initialize the HD44780
    reset_sequence(&xNextWakeTime);

    set_line(0, "L1 Me gusta");
    display_line(0);
    set_line(1, "L2 Funciona?");
    display_line(1);
    set_line(2, "L3 No me lo creo");
    display_line(2);
    set_line(3, "L4 A la primera?");
    display_line(3);

    // Test display
    int cnt = 0;
    char buf[ROWLEN] = "";
    for( ;; )
    {
        blink_dbg();
        snprintf(buf, sizeof(buf), "%d", cnt++);
        set_line(3, buf);
        display_line(3);
        //vTaskDelayUntil( &xNextWakeTime, hd44780_CHECK_FREQUENCY_MS );
    }
}
/*-----------------------------------------------------------*/
