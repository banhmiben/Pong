/*      Author: lab pong
 *  Partner(s) Name: 
 *      Lab Section:
 *      Assignment: Lab #14  Exercise #
 *      Exercise Description: [optional - include for your own benefit]
 *
 *      I acknowledge all content contained herein, excluding template or example
 *      code, is my own original work.
 */

#include <avr/io.h>
#include <timer.h>
#include <scheduler.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#define shiftC 0x01 //pattern
#define shiftD 0x02 //row

unsigned char tmpA;
unsigned char pattern = 0x00;
unsigned char row = 0x00;
unsigned char userPattern = 0x80;
unsigned char userRow[3] = {0xFD, 0xFB, 0xF7};
unsigned char j = 0; //userRow index

void Set_A2D_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	static unsigned char i = 0;
	for (i = 0; i < 15; i++) { asm("nop"); }
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	//ADEN: setting this bit enables analog-to-digital conversion
	//ADSC: setting this bit starts the first conversion.
	//ADATE: setting this bit enables auto-triggering. Since we are in
			// Free Running Mode, a new conversion will trigger whenever the previous conversion completes
}

void transmit_data(unsigned char data, unsigned char shiftNum) {
	int i;
	for (i = 0; i < 8; ++i) {
		//Sets SRCLR to 1 allowing data to be set 
		//Also clears SRCLK in preparation of sending data (shift1 SRCLR = 0x08 shift2 SRCLR = 0x20)
		if (shiftNum == shiftC) {
			PORTC = 0x08;
		} else if (shiftNum == shiftD) {
			PORTC = 0x20;
		}
		//set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		//set SRCLK = 1. Rising edge shifts next bit of data into the shift register 
		PORTC |= 0x02;
	}
	//set RCLK = 1. Rising edge copies data from "Shift" register to "Storage" register
	if (shiftNum == shiftC) {
		PORTC |= 0x04;
	} else if (shiftNum == shiftD) {
		PORTC |= 0x10;
	}
	//clears all lines in preparation of a new transmission
	PORTC = 0x00;
} 

enum UserShift_States {userStart, userInit, userWait, up, down};
int UserPaddleShiftTick(int state) {
	
	Set_A2D_Pin(0x01);
	userPattern = 0x80;
	switch(state) {
		case(userStart):
			state = userInit;
			break;
		case(userInit):
			state = userWait;
			break;
		case(userWait):
			if (ADC > 542 + 100) {
				state = up;
				j = 0;
			} else if (ADC < 542 - 100) {
				state = down;
				j = 2;
			} else {
				state = userWait;
			} break;
		case(up):
			state = userWait;
			break;
		case(down):
			state = userWait;
			break;
		default:
			state = userStart;
			break;
	}
	switch(state) {
		case(userStart):
			break;
		case(userInit):
		//	PORTB = 0x01;
			break;
		case(userWait):
		//	PORTB = 0x02;
			break;
		case(up):
		//	PORTB = 0x04;
			if (userRow[j] < 0xFE) {
				userRow[j] = (userRow[j] >> 1) | 0x80;
				++j;
				userRow[j] = (userRow[j] >> 1) | 0x80;
				++j;
				userRow[j] = (userRow[j] >> 1) | 0x80;
			} break;
		case(down):
		//	PORTB = 0x08;
			if (userRow[j] > 0xEF) {
				userRow[j] = (userRow[j] << 1) | 0x01;
				--j;
				userRow[j] = (userRow[j] << 1) | 0x01;
				--j;
				userRow[j] = (userRow[j] << 1) | 0x01;
			} break;
		default:
			break;
	}
	return state;
}

enum Output_States {outputStart, outputInit, output};
int CombineOutputTick(int state) {

	PORTB = 0x04;	
	switch(state) {
		case(outputStart):
			state = outputInit;
			break;
		case(outputInit):
			state = output;
			break;
		case(output):
			state = outputInit;
			break;
		default:
			break;
	}
	switch(state) {
		case(outputStart):
			break;
		case(outputInit):
			pattern = userPattern;
			row = userRow[0] & userRow[1] & userRow[2];
			//PORTB = 0x01;
			break;
		case(output):
			//PORTB = 0x02;
			break;
		default:
			break;
	}
	transmit_data(pattern, shiftC);
	transmit_data(row, shiftD);
	return state;
}

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	//DDRD = 0xFF; PORTD = 0x00;
	ADC_init();
	
	static task task1, task2;
	task *tasks[] = {&task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	const char starter = -1;
	//userpaddle shift
	task1.state = starter;
	task1.period = 200;
	task1.elapsedTime = task1.period;
	task1.TickFct = &UserPaddleShiftTick;
	//led displays
	task2.state = outputStart;
	task2.period = 1;
	task2.elapsedTime = task2.period;
	task2.TickFct = &CombineOutputTick;
	
	TimerSet(1);
	TimerOn();
	unsigned short i; //scheduler loop iterator

	while(1) {
		for (i = 0; i < numTasks; i++) {
			if (tasks[i] -> elapsedTime == tasks[i] -> period) {
				tasks[i] -> state = tasks[i] -> TickFct(tasks[i] -> state);
				tasks[i] -> elapsedTime = 0;
			}
			tasks[i] -> elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0;

}
