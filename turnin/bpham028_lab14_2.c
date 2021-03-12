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
#define roof 0xFE //roof row value
#define floor 0xEF //floor row value
#define leftStop 0x40 //pattern to stop on left paddle
#define rightStop 0x02 //pattern to stop on the right paddle

unsigned char tmpB;
unsigned char pattern = 0x00;
unsigned char row = 0x00;
unsigned char userPattern = 0x80;
unsigned char userRow[3] = {0xFD, 0xFB, 0xF7};
unsigned char enemyPattern = 0x01;
unsigned char enemyRow[3] = {0xFD, 0xFB, 0xF7};
unsigned char j = 0; //userRow index
unsigned char k = 0; //enemyRow index
unsigned char ballPattern = 0x00;
unsigned char ballRow = 0x00;
unsigned userPoints = 0x00;
unsigned enemyPoints = 0x00;

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

enum Output_States {outputStart, outputInit, outputUser, outputClear, outputEnemy, outputClear2, outputBall, outputClear3};
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
			state = outputBall;
			break;
		case(outputBall):
			state = outputClear3;
			break;
		case(outputClear3):
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
		case(outputBall):
			pattern = ballPattern;
			row = ballRow;
			break;
		case(outputClear3):
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

enum BallMovement_States {ballStart, ballReset, middleLeft, topRight, middleRight, bottomRight, topLeft, bottomLeft, userPoint, enemyPoint};
int BallMovementTick(int state) {
	
	switch(state) {
		case(ballStart):
			state = ballReset;
			break;
		case(ballReset):
			state = middleLeft;
			break;
		case(middleLeft):
			if (ballPattern < leftStop) {
				state = middleLeft;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[0])) {
				state = topRight;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[1])) {
				state = middleRight;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[2])) {
				state = bottomRight;
			} break;
		case(topRight):
			if ((ballPattern != rightStop) && (ballRow < roof)) {
				state = topRight;
			} else if ((ballPattern > rightStop) && (ballRow >= roof)) {
				state = bottomRight;
			} else if ((ballPattern == rightStop) && ((ballRow == enemyRow[0]) && (ballRow == 0xFE))) {
				state = bottomLeft;
			} else if ((ballPattern == rightStop) && ((ballRow == enemyRow[0]) && (ballRow != 0xFE))) {
				state = topLeft;
			} else if ((ballPattern == rightStop) && (ballRow == enemyRow[1])) { 
				state = middleLeft;
			} else if ((ballPattern == rightStop) && (ballRow == enemyRow[2])) {
				state = bottomLeft;
			} else {
				state = userPoint;
			} break;
		case(middleRight):
			if (ballPattern > rightStop) {
				state = middleRight;
			} else if ((ballPattern <= rightStop) && (ballRow == enemyRow[0])) {
				state = topLeft;
			} else if ((ballPattern <= rightStop) && (ballRow == enemyRow[1])) {
				state = middleLeft;
			} else if ((ballPattern <= rightStop) && (ballRow == enemyRow[2])) {
				state = bottomLeft;
			} break;
		case(bottomRight):
			if ((ballPattern != rightStop) && (ballRow > floor)) {
				state = bottomRight;
			} else if ((ballPattern > rightStop) && (ballRow <= floor)) {
				state = topRight;
			} else if ((ballPattern == rightStop) && (ballRow == enemyRow[0])) {
				state = topLeft;
			} else if ((ballPattern == rightStop) && (ballRow == enemyRow[1])) {
				state = middleLeft;
			} else if ((ballPattern == rightStop) && ((ballRow == enemyRow[2]) && (ballRow == 0xEF))) {
				state = topLeft;
			} else if ((ballPattern == rightStop) && ((ballRow == enemyRow[2]) && (ballRow != 0xEF))) {
				state = bottomLeft;
			} else {
				state = userPoint;
			} break;
		case(topLeft):
			if ((ballPattern != leftStop) && (ballRow < roof)) {
				state = topLeft;
			} else if ((ballPattern < leftStop) && (ballRow >= roof)) {
				state = bottomLeft;
			} else if ((ballPattern == leftStop) && ((ballRow == userRow[0]) && (ballRow == 0xFE))) {
				state = bottomRight;
			} else if ((ballPattern == leftStop) && ((ballRow == userRow[0]) && (ballRow != 0xFE))) {
				state = topRight;
			} else if ((ballPattern == leftStop) && (ballRow == userRow[1])) {
				state = middleRight;
			} else if ((ballPattern == leftStop) && (ballRow == userRow[2])) {
				state = bottomRight;
			} else {
				state = enemyPoint;
			} break;
		case(bottomLeft):
			if ((ballPattern != leftStop) && (ballRow > floor)) {
				state = bottomLeft;
			} else if ((ballPattern < leftStop) && (ballRow <= floor)) {
				state = topLeft;
			} else if ((ballPattern == leftStop) && (ballRow == userRow[0])) {
				state = topRight;
			} else if ((ballPattern == leftStop) && (ballRow == userRow[1])) {
				state = middleRight;
			} else if ((ballPattern == leftStop) && ((ballRow == userRow[2]) && (ballRow == 0xEF))) {
				state = topRight;
			} else if ((ballPattern == leftStop) && ((ballRow == userRow[2]) && (ballRow != 0xEF))) {
				state = bottomRight;
			} else {
				state = enemyPoint;
			} break;
		case(userPoint):
			state = ballReset;
			break;
		case(enemyPoint):
			state = ballReset;
			break;
		default:
			state = ballStart;
			break;
	}
	switch(state) {
		case(ballStart):
			break;
		case(ballReset):
			ballPattern = 0x04;
			ballRow = 0xFB;
			break;
		case(middleLeft):
			ballPattern = ballPattern << 1;
			break;
		case(topRight):
			ballPattern = ballPattern >> 1;
			ballRow = (ballRow >> 1) | 0x80;
			break;
		case(middleRight):
			ballPattern = ballPattern >> 1;
			break;
		case(bottomRight):
			ballPattern = ballPattern >> 1;
			ballRow = (ballRow << 1) | 0x01;
			break;
		case(topLeft):
			ballPattern = ballPattern << 1;
			ballRow = (ballRow >> 1) | 0x80;
			break;
		case(bottomLeft):
			ballPattern = ballPattern << 1;
			ballRow = (ballRow << 1) | 0x01;
			break;
		case(userPoint):
			if (userPoints < 3) {
				++userPoints;
			} else {
				userPoints = 0;
				enemyPoints = 0;
			} break;
		case(enemyPoint):
			if (enemyPoints < 3) {
				++enemyPoints;
			} else {
				enemyPoints = 0;
				userPoints = 0;
			} break;
		default:
			break;
	}
	PORTD = userPoints | (enemyPoints << 3);
	return state;
}

