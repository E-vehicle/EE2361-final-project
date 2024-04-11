#include "xc.h"
#include "Servo.h"

void initServo(void) {
    T3CON = 0x0010;
    TMR3 = 0;
    PR3 = 40000;
    T3CONbits.TON = 1;
    
    __builtin_write_OSCCONL(OSCCON & 0xbf);
    RPOR3bits.RP6R = 18;
    __builtin_write_OSCCONL(OSCCON | 0x40);
    
    OC1CON = 0;    // turn off OC1 for now
    OC1R = 1234;   // servo start position. We won?t touch OC1R again
    OC1RS = 1234;  // We will only change this once PWM is turned on
    OC1CONbits.OCTSEL = 1; // Use Timer 3 for compare source
    OC1CONbits.OCM = 0b110; // Output compare PWM w/o faults
}

void setServo(int Val) {
    OC1RS = Val;
}
