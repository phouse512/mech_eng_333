//#define NU32_STANDALONE  // uncomment if program is standalone, not bootloaded
#include "NU32.h"          // config bits, constants, funcs for startup and UART

#define MAX_MESSAGE_LENGTH 200

int main(void) {
  char message[MAX_MESSAGE_LENGTH];
  
  NU32_Startup(); // cache on, interrupts on, LED/button init, UART init
  sprintf(message, "Hello World! \r\n");
  NU32_WriteUART1(message);
  while (1) {
    sprintf(message, "Hello?  What do you want to tell me?  ");
    NU32_WriteUART1(message);
    NU32_ReadUART1(message, MAX_MESSAGE_LENGTH);
    NU32_WriteUART1(message);
    NU32_WriteUART1("\r\n");
    NU32_LED1 = !NU32_LED1; // toggle the LEDs
    NU32_LED2 = !NU32_LED2;
  }
  return 0;
}
