// Standard includes
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "uart.h"
#include "spi.h"
#include "interrupt.h"
#include "pin_mux_config.h"
#include "utils.h"
#include "prcm.h"
#include "hw_apps_rcm.h"
#include "gpio.h"
#include "hw_nvic.h"
#include "systick.h"
#include "timer.h"

// Common interface includes
#include "uart_if.h"
#include "gpio_if.h"
#include "i2c_if.h"

#include "pin_mux_config.h"
#include "Adafruit_GFX.h"
#include "simplelink.h"
#include "utils/network_utils.h"


//*****************************************************************************
//                          MACROS
//*****************************************************************************
//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                11    /* Current Date */
#define MONTH               3     /* Month 1-12 */
#define YEAR                2025  /* Current year */
#define HOUR                12    /* Time - hours */
#define MINUTE              18    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define APPLICATION_VERSION  "WQ25"
#define APP_NAME             "SSL + IR Decoding"
#define CONSOLE              UARTA0_BASE
#define SPI_IF_BIT_RATE      100000
#define SERVER_NAME          "52.88.252.80"
#define GOOGLE_DST_PORT      8443

#define POSTHEADER "POST /things/FinalThing/shadow HTTP/1.1\r\n"             // CHANGE ME
#define GETHEADER  "GET /things/FinalThing/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: ahvzuro29rftq-ats.iot.us-west-2.amazonaws.com\r\n"        // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION     "SQ25"
#define APP_NAME                "DUAL"
#define UART_PRINT              Report
#define FOREVER                 1
#define CONSOLE                 UARTA0_BASE
#define UART1                   UARTA1_BASE
#define FAILURE                 -1
#define SUCCESS                 0
#define SPI_IF_BIT_RATE         100000
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

//*****************************************************************************
//
// Player mode selector macro
//
// PLAYER_MODE = 1 : Player is RED
// PLAYER_MODE = 0 : Player is BLUE
//
//*****************************************************************************
#define PLAYER_MODE 0

// Color definitions
#define BLACK           0x0000
#define WHITE           0xFFFF
#define RED             0xF800
#define BLUE            0x001F
#define PINK            0xFA26

// some helpful macros for systick

// the cc3200's fixed clock frequency of 80 MHz
// note the use of ULL to indicate an unsigned long long constant
#define SYSCLKFREQ 80000000ULL

// systick reload value set to 40ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 3200000UL

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
// Flags
volatile unsigned char switch_intflag;     // flag set when switch is pressed
volatile unsigned char pin_out_intflag;    // flag set when IR input is received

// Projectile variables
volatile int projectile_size = 2;
volatile int projectile_scale = 1;
volatile int ammo_cnt = 6;
volatile int input_held_cnt = 0;

// Game state variables
volatile bool game_running = true;
volatile bool round_running = true;
volatile int timer = 0;
volatile int lRetVal;

// Score variables
volatile int my_score = 0;
volatile int opponent_score = 0;

// Multi-tap variables
volatile int systick_cnt = 0;
volatile int edge_counter = 0;
volatile int data = 0;
volatile bool reading_data = false;
volatile int prev_data = -1;

volatile uint32_t global_time = 0;          //
volatile uint32_t prev_detection = 0;       //
volatile uint32_t last_time_button_pressed = 0;

volatile int curr_cycle = -1;
volatile char curr_letter = 0;
volatile int x = 0;
char username[50] = "";
int l;
volatile bool name_set = false;
volatile bool caps_lock = false;

// Initialize projectile
#define MAX_PROJECTILES 6

typedef struct {
    volatile int  x_position;
    volatile int  y_position;
    volatile int  x_velocity;
    volatile int  y_velocity;
    volatile bool state;
    volatile int  size;
} Projectile;


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************

