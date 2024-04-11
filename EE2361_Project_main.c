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
    
    TRISBbits.TRISB8 = 1;       // Button input
    TRISBbits.TRISB7 = 1;       // Piezo input
	LATA = 0xffff;              // Set all of port A to HIGH
	LATB = 0xffff;              // and all of port B to HIGH
}

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

void loop() {
  // read the sensor and store it in the variable sensorReading:
  sensorReading = analogRead(knockSensor);

  // if the sensor reading is greater than the threshold:
  if (sensorReading >= threshold) {
    // toggle the status of the ledPin:
    ledState = !ledState;
    // update the LED pin itself:
    digitalWrite(ledPin, ledState);
    // send the string "Knock!" back to the computer, followed by newline
    Serial.println("Knock!");
  }
  delay(100);  // delay to avoid overloading the serial port buffer
}
