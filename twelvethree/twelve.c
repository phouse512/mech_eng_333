//#define NU32_STANDALONE  // uncomment if program is standalone, not bootloaded
#include "NU32.h"          // config bits, constants, funcs for startup and UART

#define MAX_MESSAGE_LENGTH 200
#define NUMSAMPS 1000
volatile int Waveform[NUMSAMPS];

void makeWaveform() {
  int i, center=2000, A=1000; // square wave, amplitude A centered at center
  for (i=0; i<NUMSAMPS; i++) {
    if (i<NUMSAMPS/2) Waveform[i] = center + A;
    else Waveform[i] = center - A;
  }
}

void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1ISR(void) { // INT step 1: the ISR
  LATAINV = 0x20; // toggle RA5
  IFS0bits.T1IF = 0; // clear interrupt flag
}

int main(void) {
  char message[MAX_MESSAGE_LENGTH];
  
  NU32_Startup(); // cache on, interrupts on, LED/button init, UART init


  __builtin_disable_interrupts(); // INT step 2: disable interrupts at CPU
  // INT step 3: setup peripheral
  PR1 = 30000; // set period register
  TMR1 = 0; // initialize count to 0
  T1CONbits.TCKPS = 3; // set prescaler to 256
  T1CONbits.TGATE = 0; // not gated input (the default)
  T1CONbits.TCS = 0; // PCBLK input (the default)
  T1CONbits.ON = 1; // turn on Timer1
  IPC1bits.T1IP = 5; // INT step 4: priority
  IPC1bits.T1IS = 0; // subpriority
  IFS0bits.T1IF = 0; // INT step 5: clear interrupt flag
  IEC0bits.T1IE = 1; // INT step 6: enable interrupt
  __builtin_enable_interrupts(); // INT step 7: enable interrupts at CPU
  while (1) {
    ;
  }

  //makeWaveform();
  return 0;
}