//*****************************************************************************
//
//! Display the buffer contents over I2C
//!
//! \param  pucDataBuf is the pointer to the data store to be displayed
//! \param  ucLen is the length of the data to be displayed
//!
//! \return none
//!
//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t      CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//****************************************************************************
//
//! Reads registers 0x2 - 0x5 corresponding to X/Y acceleration data
//! 
//! This function  
//!    1. Invokes the corresponding I2C APIs
//!
//! \return array containing X and Y byte data
//
//****************************************************************************
int8_t*
ReadAccData()
{
    unsigned char ucRegOffset = 2;
    unsigned char aucRdDataBuf[256];

    //
    // Write the register address to be read from.
    // Stop bit implicitly assumed to be 0.
    //
    RET_IF_ERR(I2C_IF_Write(24,&ucRegOffset,1,0));

    //
    // Read 4 bytes of successive data
    //
    RET_IF_ERR(I2C_IF_Read(24, &aucRdDataBuf[0], 4));

    //
    // Return the X and Y acceleration data
    //
    static int8_t data[2];
    data[0] = (int8_t)aucRdDataBuf[3];
    data[1] = (int8_t)aucRdDataBuf[1];

    return data;
}

//****************************************************************************
//
//! Read IR input and write to data
//!
//!
//! \return none
//
//****************************************************************************
static void GPIOA0IntHandler(void) {
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);        // clear interrupts on GPIOA0

    uint32_t pin_state = MAP_GPIOPinRead(GPIOA0_BASE, 0x40);
    if ((pin_state & 0x40) == 0) // Read falling edge
    {
        HWREG(NVIC_ST_CURRENT) = 1;
        systick_cnt = 0;

        // Initialize decoding variables on starting edge
        if (!reading_data)
        {
            reading_data = true;
            edge_counter = 0;
            data = 0;
        }
    }
    else // Read rising edge
    {
        if (reading_data)
        {
            // Measure time between falling and rising edges
            uint64_t delta = TICKS_TO_US(SYSTICK_RELOAD_VAL - SysTickValueGet());
            if (edge_counter == 0)
            {
                // Reset decoding variables on invalid start bit and exit
                if (delta <= 2000)
                {
                    reading_data = false;
                    edge_counter = 0;
                    return;
                }
            }
            else
            {
                // Write to data
                data <<= 1;
                if (delta <= 1000)
                {
                    data |= 1;
                }
            }
            edge_counter++;

            // Return the decoded signal if complete
            if (edge_counter == 13)
            {
                pin_out_intflag = 1;
                reading_data = false;
                edge_counter = 0;
            }
        }
    }
}

//****************************************************************************
//
//! Checks if switch is pressed
//!
//!
//! \return none
//
//****************************************************************************
static void GPIOA1IntHandler(void) {
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus(GPIOA1_BASE, true);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);        // clear interrupts on GPIOA1

    uint32_t pin_state = MAP_GPIOPinRead(GPIOA1_BASE, 0x20);
    if ((pin_state & 0x20) == 0) { // Read falling edge
        switch_intflag = 1;
    }
}

/**
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    systick_cnt++;
    global_time += 40;
}

/**
 * Timer Interrupt Handler
 *
 * Updates ammo count periodically and adjusts projectile sizing
 */
static void TimerA0Handler(void) {
    MAP_TimerIntClear(TIMERA0_BASE, TIMER_TIMA_TIMEOUT);  // Clear interrupt flag

    uint32_t pin_state = MAP_GPIOPinRead(GPIOA1_BASE, 0x20);
    static bool charging = false;
    static int starting_ammo = 6;  // Stores ammo count when button is first pressed

    if ((pin_state & 0x20) != 0) {  // Button is held down
        input_held_cnt++;

        if (!charging) {
            // Reset projectile_scale only when first pressed
            projectile_scale = 1;
            starting_ammo = ammo_cnt;  // Capture ammo count at start
            charging = true;
        }

        // Scale projectile size based on ammo spent
        if (ammo_cnt > 0 && (input_held_cnt % 15) == 0) {
            ammo_cnt--;

            // Ensure projectile_scale correctly follows the ammo depletion
            projectile_scale = (starting_ammo - ammo_cnt);
            if (projectile_scale > 6) {
                projectile_scale = 6;
            }
        }
        projectile_size = projectile_scale * 2;
    }
    else {  // Button is released
        charging = false;
    }
    timer++;

    if (timer > 25) {
        if (ammo_cnt < 6 && (MAP_GPIOPinRead(GPIOA1_BASE, 0x20) & 0x20) == 0) {
            ammo_cnt++;
        }
        timer = 0;
    }
}


