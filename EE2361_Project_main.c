#include "xc.h"
#include "Servo.h"

#pragma config ICS = PGx1          // Comm Channel Select (Emulator EMUC1/EMUD1 pins are shared with PGC1/PGD1)
#pragma config FWDTEN = OFF        // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config GWRP = OFF          // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF           // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF        // JTAG Port Enable (JTAG port is disabled)

#pragma config I2C1SEL = PRI       // I2C1 Pin Location Select (Use default SCL1/SDA1 pins)
#pragma config IOL1WAY = OFF       // IOLOCK Protection (IOLOCK may be changed via unlocking seq)
#pragma config OSCIOFNC = ON       // Primary Oscillator I/O Function (CLKO/RC15 functions as I/O pin)
#pragma config FCKSM = CSECME      // Clock Switching and Monitor (Clock switching is enabled, 
                                       // Fail-Safe Clock Monitor is enabled)
#pragma config FNOSC = FRCPLL      // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))

volatile unsigned long int sec = 0;
void __attribute__((interrupt, auto_psv)) _T2Interrupt(void) { 
    _T2IF = 0;
    sec++;
}

volatile unsigned long int buffer[3];
volatile unsigned int bufferPlace = 0;

volatile unsigned long int eventTime = 0;
volatile unsigned long int lastEventTime = 0;
volatile unsigned long int tripleClickTime = 0;
volatile unsigned int interruptVar = 0;
void __attribute__((interrupt, auto_psv)) _IC1Interrupt(void) {
   _IC1IF = 0;
   eventTime = IC1BUF + (sec*62500L);
   if ((eventTime - lastEventTime) > 125) {
        buffer[bufferPlace] = eventTime;
        bufferPlace = (bufferPlace + 1) % 3;
        if ((eventTime - buffer[bufferPlace]) < 40000) {
            setServo(3000);
            tripleClickTime = eventTime;
        }
   }
   lastEventTime = eventTime;
   interruptVar = interruptVar + 1;
}

void setup() {
    CLKDIVbits.RCDIV = 0;  //Set RCDIV=1:1 (default 2:1) 32MHz or FCY/2=16M
	AD1PCFG = 0x9fff;            //sets all pins to digital I/O
    TRISBbits.TRISB15 = 0;      // Green LED output
    TRISBbits.TRISB14 = 0;      // Yellow LED output
    TRISBbits.TRISB13 = 0;      // Red LED output
    TRISBbits.TRISB6 = 0;       // Servo motor output
    
    // TRISBbits.TRISB7 = 1;       // Button input IF WE WANT TO ADD BUTTON
    TRISBbits.TRISB8 = 1;       // Piezo input
	LATA = 0xffff;              // Set all of port A to HIGH
	LATB = 0xffff;              // and all of port B to HIGH
}

void initPushButton(void) {
    // Configure Timer 2 (500ns / count, 25ms max).
    // note that resolution = 500ns = 8 x 62.5ns, max period = 25ms = Tcy * 8 * 50,000
    
    __builtin_write_OSCCONL(OSCCON & 0xbf);
    RPINR7bits.IC1R = 8;
    __builtin_write_OSCCONL(OSCCON | 0x40);
    
    T2CONbits.TON = 0;
    T2CONbits.TCKPS = 0b11;
    T2CONbits.TCS = 0b0;
    T2CONbits.TGATE = 0b0;
    TMR2 = 0;
    // Initialize to zero (also best practice)
    PR2 = 0xF424; // Set period to one second
    T2CONbits.TON = 1; // Start 16-bit Timer2
    
    // Initialize the Input Capture Module
    IC1CONbits.ICTMR = 1; // Select Timer2 as the IC1 Time base
    IC1CONbits.ICI = 0b00; // Interrupt on every capture event
    IC1CONbits.ICM = 0b011; // Generate capture event on every Rising edge
    // Enable Capture Interrupt And Timer2
    
    IPC0bits.IC1IP = 1; // Setup IC1 interrupt priority level
    IFS0bits.IC1IF = 0; // Clear IC1 Interrupt Status Flag
    IEC0bits.IC1IE = 1; // Enable IC1 interrupt
    
    IFS0bits.T2IF = 0; // Clear IC1 Interrupt Status Flag
    IEC0bits.T2IE = 1; // Enable IC1 interrupt
}
