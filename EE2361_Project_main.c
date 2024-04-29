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


// Define some constants for the threshold values
// Thresholds determined by trial and error
#define SOFT_THRESHOLD 1
#define LOUD_THRESHOLD 50

// Define variables to keep track of the current state of the sequence
int state = 0;
int sequence[] = {SOFT_THRESHOLD, LOUD_THRESHOLD, SOFT_THRESHOLD}; // put combination into this value
int sequence_length = sizeof(sequence) / sizeof(sequence[0]); // calculates the sequence length of the combination

void __attribute__ ((interrupt, auto_psv)) _ADC1Interrupt(void) {
    int adValue = ADC1BUF0;
    
    // Check if the current value is greater than 1
    if (adValue > 1) {
        // Check if the current value matches the expected value in the sequence
        if (check_sequence(adValue)) {
            // If the current value matches the expected value in the sequence
            state++; // Move to the next element in the sequence
            if (state == sequence_length) { // this will run when correct sequence is inputted
                LATBbits.LATB13 = 0; // turns off red light
                LATBbits.LATB15 = 1; // turns on green light
                setServo(3000); // sets the servo to unlocked
                state = 0; // resets the locking code state to 0
                return;
            }
        } 
       // if you change the combonation you may need to add some more edge case detectors
	else if(state == 1 && adValue > SOFT_THRESHOLD && adValue < LOUD_THRESHOLD){  // case where soft then soft is inputted instead of restarting state
            state = 1;
        }
        else {
            // If the current value does not match the expected value in the sequence
            // Reset the sequence index to start over
            state = 0;
        }
    }

    // turns on lights to display if it is a soft or loud knock
    // can be removed if you do not want to display it and then delay would be very small
    if (adValue >= 50) {
        LATBbits.LATB14 = 1;
        delay(250);
        LATBbits.LATB14 = 0;
        delay(250);
        LATBbits.LATB14 = 1;
        delay(250);
        LATBbits.LATB14 = 0;
    } else if (adValue > 1 && adValue < 50) {
        LATBbits.LATB14 = 1;
        delay(750);
        LATBbits.LATB14 = 0;
    }


    _AD1IF = 0; // Clear ADC interrupt flag
}


// Function to check if the current value matches the expected value in the sequence
// add switch states to create more inputs to the combination
// change return values to cahnge up loud or soft
int check_sequence(int adValue) {
    switch (state) {
        case 0:
            return (adValue > SOFT_THRESHOLD && adValue < LOUD_THRESHOLD); // soft knock
        case 1:
            return (adValue >= LOUD_THRESHOLD); // loud knock
        case 2:
            return (adValue > SOFT_THRESHOLD && adValue < LOUD_THRESHOLD); // soft knock
        default:
            return 0;
    }
}



// function to setup inputs and outputs
void setup() {
	
    CLKDIVbits.RCDIV = 0;  //Set RCDIV=1:1 (default 2:1) 32MHz or FCY/2=16M
    AD1PCFG = 0x9fff;            //sets all pins to digital I/O
    TRISBbits.TRISB15 = 0;      // Green LED output
    TRISBbits.TRISB14 = 0;      // Yellow LED output
    TRISBbits.TRISB13 = 0;      // Red LED output
    TRISBbits.TRISB6 = 0;       // Servo motor output
    
    
    
    TRISBbits.TRISB8 = 1;       // Button input
    CNPU2bits.CN22PUE = 1;
    LATBbits.LATB14 = 0;        // and all of port B to HIGH

    initPiezo();
    initServo();
}

//function to initialize the Piezo
void initPiezo(void) {

    T2CONbits.TON = 0;
    T2CONbits.TCKPS = 0b11;
    T2CONbits.TCS = 0b0;
    T2CONbits.TGATE = 0b0;
    TMR2 = 0;  // Initialize to zero (also best practice)
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
    AD1CON1bits.ADON = 1; // turn it on 
    
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

// main function
int main(void) {
	setup();
	setServo(5100); // initialize as locked
    LATBbits.LATB13 = 1;  // initializes red light to indicate locked
	while (1) {
		
            if(PORTBbits.RB8 == 0){  // if reset button is pressed
            	setServo(5100); // lock the servo
           	 LATBbits.LATB13 = 1; // turn red light on
            	LATBbits.LATB15 = 0; // turn green light off
        }
    }
    return 0;
}
