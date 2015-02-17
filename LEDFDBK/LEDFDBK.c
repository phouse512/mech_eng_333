//#define NU32_STANDALONE // uncomment if program is standalone, not bootloaded
#include "NU32.h" // config bits, constants, funcs for startup and UART
#include "LCD.h"

#define NUMSAMPS 1000
#define PLOTPTS 200 // number of data points to plot
#define DECIMATION 10 // plot every 10th point
#define SAMPLE_TIME 10 // 250ns for sampling
volatile int Waveform[NUMSAMPS];
volatile int ADCarray[PLOTPTS]; // measured values to plot
volatile int REFarray[PLOTPTS]; // reference values to plot
volatile int StoringData = 0; // if this flag = 1, currently storing plot data
volatile float Kp = 0, Ki = 0; // control gains
volatile int Eint = 0, Eprev = 0;

void printGainsToLCD() {
	char screen_message_Kp[100], screen_message_Ki[100];
	LCD_Clear();                              // clear LCD screen
    LCD_Move(0,0);
    sprintf(screen_message_Kp, "Kp = %d", Kp);
    sprintf(screen_message_Ki, "Ki = %d", Ki);
    LCD_WriteString(screen_message_Kp);                     // write msg at row 0 col 0
    LCD_Move(0,6);
    //LCD_WriteString((char *) Kp);
    LCD_Move(1,0);
    LCD_WriteString(screen_message_Ki);
    LCD_Move(1,6);
}

void makeWaveform() {
	int i, center=500, A=300; // square wave, amplitude A centered at center
	for (i=0; i<NUMSAMPS; i++) {
		if (i<NUMSAMPS/2) Waveform[i] = center + A;
		else Waveform[i] = center - A;
	}
}

unsigned int adc_sample_convert() {
	unsigned int elapsed = 0, finish_time = 0;
	AD1CHSbits.CH0SA = 0;
	AD1CON1bits.SAMP = 1;
	elapsed = _CP0_GET_COUNT();
	finish_time = elapsed + SAMPLE_TIME;
	while (_CP0_GET_COUNT() < finish_time) {
		;
	}
	AD1CON1bits.SAMP = 0;
	while (!AD1CON1bits.DONE) {
		;
	}

	return ADC1BUF0;
}

// unsigned int PI_controller(int adcval, int expected) {

// }

// eprev = 0; // initial "previous error" is zero
// eint = 0; // initial error integral is zero
// now = 0; // "now" tracks the elapsed time
// every dt seconds do {
// 	s = readSensor(); // read sensor value
// 	r = ref erenceValue(now); // get reference signal for time "now"
// 	e = r - s; // calculate the error
// 	edot = e - eprev; // error difference
// 	eint = eint + e; // error sum
// 	u = Kp*e + Ki*eint + Kd*edot; // calculate the control signal
// 	sendControl(u); // send control signal to the plant
// 	eprev = e; // current error is now previous error
// 	now = now + dt; // update the "now" time
// }




void __ISR(_TIMER_2_VECTOR, IPL5SOFT) Controller(void) { // _TIMER_2_VECTOR = 8 (p32mx795f512l.h)
	static int counter = -1; // initialize counter once
	static int plotind = 0; // index for data arrays; counts up to PLOTPTS
	static int decctr = 0; // counts to store data one every DECIMATION
	static int adcval = 0; //
	counter++; // add one to counter every time ISR is entered
	if (counter==NUMSAMPS) counter = -1; // roll the counter over when needed
	OC1RS = Waveform[counter];// insert line(s) to set OC1RS
	adcval = adc_sample_convert();
	if (StoringData) {
		decctr++;
		if (decctr == DECIMATION) { // after DECIMATION control loops,
			decctr = 0; // reset decimation counter
			ADCarray[plotind] = adcval; // store data in global arrays
			REFarray[plotind] = Waveform[counter];
			plotind++; // increment plot data index
		}
		if (plotind == PLOTPTS) { // if max number of plot points plot is reached,
			plotind = 0; // reset the plot index
			StoringData = 0; // tell main data is ready to be sent to Matlab
		}
	}

	IFS0bits.T2IF = 0;
}



void main(void) {
	NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
	LCD_Setup();

	char message[100]; // message to and from Matlab
	float kptemp = 0, kitemp = 0; // temporary local gains
	int i = 0; // plot data loop counter
	makeWaveform();

	__builtin_disable_interrupts(); // INT step 2: disable interrupts at CPU
	T3CONbits.TCKPS = 3; //3// set prescaler to 256
	PR3 = 3999; //3999  set period register
	TMR3 = 0; // initialize count to 0
	OC1CONbits.OCM = 0b110;  // PWM mode without fault pin; other OC1CON bits are defaults
	OC1CONbits.OC32 = 0;
	OC1CONbits.OCTSEL = 1;
	OC1RS = 3000;
	OC1R = 500;
	T3CONbits.ON = 1; // turn on Timer3
	OC1CONbits.ON = 1;

	T2CONbits.TGATE = 0; // not gated input (the default)
	T2CONbits.TCS = 0; // PCBLK input (the default)
	T2CONbits.TCKPS = 2;
	PR2 = 99999;		//Set Timer 2 for 1kHz
	T2CONbits.ON = 1; // turn on Timer3
	IPC2bits.T2IP = 5; // INT step 4: priority
	IPC2bits.T2IS = 0; // subpriority
	IFS0bits.T2IF = 0; // INT step 5: clear interrupt flag
	IEC0bits.T2IE = 1; // INT step 6: enable interrupt

	__builtin_enable_interrupts(); // INT step 7: enable interrupts at CPU


	AD1PCFGbits.PCFG0 = 0;
	AD1CON3bits.ADCS = 2;
	AD1CON1bits.ADON = 1;

	while (1) {
		NU32_ReadUART1(message,99); // wait for a message from Matlab
		sscanf(message, "%f %f", &kptemp, &kitemp);
		__builtin_disable_interrupts(); // keep ISR disabled as briefly as possible
		Kp = kptemp; // copy local variables to globals used by ISR
		Ki = kitemp;
		Eint = 0;
		__builtin_enable_interrupts(); // only 2 simple C commands while ISRs disabled
		StoringData = 1; // message to ISR to start storing data
		printGainsToLCD();
	    //LCD_WriteString((char *) Ki);
		while (StoringData) { // wait until ISR says data storing done
			; // do nothing
		}
		for (i=0; i<PLOTPTS; i++) { // send plot data to Matlab
			// when first number sent = 1, done sending data
			sprintf(message, "%d %d %d\r\n", PLOTPTS-i, ADCarray[i], REFarray[i]);
			NU32_WriteUART1(message);
		}
	}
}