void drawShipWithAmmo(int x, int y, int size, int color, int ammo) {
    int ammo_size = size / 6;
    int padding = size / 8;  // Space between ammo squares

    // Draw the ship
    fillRect(x, y, size, size, color);

    // Draw ammo squares based on ammo count
    if (ammo >= 1) fillRect(x + size + padding, y + size - ammo_size, ammo_size, ammo_size, color);
    if (ammo >= 2) fillRect(x - ammo_size - padding, y + size - ammo_size, ammo_size, ammo_size, color);
    if (ammo >= 3) fillRect(x + size + padding, y + size - (2 * ammo_size) - padding, ammo_size, ammo_size, color);
    if (ammo >= 4) fillRect(x - ammo_size - padding, y + size - (2 * ammo_size) - padding, ammo_size, ammo_size, color);
    if (ammo >= 5) fillRect(x + size + padding, y + size - (3 * ammo_size) - (2 * padding), ammo_size, ammo_size, color);
    if (ammo >= 6) fillRect(x - ammo_size - padding, y + size - (3 * ammo_size) - (2 * padding), ammo_size, ammo_size, color);
}


void InitUART1(void)
{
    // Configure UART1 for 115200 baud, 8 data bits, 1 stop bit, no parity
    MAP_UARTConfigSetExpClk(UART1,
                            MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                            115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    MAP_UARTFIFOEnable(UART1);
    MAP_UARTEnable(UART1);
}

void UART1SendProjectile(Projectile *proj)
{
    char buffer[11];

    // Pack projectile data into buffer
    buffer[0] = (proj->x_position >> 8) & 0xFF;
    buffer[1] = proj->x_position & 0xFF;

    buffer[2]  = (proj->x_velocity >> 24) & 0xFF;
    buffer[3]  = (proj->x_velocity >> 16) & 0xFF;
    buffer[4]  = (proj->x_velocity >> 8) & 0xFF;
    buffer[5]  = proj->x_velocity & 0xFF;

    buffer[6]  = (proj->y_velocity >> 24) & 0xFF;
    buffer[7]  = (proj->y_velocity >> 16) & 0xFF;
    buffer[8]  = (proj->y_velocity >> 8) & 0xFF;
    buffer[9]  = proj->y_velocity & 0xFF;

    buffer[10] = proj->size;

    // Send each byte through UART1
    int i;
    for (i = 0; i < 11; i++)
    {
        while (MAP_UARTBusy(UART1));
        MAP_UARTCharPut(UART1, buffer[i]);
    }
}

void UART1ReceiveProjectile(Projectile *proj)
{
    char buffer[11] = "";
    int index = 0;

    // Wait until we have received exactly 12 bytes of projectile data
    while (index < 11)
    {
        if (MAP_UARTCharsAvail(UART1))
        {
            buffer[index++] = MAP_UARTCharGetNonBlocking(UART1);
        }
        // Exit round and update score if collision on opponent's board was received
        if (strncmp(buffer, "QUIT", 4) == 0) {
            round_running = false;
            my_score++;

            // Send POST request
            char msg[100];
            snprintf(msg, sizeof(msg), "{ \"username\": \"%s\", \"my_score\": %d, \"opponent_score\": %d }", username, my_score, opponent_score);
            char jsonmsg[100];
            jsonify(msg, jsonmsg);
            http_post(lRetVal, jsonmsg);
            return;
        }
    }

    // Reconstruct projectile data from received bytes
    proj->x_position = 128 - ((buffer[0] << 8) | buffer[1]);
    proj->y_position = 128 - buffer[10];
    proj->x_velocity = ((buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5]) * -1;
    proj->y_velocity = ((buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9]) * -1;
    proj->size = buffer[10];
    proj->state = true;
}

void drawWinLoseScreen(bool won) {
    fillScreen(0xFA26);

    // Draw WIN or LOSE message
    char *message = won ? "WIN" : "LOSE";
    int i;
    int x = won ? 32 : 22;
    for (i = 0; i < strlen(message); i++) {
        drawChar(x, 28, message[i], 0xFFFF, PINK, 4);
        x += 24;
    }

    // Draw restart symbol
    int restartX = 86;
    int restartY = 36;
    for (i = 0; i < 4; i++) {
        drawCircle(restartX, restartY, 14 - i, 0xFFFF);
    }
    fillRect(restartX - 15, restartY - 15, 8, 18, PINK);
    fillTriangle(restartX - 8, restartY + 2, restartX - 12, restartY + 2, restartX - 12, restartY - 4, 0xFFFF);
    fillTriangle(restartX - 16, restartY + 2, restartX - 12, restartY + 2, restartX - 12, restartY - 4, 0xFFFF);

    // Draw home symbol
    int homeX = 42;
    int homeY = 36;
    fillRect(homeX - 13, homeY - 13, 21, 15, 0xFFFF);
    fillTriangle(homeX + 11, homeY - 1, homeX - 3, homeY - 1, homeX - 3, homeY + 13, 0xFFFF);
    fillTriangle(homeX - 17, homeY - 1, homeX - 3, homeY - 1, homeX - 3, homeY + 13, 0xFFFF);
    fillRect(homeX - 6, homeY - 13, 7, 10, PINK);
}

void jsonify(const char *msg, char *output) {
    const char *format = "{"
                         "\"state\": {\r\n"
                         "\"desired\" : {\r\n"
                         "\"default\" : %s\r\n"
                         "}"
                         "}"
                         "}\r\n\r\n";
    snprintf(output, 256, format, msg);
}


static int http_post(int iTLSSockID, char* msg){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(msg);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, msg);
    pcBufHeaders += strlen(msg);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

/**
 * Initializes SysTick Module
 */
 static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}

 static void InitTimerA0(void) {
     // Enable clock for TimerA0
     MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);

     // Configure TimerA0 as a periodic timer
     MAP_TimerConfigure(TIMERA0_BASE, TIMER_CFG_PERIODIC);

     // Set the timer period to trigger every 40ms (same as SysTick)
     MAP_TimerLoadSet(TIMERA0_BASE, TIMER_A, SYSTICK_RELOAD_VAL);

     // Register the interrupt handler
     MAP_TimerIntRegister(TIMERA0_BASE, TIMER_A, TimerA0Handler);

     // Enable TimerA0 interrupts
     MAP_TimerIntEnable(TIMERA0_BASE, TIMER_TIMA_TIMEOUT);
     MAP_IntEnable(INT_TIMERA0A);

     // Start TimerA0
     MAP_TimerEnable(TIMERA0_BASE, TIMER_A);
 }

