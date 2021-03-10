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

unsigned char tmpB;
unsigned char pattern = 0x00;
unsigned char row = 0x00;
unsigned char userPattern = 0x80;
unsigned char userRow[3] = {0xFD, 0xFB, 0xF7};
unsigned char enemyPattern = 0x01;
unsigned char enemyRow[3] = {0xFD, 0xFB, 0xF7};
unsigned char j = 0; //userRow index
unsigned char k = 0; //enemyRow index

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

enum EnemyShift_States {enemyStart, enemyInit, enemyWait, enemyUp, enemyDown};
int EnemyPaddleShiftTick(int state) {
	
	enemyPattern = 0x01;
	switch(state) {
		case(enemyStart):
			state = enemyInit;
			break;
		case(enemyInit):
			state = enemyWait;
			break;
		case(enemyWait):
			if (tmpB == 0x01) {
				state = enemyUp;
				k = 0;
			} else if (tmpB == 0x02) {
				state = enemyDown;
				k = 2;
			} else {
				state = enemyWait;
			} break;
		case(enemyUp):
			state = enemyWait;
			break;
		case(enemyDown):
			state = enemyWait;
			break;
		default:
			state = enemyStart;
			break;
	}
	switch(state) {
		case(enemyStart):
			break;
		case(enemyInit):
		//	PORTB = 0x01;
			break;
		case(enemyWait):
		//	PORTB = 0x02;
			break;
		case(enemyUp):
		//	PORTB = 0x04;
			if (enemyRow[k] < 0xFE) {
				enemyRow[k] = (enemyRow[k] >> 1) | 0x80;
				++k;
				enemyRow[k] = (enemyRow[k] >> 1) | 0x80;
				++k;
				enemyRow[k] = (enemyRow[k] >> 1) | 0x80;
			} break;
		case(enemyDown):
		//	PORTB = 0x08;
			if (enemyRow[k] > 0xEF) {
				enemyRow[k] = (enemyRow[k] << 1) | 0x01;
				--k;
				enemyRow[k] = (enemyRow[k] << 1) | 0x01;
				--k;
				enemyRow[k] = (enemyRow[k] << 1) | 0x01;
			} break;
		default:
			break;
	}
	return state;
}

enum Output_States {outputStart, outputInit, outputUser, outputClear, outputEnemy, outputClear2};
int CombineOutputTick(int state) {

	switch(state) {
		case(outputStart):
			state = outputInit;
			break;
		case(outputInit):
			state = outputUser;
			break;
		case(outputUser):
			state = outputClear;
			break;
		case(outputClear):
			state = outputEnemy;
			break;
		case(outputEnemy):
			state = outputClear2;
			break;
		case(outputClear2):
			state = outputUser;
			break;
		default:
			break;
	}
	switch(state) {
		case(outputStart):
			break;
		case(outputInit):
			//pattern = userPattern | enemyPattern;
			//row = (userRow[0] & userRow[1] & userRow[2]) | (enemyRow[0] & enemyRow[1] & enemyRow[2]);
			//PORTB = 0x01;
			break;
		case(outputUser):
			pattern = userPattern;
			row = userRow[0] & userRow[1] & userRow[2];
	//		transmit_data(pattern, shiftC);
	//		transmit_data(row, shiftD);
			break;
		case(outputClear):
			pattern = 0x00;
			row = 0xFF;
			break;
		case(outputEnemy):
			pattern = enemyPattern;
			row = enemyRow[0] & enemyRow[1] & enemyRow[2];
	//		transmit_data(pattern, shiftC);
	//		transmit_data(row, shiftD);
			break;
		case(outputClear2):
			pattern = 0x00;
			row = 0xFF;
			break;
		default:
			break;
	}
	transmit_data(pattern, shiftC);
	transmit_data(row, shiftD);
	return state;
}


/*enum Output_States {outputStart, outputInit, userTop, userMiddle, userBottom, clearUser, enemyTop, enemyMiddle, enemyBottom, clearEnemy};
int CombineOutputTick(int state) {
	
	switch(state) {
		case(outputStart):
			state = outputInit;
			break;
		case(outputInit):
			state = userTop;
			break;
		case(userTop):
			state = userMiddle;
			break;
		case(userMiddle):
			state = userBottom;
			break;
		case(userBottom):
			state = clearUser;
			break;
		case(clearUser):
			state = enemyTop;
			break;
		case(enemyTop):
			state = enemyMiddle;
			break;
		case(enemyMiddle):
			state = enemyBottom;
			break;
		case(enemyBottom):
			state = clearEnemy;
			break;
		case(clearEnemy):
			state = userTop;
			break;
		default:
			state = outputStart;
			break;
	}
	switch(state) {
		case(outputStart):
			break;
		case(outputInit):
			break;
		case(userTop):
			pattern = userPattern;
			row = userRow[0];
			break;
		case(userMiddle):
			row = userRow[1];
			break;
		case(userBottom):
			row = userRow[2];
			break;
		case(clearUser):
			pattern = 0x00;
			row = 0xFF;
			break;
		case(enemyTop):
			pattern = enemyPattern;
			row = enemyRow[0];
			break;
		case(enemyMiddle):
			row = enemyRow[1];
			break;
		case(enemyBottom):
			row = enemyRow[2];
			break;
		case(clearEnemy):
			pattern = 0x00;
			row = 0xFF;
			break;
		default:
			break;
	}
	transmit_data(pattern, shiftC);
	transmit_data(row, shiftD);
	return state;
}
*/

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	//DDRD = 0xFF; PORTD = 0x00;
	ADC_init();
	
	static task task1, task2, task3;
	task *tasks[] = {&task1, &task2, &task3};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	const char starter = -1;
	//userpaddle shift
	task1.state = starter;
	task1.period = 100;
	task1.elapsedTime = task1.period;
	task1.TickFct = &UserPaddleShiftTick;
	//led displays
	task2.state = outputStart;
	task2.period = 1;
	task2.elapsedTime = task2.period;
	task2.TickFct = &CombineOutputTick;
	//enemypaddle shift
	task3.state = starter;
	task3.period = 100;
	task3.elapsedTime = task3.period;
	task3.TickFct = &EnemyPaddleShiftTick;
	
	TimerSet(1);
	TimerOn();
	unsigned short i; //scheduler loop iterator

	while(1) {
		tmpB = ~PINB;
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
