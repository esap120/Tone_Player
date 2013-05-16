// Evan Sapienza
// 3-20-13
// Tone Player
// Toggled on and off with a button
//-----------------------
#include "msp430g2553.h"
//-----------------------
// define the bit mask (within P1) corresponding to output TA0
#define TA0_BIT 0x02

// define the port and location for the button (this is the built in button)
// specific bit for the button
#define BUTTON_BIT 0x08
//----------------------------------

// Some global variables (mainly to look at in the debugger)
volatile unsigned halfPeriod; // half period count for the timer
volatile unsigned long intcount=0; // number of times the interrupt has occurred
volatile unsigned soundOn=0; // state of sound: 0 or OUTMOD_4 (0x0080)

volatile unsigned minHP=100;  // minimum half period
volatile unsigned maxHP=2000; // maximum half period
volatile int deltaHP=-1; // step in half period per half period

int counter = 0;
int button_counter = 0;

void init_timer(void); // routine to setup the timer
void init_button(void); // routine to setup the button

// ++++++++++++++++++++++++++
void main(){
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ; // 1Mhz calibration for clock
	DCOCTL = CALDCO_1MHZ;

	halfPeriod=maxHP; // initial half-period at lowest frequency
	init_timer(); // initialize timer
	init_button(); // initialize the button
	_bis_SR_register(GIE+LPM0_bits);// enable general interrupts and power down CPU
}

// +++++++++++++++++++++++++++
// Sound Production System
void init_timer(){ // initialization and start of timer
	TA0CTL |=TACLR; // reset clock
	TA0CTL =TASSEL1+ID_0+MC_2; // clock source = SMCLK, clock divider=1, continuous mode,
	TA0CCTL0=soundOn+CCIE; // compare mode, outmod=sound, interrupt CCR1 on
	TA0CCR0 = TAR+halfPeriod; // time for first alarm
	P1SEL|=TA0_BIT; // connect timer output to pin
	P1DIR|=TA0_BIT;
}

// +++++++++++++++++++++++++++
void interrupt sound_handler(){
	TACCR0 += halfPeriod; // advance 'alarm' time
	if (soundOn){ // change half period if the sound is playing
		if((button_counter % 4 == 0) || (button_counter % 4 == 1)) {
			counter++;
			if (counter < 2000){
				halfPeriod = 294;
			}
			else if (counter < 4000){
				halfPeriod = 262;
			}
			else if (counter < 6000){
				halfPeriod = 247;
			}
			else if (counter < 8000){
				halfPeriod = 220;
			}
			else {
				counter = 0;
			}
		}
		else{
			halfPeriod += deltaHP; 			// adjust the period
			if (halfPeriod<minHP){			// check min
				deltaHP=-deltaHP;			// reverse direction
				halfPeriod = minHP;			// stay at minHP
			} else if (halfPeriod>maxHP){	/// check max
				deltaHP=-deltaHP;			// reverse direction
				halfPeriod=maxHP;			// stick at max
			}
		}
	}
	TA0CCTL0 = CCIE + soundOn; //  update control register with current soundOn
	++intcount; // advance debug counter
}
ISR_VECTOR(sound_handler,".int09") // declare interrupt vector

// +++++++++++++++++++++++++++
// Button input System
// Button toggles the state of the sound (on or off)
// action will be interrupt driven on a downgoing signal on the pin
// no debouncing (to see how this goes)

void init_button(){
// All GPIO's are already inputs if we are coming in after a reset
	P1OUT |= BUTTON_BIT; // pullup
	P1REN |= BUTTON_BIT; // enable resistor
	P1IES |= BUTTON_BIT; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT;// clear interrupt flag
	P1IE  |= BUTTON_BIT; // enable interrupt
}
void interrupt button_handler(){
// check that this is the correct interrupt
// (if not, it is an error, but there is no error handler)
	if (P1IFG & BUTTON_BIT){
		P1IFG &= ~BUTTON_BIT; // reset the interrupt flag
		soundOn ^= OUTMOD_4; // flip bit for outmod of sound
	}
	counter = 0;
	button_counter++;
}
ISR_VECTOR(button_handler,".int02") // declare interrupt vector
// +++++++++++++++++++++++++++
