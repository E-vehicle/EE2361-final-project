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

// Define some constants for the threshold values
#define SOFT_THRESHOLD 50
#define LOUD_THRESHOLD 0

// Define variables to keep track of the current state of the sequence
int sequence_index = 0;
int sequence[] = {SOFT_THRESHOLD, LOUD_THRESHOLD, SOFT_THRESHOLD};
int sequence_length = sizeof(sequence) / sizeof(sequence[0]);
int previous_adValue = -1; // Variable to store the previous ADC value

void __attribute__ ((interrupt, auto_psv)) _ADC1Interrupt(void) {
    int adValue = ADC1BUF0;

    // Check if the current value is greater than 1 and has changed
    if (adValue > 1 && adValue != previous_adValue) {
        // Check if the current value matches the expected value in the sequence
        if (check_sequence(adValue)) {
            // If the current value matches the expected value in the sequence
            sequence_index++; // Move to the next element in the sequence
            if (sequence_index >= sequence_length) {
                LATBbits.LATB13 = 1;
                delay(500);
                LATBbits.LATB13 = 0;
                sequence_index = 0;
                return;
            }
        } else {
            // If the current value does not match the expected value in the sequence
            // Reset the sequence index to start over
            sequence_index = 0;
        }
    }

    // Perform your existing code based on adValue
    if (adValue >= 50) {
        LATBbits.LATB14 = 1;
        delay(500);
        LATBbits.LATB14 = 0;
    } else if (adValue > 0 && adValue < 50) {
        LATBbits.LATB15 = 1;
        delay(500);
        LATBbits.LATB15 = 0;
    } else if (adValue >= 50) {
        LATBbits.LATB13 = 1;
        delay(500);
        LATBbits.LATB13 = 0;
    }

    // Update the previous ADC value
    previous_adValue = adValue;

    _AD1IF = 0; // Clear ADC interrupt flag
}

// Function to check if the current value matches the expected value in the sequence
int check_sequence(int adValue) {
    switch (sequence_index) {
        case 0:
            return (adValue > LOUD_THRESHOLD && adValue < SOFT_THRESHOLD);
        case 1:
            return (adValue >= SOFT_THRESHOLD);
        case 2:
            return (adValue > LOUD_THRESHOLD && adValue < SOFT_THRESHOLD);
        default:
            return 0;
    }
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
	LATBbits.LATB14 = 0;              // and all of port B to HIGH

	initPiezo();
	initServo();
}

void initPiezo(void) {

    T2CONbits.TON = 0;
    T2CONbits.TCKPS = 0b11;
    T2CONbits.TCS = 0b0;
    T2CONbits.TGATE = 0b0;
    TMR2 = 0;
    // Initialize to zero (also best practice)
    PR2 = 0xF424; // Set period to one second
    T2CONbits.TON = 1; // Start 16-bit Timer2

    TRISAbits.TRISA0 = 1;
    
    AD1PCFGbits.PCFG0 = 0;
    
    
    AD1CON2bits.VCFG = 0;
    AD1CON3bits.ADCS = 0b00000011;
    AD1CON1bits.SSRC = 0b010;
    AD1CON3bits.SAMC = 0b00011;
    AD1CON1bits.FORM = 0b00;
    
    AD1CON1bits.ASAM = 1;
    AD1CON2bits.SMPI = 0b0000;
    AD1CON1bits.ADON = 1; //turn it on 
    
    _AD1IF = 0;
    _AD1IE = 1;
    
    TMR3 = 0;
    T3CON = 0;
    T3CONbits.TCKPS = 0b10;
    PR3 = 15624;
    T3CONbits.TON = 1;
    
}

void delay(unsigned int ms) {
    int i;
    for (i = 0; i < ms; i++) {
        asm("repeat #15993");
        asm("nop");
    }
    return;
}

int main(void) {
	setup();
	setServo(3600);
	while (1) {

    }
    return 0;
}