//*****************************************************************************
//
//! Main function handling the I2C example
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main(){
    char acCmdStore[512];
    unsigned long ulStatus;

    //
    // Initialize board configurations
    //
    BoardInit();

    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

    InitUART1();

    //
    // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    // Enable SysTick
    SysTickInit();

    InitTimerA0();

    //
    // Configuring UART
    //
    InitTerm();

    //
    // I2C Init
    //
    I2C_IF_Open(I2C_MASTER_MODE_FST);
    
    //
    // Display the banner followed by the usage description
    //
    DisplayBanner(APP_NAME);

    //
    // Reset the peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    Adafruit_Init();

    //
    // Register the interrupt handler
    //
    MAP_GPIOIntRegister(GPIOA1_BASE, GPIOA1IntHandler);

    //
    // Configure falling edge interrupt on switch
    //
    MAP_GPIOIntTypeSet(GPIOA1_BASE, 0x20, GPIO_FALLING_EDGE);
    ulStatus = MAP_GPIOIntStatus(GPIOA1_BASE, false);
    MAP_GPIOIntClear(GPIOA1_BASE, ulStatus);

    // Enable switch interrupt
    MAP_GPIOIntEnable(GPIOA1_BASE, 0x20);
    switch_intflag = 0;

    //
    // Register the multi-tap interrupt handler
    //
    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler);

    //
    // Configure both edges interrupt on pin out
    //
    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x40, GPIO_BOTH_EDGES);

    ulStatus = MAP_GPIOIntStatus(GPIOA0_BASE, false);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);

    // clear global variables
    pin_out_intflag = 0;

    // Enable pin out interrupt
    MAP_GPIOIntEnable(GPIOA0_BASE, 0x40);

    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }

    // Initialize ship
    int SHIP_SIZE = 24;
    int PLAYER_COLOR = (PLAYER_MODE) ? RED : BLUE;
    int SCREEN = 128;

    // Create an array to hold up to MAX_PROJECTILES projectiles.
    Projectile projectiles[MAX_PROJECTILES];
    Projectile incoming_proj[MAX_PROJECTILES];

    while (FOREVER)
    {
        // Draw username prompt
        fillScreen(PINK);
        int i;
        int cx = 0;
        x = 12;
        char username_prompt[9] = "Username:";
        for (i = 0; i < 9; i++) {
            drawChar(x, 12, username_prompt[i], 0xFFFF, PINK, 2);
            x += 12;
        }
        x = 0;

        // Reset multi-tap variables
        memset(username, 0, sizeof(username));
        username[0] = '\0';
        data = 0;
        prev_data = -1;
        global_time = 0;
        prev_detection = 0;
        last_time_button_pressed = 0;
        curr_cycle = -1;
        curr_letter = 0;
        name_set = false;
        caps_lock = false;

        bool cursor_visible = true;
        uint32_t last_cursor_toggle = global_time;
        name_set = false;

        // Start multi-tap input and draw cursor
        while (!name_set) {
            if (global_time - last_cursor_toggle > 500) {  // Blink every 500ms
                cursor_visible = !cursor_visible;  // Toggle visibility
                fillRect(106 - cx, 73, 12, 2, cursor_visible ? 0xFFFF : PINK); // Toggle color
                last_cursor_toggle = global_time;
            }

            if (pin_out_intflag) {
                pin_out_intflag = 0;
                if ((global_time - prev_detection) >= 200) { // Ignore repeated data after first signal
                    prev_detection = global_time;

                    // Remove previous cursor
                    fillRect(106 - cx, 73, 12, 2, PINK);

                    if (((data != prev_data) || (global_time - last_time_button_pressed >= 1500)) && (prev_data != 0xD6F) && (prev_data != 0x22F)) {
                        if (prev_data != 0xFEF) {
                            l = strlen(username);
                            username[l] = curr_letter;
                            username[l + 1] = '\0';
                        }
                        x += 12;
                        curr_cycle = 0;
                    } else {
                        curr_cycle++;
                        cx -= 12;
                    }
                    last_time_button_pressed = global_time;

                    if (prev_data == 0x22F && data != 0x22F) { cx += 12; }

                    switch (data) {
                        case 0x6EF: curr_letter = ' '; break;
                        case 0xFEF: caps_lock = !caps_lock; curr_cycle--; x -= 12; cx -= 12; break;
                        case 0x7EF: curr_cycle %= 3; curr_letter = 'a' + curr_cycle - (caps_lock * 32); break;
                        case 0xBEF: curr_cycle %= 3; curr_letter = 'd' + curr_cycle - (caps_lock * 32); break;
                        case 0x3EF: curr_cycle %= 3; curr_letter = 'g' + curr_cycle - (caps_lock * 32); break;
                        case 0xDEF: curr_cycle %= 3; curr_letter = 'j' + curr_cycle - (caps_lock * 32); break;
                        case 0x5EF: curr_cycle %= 3; curr_letter = 'm' + curr_cycle - (caps_lock * 32); break;
                        case 0x9EF: curr_cycle %= 4; curr_letter = 'p' + curr_cycle - (caps_lock * 32); break;
                        case 0x1EF: curr_cycle %= 3; curr_letter = 't' + curr_cycle - (caps_lock * 32); break;
                        case 0xEEF: curr_cycle %= 4; curr_letter = 'w' + curr_cycle - (caps_lock * 32); break;
                        case 0x22F:  // Handle delete key
                            curr_letter = ' ';
                            curr_cycle--;
                            l = strlen(username);
                            if (l > 0) {
                                username[l - 1] = '\0';
                                // Clear the cursor at the old position before moving it back
                                fillRect(106 - cx, 73, 12, 2, PINK);
                                x -= 12;
                                if (prev_data != data) {
                                    cx -= 24;
                                } else {cx -= 12;}
                                if (cx < -12) { cx = -12; }
                                if (x < 12) { x = 12; }
                            }
                            prev_data = data;
                            break;
                        case 0xD6F:
                            if (strlen(username) > 0) {
                                Report("%s", username);
                                name_set = true;
                            }
                            curr_cycle = -1;
                            break;
                    }
                    prev_data = data;

                    if (data != 0xD6F && data != 0xFEF) { drawChar(x, 36, curr_letter, 0xFFFF, PINK, 2); }

                    if (data != 0xD6F) {
                        // Draw the new cursor after typing
                        cx += 12;
                        fillRect(106 - cx, 73, 12, 2, 0xFFFF);
                    }
                }
            }
        }

        // Signal other board of own ready state
        char readyMsg[5] = "READY";
        for (i = 0; i < 5; i++)
        {
            while (MAP_UARTBusy(UART1));
            MAP_UARTCharPut(UART1, readyMsg[i]);
        }

        // Send POST request
        char msg[100];
        snprintf(msg, sizeof(msg), "{ \"username\": \"%s\" }", username);
        char jsonmsg[100];
        jsonify(msg, jsonmsg);
        http_post(lRetVal, jsonmsg);

        // Confirm other board's ready state and start game
        char buffer[5] = "";
        int index = 0;
        while (index < 5)
        {
            if (MAP_UARTCharsAvail(UART1))
            {
                buffer[index++] = MAP_UARTCharGetNonBlocking(UART1);
            }
            if (strncmp(buffer, "READY", 5) == 0) {
                game_running = true;
            }
        }

        my_score = 0;
        opponent_score = 0;

        // Send POST request
        snprintf(msg, sizeof(msg), "{ \"username\": \"%s\" }", username);
        jsonify(msg, jsonmsg);
        http_post(lRetVal, jsonmsg);

        while (game_running)
        {
            fillScreen(BLACK);
            int shipPosition[2] = { ( (SCREEN / 2) - (SHIP_SIZE / 2) ), ( (SCREEN / 2) - (SHIP_SIZE / 2) ) };
            int8_t shipVelocity[2] = { 0, 0 };
            drawShipWithAmmo( ((SCREEN / 2) - (SHIP_SIZE / 2)), ((SCREEN / 2) - (SHIP_SIZE / 2)), SHIP_SIZE, PLAYER_COLOR, 6);

            int i;
            for (i = 0; i < MAX_PROJECTILES; i++)
            {
                projectiles[i].state = false;
                projectiles[i].x_position = 0;
                projectiles[i].y_position = 0;
                projectiles[i].x_velocity = 0;
                projectiles[i].y_velocity = 0;
                projectiles[i].size = 0;

                incoming_proj[i].state = false;
                incoming_proj[i].x_position = 0;
                incoming_proj[i].y_position = 0;
                incoming_proj[i].x_velocity = 0;
                incoming_proj[i].y_velocity = 0;
                incoming_proj[i].size = 0;
            }

            round_running = true;
            while (round_running)
            {
                // Handle Incoming Projectile
                if (MAP_UARTCharsAvail(UART1))
                {
                    Projectile new_proj;
                    UART1ReceiveProjectile(&new_proj);
                    if (!round_running) {
                        break;
                    }

                    // Add the received projectile to an available slot
                    int i;
                    for (i = 0; i < MAX_PROJECTILES; i++)
                    {
                        if (!incoming_proj[i].state)  // Find an inactive slot
                        {
                            incoming_proj[i] = new_proj;
                            break;
                        }
                    }
                }

                // Update Ship Position
                drawShipWithAmmo(shipPosition[0], shipPosition[1], SHIP_SIZE, BLACK, 6); // Erase ship from the old position

                // Get acceleration data and scale with max acceleration
                int8_t* accData = ReadAccData();
                int8_t xAcc = (int8_t)(((double)accData[0] / 64) * 13);
                int8_t yAcc = (int8_t)(((double)accData[1] / 64) * 13);

                // Update velocity based on acceleration and apply friction
                shipVelocity[0] = (shipVelocity[0] + xAcc) * 0.99;
                shipVelocity[1] = (shipVelocity[1] + yAcc) * 0.99;

                // Update position based on velocity
                shipPosition[0] += shipVelocity[0];
                shipPosition[1] += shipVelocity[1];

                // Keep ship position within screen bounds
                if (shipPosition[0] <= 0) {
                    shipPosition[0] = 0;
                    shipVelocity[0] = 0;
                } else if (shipPosition[0] > SCREEN - SHIP_SIZE - 1) {
                    shipPosition[0] = SCREEN - SHIP_SIZE - 1;
                    shipVelocity[0] = 0;
                }

                if (shipPosition[1] <= 0) {
                    shipPosition[1] = 0;
                    shipVelocity[1] = 0;
                } else if (shipPosition[1] > SCREEN - SHIP_SIZE - 1) {
                    shipPosition[1] = SCREEN - SHIP_SIZE - 1;
                    shipVelocity[1] = 0;
                }

                // Draw ship at the new position
                drawShipWithAmmo(shipPosition[0], shipPosition[1], SHIP_SIZE, PLAYER_COLOR, ammo_cnt);


                // Handle New Projectile
                if (switch_intflag)
                {
                    switch_intflag = 0;
                    HWREG(NVIC_ST_CURRENT) = 1;  // Reset SysTick counter
                    input_held_cnt = 0;

                    // Fire projectile only if ammo is available
                    if (ammo_cnt > 0 || projectile_scale > 1) {
                        int i;
                        for (i = 0; i < MAX_PROJECTILES; i++)
                        {
                            if (!projectiles[i].state)  // Find an inactive slot
                            {
                                projectiles[i].state = true;
                                projectiles[i].x_position = shipPosition[0] + (SHIP_SIZE / 2);
                                projectiles[i].y_position = shipPosition[1] + SHIP_SIZE + projectile_size;
                                projectiles[i].x_velocity = xAcc;
                                projectiles[i].y_velocity = 10 + (projectile_scale * 4);
                                projectiles[i].size = projectile_size;

                                // Reset scale to minimum after firing
                                projectile_scale = 1;
                                if (ammo_cnt > 0) {
                                    ammo_cnt--;   // Decrease ammo immediately on shot
                                }
                                break;
                            }
                        }
                    }
                }


                // Update Projectiles
                int j;
                for (j = 0; j < MAX_PROJECTILES; j++)
                {
                    if (projectiles[j].state)
                    {
                        // Erase the projectile at its old position
                        fillCircle(projectiles[j].x_position,
                                   projectiles[j].y_position,
                                   projectiles[j].size,
                                   BLACK);

                        // Update projectile position based on its velocity
                        projectiles[j].x_position += projectiles[j].x_velocity;
                        projectiles[j].y_position += projectiles[j].y_velocity;

                        // Check if projectile is off-screen
                        if (projectiles[j].x_position < 0 || projectiles[j].x_position > (SCREEN - projectiles[j].size - 1) ||
                            projectiles[j].y_position > (SCREEN - projectiles[j].size - 1))
                        {
                            projectiles[j].state = false;  // Deactivate projectile
                            if (projectiles[j].y_position > (SCREEN - projectiles[j].size - 1)) {
                                UART1SendProjectile(&projectiles[j]);
                            }
                        }
                        else
                        {
                            // Draw projectile at its new position
                            fillCircle(projectiles[j].x_position,
                                       projectiles[j].y_position,
                                       projectiles[j].size,
                                       PLAYER_COLOR);
                        }
                    }
                }

                // Update Incoming Projectiles
                for (j = 0; j < MAX_PROJECTILES; j++)
                {
                    if (incoming_proj[j].state)
                    {
                        // Erase the projectile at its old position
                        fillCircle(incoming_proj[j].x_position,
                                   incoming_proj[j].y_position,
                                   incoming_proj[j].size,
                                   BLACK);

                        // Update projectile position based on its velocity
                        incoming_proj[j].x_position += incoming_proj[j].x_velocity;
                        incoming_proj[j].y_position += incoming_proj[j].y_velocity;

                        // Collision Detection
                        int proj_x = incoming_proj[j].x_position;
                        int proj_y = incoming_proj[j].y_position;
                        int proj_size = incoming_proj[j].size;

                        // Check if projectile collides with the ship
                        if (proj_x + proj_size > shipPosition[0] && proj_x - proj_size < shipPosition[0] + SHIP_SIZE &&
                            proj_y + proj_size > shipPosition[1] && proj_y - proj_size < shipPosition[1] + SHIP_SIZE)
                        {
                            char quitMsg[4] = "QUIT";
                            int i;
                            for (i = 0; i < 4; i++)
                            {
                                while (MAP_UARTBusy(UART1));
                                MAP_UARTCharPut(UART1, quitMsg[i]);
                            }

                            // Clear the projectile
                            incoming_proj[j].state = false;
                            round_running = false;

                            opponent_score++;
                            // Send POST request
                            char msg[100];
                            snprintf(msg, sizeof(msg), "{ \"username\": \"%s\", \"my_score\": %d, \"opponent_score\": %d }", username, my_score, opponent_score);
                            char jsonmsg[100];
                            jsonify(msg, jsonmsg);
                            http_post(lRetVal, jsonmsg);
                            
                            break;
                        }

                        // Check if projectile is off-screen
                        if (incoming_proj[j].x_position < 0 || incoming_proj[j].x_position > (SCREEN - incoming_proj[j].size - 1) ||
                                incoming_proj[j].y_position < (incoming_proj[j].size + 1))
                        {
                            incoming_proj[j].state = false;  // Deactivate projectile
                        }
                        else
                        {
                            // Draw projectile at its new position
                            fillCircle(incoming_proj[j].x_position,
                                       incoming_proj[j].y_position,
                                       incoming_proj[j].size,
                                       (PLAYER_MODE) ? BLUE : RED);
                        }
                    }
                }
            }

            // Display end screen
            if (my_score >= 5 || opponent_score >= 5) {
                data = 0;
                prev_data = 0;
                drawWinLoseScreen((my_score > opponent_score));
                int selectX = 65;
                bool select_visible = false;
                uint32_t last_select_toggle = global_time;
                char ack = 0;

                // Clear UART Buffer
                while (MAP_UARTCharsAvail(UART1)) {
                    MAP_UARTCharGetNonBlocking(UART1);
                }

                while (data != 0x58F) {
                    if (global_time - last_select_toggle > 400) {  // Blink every 400ms
                        select_visible = !select_visible;  // Toggle visibility
                        last_select_toggle = global_time;
                        drawRoundRect(selectX, 15, 42, 42, 5, select_visible ? 0xFFFF : PINK);
                        drawRoundRect(selectX + 1, 16, 40, 40, 5, select_visible ? 0xFFFF : PINK);
                    }

                    if (data != prev_data) {
                        if (data == 0xD2F) {
                            prev_data = data;
                            drawRoundRect(selectX, 15, 42, 42, 5, PINK);
                            drawRoundRect(selectX + 1, 16, 40, 40, 5, PINK);
                            selectX = 65;
                            game_running = true;
                        } else if (data == 0x32F) {
                            prev_data = data;
                            drawRoundRect(selectX, 15, 42, 42, 5, PINK);
                            drawRoundRect(selectX + 1, 16, 40, 40, 5, PINK);
                            selectX = 18;
                            game_running = false;
                        }
                    }
                }
                my_score = 0;
                opponent_score = 0;

                if (game_running) {
                    while (MAP_UARTBusy(UART1));
                    MAP_UARTCharPut(UART1, 'r');
                } else {
                    while (MAP_UARTBusy(UART1));
                    MAP_UARTCharPut(UART1, 'h');
                }

                while (!MAP_UARTCharsAvail(UART1));
                ack = MAP_UARTCharGetNonBlocking(UART1);
                if (ack == 'r' && game_running) {
                    game_running = true;

                    // Send POST request
                    char msg[100];
                    snprintf(msg, sizeof(msg), "{ \"username\": \"%s\" }", username);
                    char jsonmsg[100];
                    jsonify(msg, jsonmsg);
                    http_post(lRetVal, jsonmsg);
                } else if (ack == 'h') {
                    game_running = false;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @
//
//*****************************************************************************
