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


unsigned char cmd[16];
unsigned char* tempHistory[32];
unsigned char time[3];
unsigned char log[15] = {[6] = ':', [7] = ' ', [8] = ' ', [13] = '\r', [14] = '\n'};
unsigned char *hr = &time[0];
unsigned char *min = &time[1];
unsigned char *sec = &time[2];

// Queue points
unsigned char start = 0, size = 0;

char cmdPos = 0;
unsigned char len = 64;
static const char *err = "No temperatures recorded.";
//unsigned char sec, min, hr;
unsigned char flagCMD = 0, flagTIMER = 0;
unsigned int adc_read;



void commandHandler(void);
void updateTime(void);
void getTemp(void);
void to_hex(unsigned int value);
void getTime(void);

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
    P1OUT = 0;

    ADC10CTL0   &=  ENC;

    ADC10CTL0   =   SREF_1          // V_ref+ and V_ss as references
                    | ADC10SHT_3    // sample for 16 ADC clock cycles
                    | REFON         // reference generator on
                    | ADC10ON;      // turn on the ADC

    // sample period for the temperature sensor must be higher than 30us
    ADC10CTL1   =   INCH_10         // temperature sensor channel
                    | ADC10DIV_2    // divide source clock by 2
                    | ADC10SSEL_3;  // use SMCLK


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

        IE2 |= UCA0RXIFG;

        // GIE | LPM3
        __bis_SR_register(0b011011000);
    }
}

void updateTime(void)
{
    if (*sec >= 60)
    {
        (*min)++;
        *sec -= 60;

        if(*min % 5 == 0)
            getTemp();
        {
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
            {
                IE2 |= UCA0TXIFG;
                loop_until_bit_is_set(IFG2,(UCA0TXIFG));
                UCA0TXBUF = log[i];
            }

            IE2 |= UCA0TXIFG;
            loop_until_bit_is_set(IFG2,(UCA0TXIFG));
            UCA0TXBUF = '\n';
            IE2 |= UCA0TXIFG;
            loop_until_bit_is_set(IFG2,(UCA0TXIFG));
            UCA0TXBUF = '\r';

            IE2 &= ~UCA0TXIFG;

                // ********************************//
            TACCTL0 = CCIE;

            break;

        case 's':
        case 'S':
    //                P1OUT = LED1;
       //     P1OUT |= LED2;
      //      P1OUT |= LED1;
            *hr  = (cmd[1] - '0') * 10 + cmd[2] - '0';
            *min = (cmd[3] - '0') * 10 + cmd[4] - '0';
            *sec = (cmd[5] - '0') * 10 + cmd[6] - '0';
            /*
            for (i = 0; i < 6; i++)
            {
                time[i] = cmd[i + 1] - '0';
            }
*/
//            sec = min = hr = 0;


            break;

        case 'o':
        case 'O':
            if (size == 0)
            {

                for (i = 0; i < 25; i++)
                {
                    IE2 |= UCA0TXIFG;
                    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
                    UCA0TXBUF = err[i];
                }
                IE2 &= ~UCA0TXIFG;
            }

            else
            {
                for(i = 0; i < 15; i++)
                {
                    IE2 |= UCA0TXIFG;
                    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
                    UCA0TXBUF = tempHistory[start][i];
                }

                IE2 &= ~UCA0TXIFG;

                size--;
                start++;

            }


            break;

        case 'l':
        case 'L':
            if (size == 0)
            {

                for (i = 0; i < 25; i++)
                {
                    IE2 |= UCA0TXIFG;
                    loop_until_bit_is_set(IFG2,(UCA0TXIFG));
                    UCA0TXBUF = err[i];
                }
                IE2 &= ~UCA0TXIFG;
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
    log[5] = (*sec) % 10 + '0';
    log[4] = (*sec) / 10 + '0';
    log[3] = (*min) % 10 + '0';
    log[2] = (*min) / 10 + '0';
    log[1] = (*hr)  % 10 + '0';
    log[0] = (*hr)  / 10 + '0';
}

void getTemp(void)
{
    P1OUT = LED1;

    ADC10CTL0   |=  ENC         /* enable conversion */
                | ADC10SC;  /* start conversion */
            /* wait for conversion to finish */
    loop_until_bit_is_set(ADC10CTL0, ADC10IFG);

            /* grab value */
    adc_read = ADC10MEM;

            /* convert it to hexadecimal */
    to_hex(adc_read);

    getTime();

    tempHistory[(start - size) % 32] = log;
    size++;
}

void to_hex(unsigned int value)
{
    char t;
    size_t i;
    for(i = 0; i < 4; i++)
    {
        t = value & 0xf;
        value >>= 4;
        log[12 - i] = (t < 10) ? (t + '0') : (t + 'a' - 10);
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
  //  P1OUT |= LED2;
    // seconds!!!!!!!!!
    (*sec)++;
    flagTIMER = 1;
    __bic_SR_register_on_exit(LPM3_bits);
}