/*enum BallMovement_States {ballStart, ballReset, middleLeft, topRight, middleRight, bottomRight, topLeft, bottomLeft};
int BallMovementTick(int state) {
	
	switch(state) {
		case(ballStart):
			state = ballReset;
			break;
		case(ballReset):
			state = middleLeft;
			break;
		case(middleLeft):
			if (ballPattern < leftStop) {
				state = middleLeft;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[0])) {
				state = topRight;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[1])) {
				state = middleRight;
			} else if ((ballPattern >= leftStop) && (ballRow == userRow[2])) {
				state = bottomRight;
			} break;
		case(topRight):
			if ((ballPattern > rightStop) && (ballRow < roof)) {
				state = topRight;
			} else if ((ballPattern > rightStop) && (ballRow >= roof)) {
				state = bottomRight;
			} break;
		case(middleRight):
			if (ballPattern > rightStop) {
				state = middleRight;
			} break;
		case(bottomRight):
			if ((ballPattern > rightStop) && (ballRow > floor)) {
				state = bottomRight;
			} else if ((ballPattern > rightStop) && (ballRow <= floor)) {
				state = topRight;
			} break;
		default:
			state = ballStart;
			break;
	}
	switch(state) {
		case(ballStart):
			break;
		case(ballReset):
			ballPattern = 0x04;
			ballRow = 0xFB;
			break;
		case(middleLeft):
			ballPattern = ballPattern << 1;
			break;
		case(topRight):
			ballPattern = ballPattern >> 1;
			ballRow = (ballRow >> 1) | 0x80;
			break;
		case(middleRight):
			ballPattern = ballPattern >> 1;
			break;
		case(bottomRight):
			ballPattern = ballPattern >> 1;
			ballRow = (ballRow << 1) | 0x01;
			break;
		default:
			break;
	}
	return state;
}*/

int main(void) {
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	ADC_init();
	
	static task task1, task2, task3, task4;
	task *tasks[] = {&task1, &task2, &task3, &task4};
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
	//ball movement
	task4.state = starter;
	task4.period = 300;
	task4.elapsedTime = task4.period;
	task4.TickFct = &BallMovementTick;
	
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
