// Lance Adler
// Hunter Tanchin
// Zachary Freer

#include <msp430.h>
#include <stddef.h>
#include <stdlib.h>
#include "uart.h"
#include "interrupt.h"
#include "bitmanip.h"
// #include "global.h"


// Constants for the the red and green leds and the push button
#define LED1    BIT0
#define LED2    BIT6
#define BUTTON  BIT3

/* if we are not using gcc, we need these headers */
//#ifndef __GNUC__
#include "wdt.h"
#include "dco.h"
//#endif

//
//#ifdef __GNUC__
//__attribute__((noreturn))
//void
//#else
//int
//#endif

static const char *err = "No temperatures recorded.";

static unsigned char log00[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log01[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log02[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log03[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log04[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log05[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log06[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log07[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};

static unsigned char log08[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log09[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log10[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log11[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log12[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log13[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log14[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log15[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};

static unsigned char log16[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log17[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log18[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log19[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log20[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log21[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log22[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log23[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};

static unsigned char log24[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log25[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log26[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log27[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log28[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log29[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log30[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
static unsigned char log31[10];// = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};


static unsigned char* log[32] = {log00, log01, log02, log03, log04, log05, log06, log07, log08, log09, log10, log11, log12, log13, log14, log15, log16, log17, log18, log19, log20, log21, log22, log23, log24, log25, log26, log27, log28, log29, log30, log31};

unsigned char cmd[16];
unsigned char time[3];
unsigned char clock[6]; // = {[6] = '\r', [7] = '\n'};

//unsigned char log[15] = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
unsigned char *hr = &time[0];
unsigned char *min = &time[1];
unsigned char *sec = &time[2];

// Queue points
unsigned char start = 0, size = 0, next = 0;

char cmdPos = 0;
unsigned char j;

//unsigned char sec, min, hr;
unsigned char flagCMD = 0, flagTIMER = 0, flagBUTTON = 0;
unsigned int adc_read;

unsigned char checkpoint, dELETEME;

void buttonHandler(void);
void commandHandler(void);
void updateTime(void);
void getTemp(void);
void to_hex(unsigned int value);
void getTime(void);
void outUART(unsigned char c);

main(void)
{
//    # ifndef __GNUC__
    /* If we are compiling with gcc, the symbol __GNUC__ is defined and we do
     * not run any of these functions. Since we added these functions to the
     * constructors array, the runtime takes care of that for us. If we are
     * not using gcc, we must call these functions manually  */


    wdt_disable();
    dco_calibrate();
//    #endif

    P1DIR |= LED1 + LED2;

    P1DIR &= ~BUTTON;

    // Enable resistor for button as pull up
    P1REN = BUTTON;
    P1OUT = BUTTON;

    // Falling edge triggered and resets the flag bit
    P1IES = BUTTON;
    P1IFG = 0x00;

    P1IE = BUTTON;

    adc_init();
    timera_init();
    uart_init();

    // GIE | LPM3
    __bis_SR_register(0b011011000);

    while(1)
    {

        if ((cmdPos > 0) && (cmdPos <=16) && cmd[cmdPos - 1] == '\r' && flagCMD)
        {
            commandHandler();
            flagCMD = 0;
        }

        if (flagTIMER)
        {
            updateTime();
            flagTIMER = 0;
        }

        if(flagBUTTON)
        {
            buttonHandler();
            flagBUTTON = 0;
        }

        IE2 |= UCA0RXIFG;

        // GIE | LPM3
        __bis_SR_register(0b011011000);
    }
}

ISR(PORT1_VECTOR)
{

    P1IE = 0x00;
    P1IFG = 0x00;
    flagBUTTON = 1;

    __bic_SR_register_on_exit(LPM3_bits);
}

void buttonHandler(void)
{
    P1OUT ^= LED1;
    getTime();

    for (j = 0; j < 6; j++)
        outUART(clock[j]);

    outUART('\n');
    outUART('\r');

    IE2 &= ~UCA0TXIFG;

    if (size <= 0)
    {

        for (j = 0; j < 25; j++)
            outUART(err[j]);

        outUART('\n');
        outUART('\r');
        IE2 &= ~UCA0TXIFG;
    }


    else
    {
        while (size > 0)
        {
            for(j = 0; j < 6; j++)
                outUART(log[start][j]);

            outUART(':');
            outUART(' ');
            outUART(' ');

            for (j = 6; j < 10; j++)
                outUART(log[start][j]);

            outUART('\n');
            outUART('\r');

            IE2 &= ~UCA0TXIFG;

            size--;
            start = (start + 1) % 32;
        }
    }

    P1IE = BUTTON;
}

void updateTime(void)
{
//    if((*sec - dELETEME) % 5 == 0)
//        getTemp();

    if (*sec >= 60)
    {
        (*min)++;
        *sec -= 60;

        if((*min - checkpoint) % 5 ==0)
            getTemp();

        if(*min >= 60)
        {
            (*hr)++;
            *min -= 60;

            if(*hr >= 24)
            {
                (*hr) -= 24;
            }
        }
    }
}

void commandHandler(void)
{
   // P1OUT ^= LED2;
    unsigned char i;

    switch(cmd[0])
    {
        case 't':
        case 'T':

            getTime();

            for (i = 0; i < 6; i++)
                outUART(clock[i]);

            outUART('\n');
            outUART('\r');

            IE2 &= ~UCA0TXIFG;

                // ********************************//
            TACCTL0 = CCIE;

            break;

        case 's':
        case 'S':

            *hr  = (cmd[1] - '0') * 10 + cmd[2] - '0';
            *min = (cmd[3] - '0') * 10 + cmd[4] - '0';
            *sec = (cmd[5] - '0') * 10 + cmd[6] - '0';

            outUART('\n');
            outUART('\r');
            IE2 &= ~UCA0TXIFG;

            checkpoint = *min;
            dELETEME = *sec;

            flagTIMER = 1;
            TACCTL0 = CCIE;

            break;

        case 'o':
        case 'O':
            if (size <= 0)
            {

                for (i = 0; i < 25; i++)
                    outUART(err[i]);
//                {
//                    IE2 |= UCA0TXIFG;
//                    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
//                    UCA0TXBUF = err[i];
//                }
                outUART('\n');
                outUART('\r');
                IE2 &= ~UCA0TXIFG;
            }

            else
            {
                for(i = 0; i < 6; i++)
                    outUART(log[start][i]);

                outUART(':');
                outUART(' ');
                outUART(' ');

                for(i = 6; i < 10; i++)
                    outUART(log[start][i]);

                outUART('\n');
                outUART('\r');

                IE2 &= ~UCA0TXIFG;

                size--;
                start = (start + 1) % 32;

            }


            break;

        case 'l':
        case 'L':
            if (size <= 0)
            {

                for (i = 0; i < 25; i++)
                    outUART(err[i]);

                outUART('\n');
                outUART('\r');
                IE2 &= ~UCA0TXIFG;
            }


            else
            {
                while (size > 0)
                {
                    for(i = 0; i < 6; i++)
                        outUART(log[start][i]);

                    outUART(':');
                    outUART(' ');
                    outUART(' ');

                    for (i = 6; i < 10; i++)
                        outUART(log[start][i]);

                    outUART('\n');
                    outUART('\r');

                    IE2 &= ~UCA0TXIFG;

                    size--;
                    start = (start + 1) % 32;
                }
            }

            break;
    }

    for (i = 0; i < 16; i++)
        cmd[i] = 0;
    cmdPos = 0;

    IE2 |= UCA0RXIFG;
}

void getTime(void)
{
    clock[5] = (*sec) % 10 + '0';
    clock[4] = (*sec) / 10 + '0';
    clock[3] = (*min) % 10 + '0';
    clock[2] = (*min) / 10 + '0';
    clock[1] = (*hr)  % 10 + '0';
    clock[0] = (*hr)  / 10 + '0';
}

void getTemp(void)
{
//    P1OUT ^= LED1;

    ADC10CTL0   |=  ENC         /* enable conversion */
                | ADC10SC;  /* start conversion */
            /* wait for conversion to finish */
    loop_until_bit_is_set(ADC10CTL0, ADC10IFG);

            /* grab value */
    adc_read = ADC10MEM;

            /* convert it to hexadecimal */
    to_hex(adc_read);

    getTime();

    for(j = 0; j < 6; j++)
        log[(start + size) % 32][j] = clock[j];

    if (size >= 32)
        start = (start + 1) % 32;
    else
        size++;

//    next = (next + 1) % 32;

//    if (next == (start + 1) % 32);
//        start = (start + 1) % 32;
}

void to_hex(unsigned int value)
{
    char t;
    size_t i;
    for(i = 0; i < 4; i++)
    {
        t = value & 0xf;
        value >>= 4;
        log[(start + size) % 32][9 - i] = (t < 10) ? (t + '0') : (t + 'a' - 10);
    }
}

// Interrupt procedure when the UART transmit buffer is full
ISR(USCIAB0TX_VECTOR)
{
    IE2 &= ~UCA0TXIFG;
}

// Interrupt procedure when the UART receiving buffer is full
ISR(USCIAB0RX_VECTOR)
{
    cmd[cmdPos++] = UCA0RXBUF;

    // echo it back to the terminal
    IE2 |= UCA0TXIFG;
    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
    UCA0TXBUF = cmd[cmdPos - 1];

    // If end of command the last character must be a carriage return, turn off the
    // receiving interrupt because our string is full
    if (cmd[cmdPos - 1] == '\r')
    {
        IE2 &= ~UCA0RXIFG;
        IE2 &= ~UCA0TXIFG;
        flagCMD = 1;
        // LPM3
        __bic_SR_register_on_exit(LPM3_bits);
    }
}

ISR(TIMER0_A0_VECTOR)
{
    (*sec)++;
    flagTIMER = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}

void outUART(unsigned char c)
{
    IE2 |= UCA0TXIFG;
    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
    UCA0TXBUF = c;
}
