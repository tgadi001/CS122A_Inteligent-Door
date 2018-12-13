/*
This code was automatically generated using the Riverside-Irvine State machine Builder tool
Version 2.8 --- 11/26/2018 21:39:58 PST
*/

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "RIMS.h"
#include "usart_ATmega1284.h"

int AutoFlag = 0; //Flag that indicates if Automatic mode is selected
int PrevAutoFlag = -1;//Automatic mode flag that makes sure motor runs once when light changes
int BTFlag = 0;// Flag that checks if there was a Bluetooth signal
const int LockDoor = 1;
const int UnlockDoor = 2;
unsigned char motor[] = {0x93, 0xC6, 0x6C, 0x39}; //1 step for the motor
//unsigned char motor[] = {0x01, 0x02, 0x04, 0x08};



volatile unsigned char TimerFlag=0;
void TimerISR() {
	TimerFlag = 1;
}

//State machine for the Mode selection
enum SM1_States { SM1_Manual, SM1_Auto_mode } SM1_State;
void TickFct_Mode_Select() {
	switch(SM1_State) { // Transitions
		case SM1_Manual:
		if((PINA & 0x02) == 0x02){
			//Checks if the switch is on Automatic mode
			SM1_State = SM1_Auto_mode;
		}
		else if((PINA & 0x02)== 0x00){
			//Keeps the state machine in Manual mode
			SM1_State = SM1_Manual;
			
		}
		break;
		case SM1_Auto_mode:
		if ( (PINA & 0x02) == 0x02) {
			//keeps the state in Automatic mode
			SM1_State = SM1_Auto_mode;
		}
		else if ((PINA & 0x02) == 0x00) {
			//Checks if the switch in Manual mode
			SM1_State = SM1_Manual;
		}
		break;
		default:
		SM1_State = SM1_Manual;
	} // Transitions


	switch(SM1_State) { // State actions
		case SM1_Manual:
		PORTC = 0x02;
		if(BTFlag == LockDoor)
		{
			//Lock signal is received, motor locks and C3 pin LED lights up while locking
			PORTC |= 0x08;
			Motor_AntiClockwise();
			BTFlag = 0;
		}
		else if(BTFlag == UnlockDoor)
		{
			//Unlock signal is received, motor locks and C2 pin LED lights up while locking
			PORTC |= 0x04;
			Motor_Clockwise();
			BTFlag = 0;
		}
		PORTC = 0x02; //reset LEDs
		break;
		case SM1_Auto_mode:
		PORTC |= 0x01;
		if( (AutoFlag == 1) && (PrevAutoFlag != AutoFlag))
		{
			//Unlocks the door when light is sensed and makes sure that motor runs once
			PORTC |= 0x04;
			Motor_Clockwise();
			PrevAutoFlag = AutoFlag;//Makes sure that motor ran once
		}
		else if( (AutoFlag == 0) && (PrevAutoFlag != AutoFlag))
		{
			//Locks the door when light is sensed and
			PORTC |= 0x08;
			Motor_AntiClockwise();
			PrevAutoFlag = AutoFlag; //Makes sure that motor ran once

		}
		else
		{
			PORTC &= 0xF3; //Resets LED
		}
		
		
		break;
		default: // ADD default behaviour below
		break;
	} // State actions
	
}

enum SM2_States { Sensor_Off} SM2_State;
void LightSenorSM() {
	
	//State machine that constantly checks the Light sensor
	
	switch(SM2_State) { // Transitions
		case Sensor_Off:
		SM2_State = Sensor_Off;
		break;
		default:
		SM2_State = Sensor_Off;
	} // Transitions

	switch(SM2_State) { // State actions
		case Sensor_Off:
		if((PINA & 0x01) == 0x01)
		{
			AutoFlag = 0; //No light from sensor
		}
		else{
			AutoFlag = 1; // detected light
		}
		break;
		default:
		break;
	} // State actions
	
}


enum SM3_States { BT_Receive} SM3_State;

void BT_SM(){
	//State machine that checks for Bluetooth input
	switch(SM3_State) { // Transitions
		case BT_Receive:
		SM3_State = BT_Receive;
		break;
		default:
		SM3_State = BT_Receive;
	} // Transitions

	switch(SM3_State) { // State actions
		case BT_Receive:
		if(USART_HasReceived(0))
		{
			if(USART_Receive(0) == 0x00)
			{
				BTFlag = LockDoor;
			}
			else  if(USART_Receive(0) == 0xFF)
			{
				BTFlag = UnlockDoor;
			}
			USART_Flush(0);
		}
		break;
		default: // ADD default behaviour below
		break;
	} // State actions
	
}

//------------------------------------------------
//Functions that handle motor operations
//------------------------------------------------
void Motor_Clockwise(){
	int h = 0;
	for(int i = 0; i < 150; ++i)
	{
		if( h == 4)
		{
			h = 0;
		}
		PORTB = motor[h];
		_delay_ms(20);
		++h;
	}
}


void Motor_AntiClockwise(){
	int h = 3;
	for(int i = 0; i < 150; ++i)
	{
		if( h == -1)
		{
			h = 3;
		}
		PORTB = motor[h];
		_delay_ms(20);
		--h;
	}
}
//------------------------------------------------

int main() {
	
	DDRA = 0x00;
	DDRB = 0xFF;
	DDRC = 0xFF;
	PORTC = 0x00;
	PORTA = 0x00;
	
	initUSART(0);
	USART_Flush(0);
	
	
	//TimerSet(50);
	//TimerOn();
	SM1_State = SM1_Manual;
	SM2_State = Sensor_Off;
	SM3_State = BT_Receive;
	
	
	
	while(1) {
		
		LightSenorSM();
		BT_SM();
		TickFct_Mode_Select();
		
		//while (!TimerFlag){}   // This
		//TimerFlag = 0;         // Lower flag raised by timer
	}
}

