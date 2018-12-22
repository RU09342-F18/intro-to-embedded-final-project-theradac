#include <msp430.h> 

// necessary variables
int i;
unsigned int j = 0;
char WaveSel = 'q';
unsigned int in;
float volts;
int dist;
long Freq;
// Sine look up table with 8 values to make a sine wave
int SinLUT[] = {8,13,15,13,8,2,0,2,};


void SetVcoreUp (unsigned int level)
{
  // Open PMM registers for write
  PMMCTL0_H = PMMPW_H;
  // Set SVS/SVM high side new level
  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
  // Set SVM low side to new level
  SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
  // Wait till SVM is settled
  while ((PMMIFG & SVSMLDLYIFG) == 0);
  // Clear already set flags
  PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
  // Set VCore to new level
  PMMCTL0_L = PMMCOREV0 * level;
  // Wait till new level reached
  if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0);
  // Set SVS/SVM low side to new level
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
  // Lock PMM registers for write access
  PMMCTL0_H = 0x00;
}

void clkSet(void)
{
    // Increase Vcore setting to level3 to support fsystem=25MHz
    // NOTE: Change core voltage one level at a time..
    SetVcoreUp (0x01);
    SetVcoreUp (0x02);
    SetVcoreUp (0x03);

    UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_7;                      // Select DCO range 50MHz operation
    UCSCTL2 = FLLD_0 + 762;                   // Set DCO Multiplier for 25MHz
                                              // (N + 1) * FLLRef = Fdco
                                              // (762 + 1) * 32768 = 25MHz
                                              // Set FLL Div = fDCOCLK/2
    __bic_SR_register(SCG0);                  // Enable the FLL control loop
}
void initADC(void)
{
    P6SEL |= 0x01;                            // Enable A/D channel A0
    ADC12CTL0 = ADC12SHT0_4 + ADC12ON + ADC12REFON + ADC12REF2_5V; // turns on adc 12 and samples every 12 ADC12CLK cycles
    ADC12CTL1 |= ADC12SHP + ADC12SSEL_3; // ADC12 uses SMCLK
    ADC12MCTL0 |= ADC12INCH_0; // selects channel 0 for ADC input
    ADC12IE |= ADC12IE0; // enables ADC interrupt
    ADC12CTL0 |= ADC12ENC;                    // Enable conversions
    ADC12CTL0 |= ADC12SC;                     // Start conversion
}
void initTimer(void)
{
    // Timer A0 setup
    TA0CCTL0 = CCIE; // enables interrupt on timer A0
    TA0CCR0 = Freq; // timer A0 counts to 10000
    TA0CTL = TASSEL_2 + MC_1 + TACLR; // sets timer A to up mode, use SMCLK
}
void initGPIO(void)
{
    // GPIO setup
    P4SEL = 0x00; // port 6 set to GPIO
    P4DIR |= BIT0 + BIT1 + BIT2 + BIT3; // pins 6.0-3 set to output
    P4OUT &= 0X00; // starts all pins as 0
}
void initUART(void)
{
    P3SEL |= BIT3 + BIT4; // sets pin 3.3 and 3.4 for USCI_A0 TXD/RXD
    UCA0CTL1 |= UCSWRST; // resets USCI logic to hold in reset state
    UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
    UCA0BR0 = 0x03;                           // 1 MHz 9600(see User's Guide)
    UCA0BR1 = 0x00;                           // 1 MHz 9600
    UCA0MCTL = UCBRS_3 + UCBRF_0;   // Modln UCBRSx = 3, UCBRFx = 0
    UCA0CTL1 &= ~(UCSWRST); // starts USCI state machine
    UCA0IE |= UCRXIE;
}
/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    clkSet();
    initGPIO(); // initializes GPIO
    initTimer(); // initilaizes timer
    initUART(); // initializes UART
    initADC(); // Initialize ADC

        __bis_SR_register(LPM0_bits + GIE); // enables low power mode 0 and global interrupts
}
// UART ISR
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0(void)
{
    switch (__even_in_range(UCA0IV, 2))
    {
    case 0:
        break; // vector 0, no interrupts
    case 2: // RXIFG
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = UCA0RXBUF; // holds recieved charater
        WaveSel = UCA0RXBUF;
        break;
    default:
        break;
    }

}
// ADC ISR
#pragma vector = ADC12_VECTOR
__interrupt void ADC(void)
{
    ADC12CTL0 &= ~ADC12SC;
    in = ADC12MEM0; // sets in to ADC conversion results
    volts = in*0.00081; // converts bits to voltage

    dist = (volts*-18.75)+56.0625; // gets distance in cm
    if (dist >= 10 && dist <= 40)
    {
        if (WaveSel == 'q')
        {
            Freq = dist*500 + 5000;
            TA0CCR0 = Freq;
        }
        else
        {
            Freq = dist*330 - 3200;
            TA0CCR0 = Freq;
        }
    }
    else if (dist < 10)
    {
        dist = 10;
    }
    else if (dist >= 40 && dist <= 50)
    {
        dist = 40;
    }
    else
    {
        Freq = 0;
        TA0CCR0 = Freq;
        P4OUT &= 0;
    }
    ADC12CTL0 |= ADC12SC; // enables sampling and conversion
}

// Timer A0 interrupt service routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    if(WaveSel == 's')
    {
        switch(j)
        {
        case 0:
            P4OUT = SinLUT[0];
            j++;
            break;
        case 7:
            P4OUT = SinLUT[7];
            j = 0;
            break;
        default:
            P4OUT = SinLUT[j++];
            break;
        }

    }
    else if(WaveSel == 't')
    {
        switch(P4OUT)
        {
        case 0:
            i = 3;
            P4OUT = P4OUT + i;
            break;
        case 15:
            i = -3;
            P4OUT = P4OUT + i;
            break;
        default:
            P4OUT = P4OUT + i;
        }
    }
    else if(WaveSel == 'r')
    {
        switch(P4OUT)
        {
        case 0:
            i = 2;
            ++P4OUT;
            break;
        case 15:
            i = 15;
            P4OUT = P4OUT - i;
            break;
        default:
            P4OUT = P4OUT + i;
        }
    }
    else if(WaveSel == 'q')
    {
        switch(P4OUT)
        {
        case 0:
            P4OUT = 15;
            break;
        case 15:
            P4OUT = 0;
            break;
        default:
            P4OUT = 0;
        }
    }
    else
    {
        switch(P4OUT)
        {
        case 0:
            P4OUT = 15;
            break;
        case 15:
            P4OUT = 0;
            break;
        default:
            P4OUT = 0;
        }
    }
}
