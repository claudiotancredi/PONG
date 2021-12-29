/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_RIT.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    RIT.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "lpc17xx.h"
#include "RIT.h"
#include "../led/led.h"
#include "../GLCD/GLCD.h" 
#include "../timer/timer.h"
#include "../adc/adc.h"
#include <stdio.h>

int paddle_x = 100;

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/

extern int button;
extern int record;
extern int flag_end_game;

int current_score = 0;

void reset_game(void){
	flag_end_game=0;
	LCD_Clear(Black);
	GUI_Text(104, 147, (uint8_t *) "PONG", White, Black);
	GUI_Text(44, 165, (uint8_t *) "Press KEY1 to start", White, Black);
	return;
}

void start_game(void){
	char r[]="";
	ADC_init();	
	LCD_DrawGameHBorder(44,147,230,147,30,Black); //Delete text
	LCD_DrawGameHBorder(0,0,239,0, 5, Red); //Top border
	LCD_DrawGameVBorders(0, 5,278,235,5,Red); //Side borders
	LCD_DrawGameHBorder(paddle_x,278,139,278,10,Green); //paddle
	sprintf(r,"%d", record);
	GUI_Text(233-9*strlen(r),8,(uint8_t*) r, White, Black);
	sprintf(r,"%d",current_score);
	GUI_Text(8,156,(uint8_t*) r, White, Black);
	init_timer(1, 0x17D7840); /*1 s * 25 MHz = 25*10^6=0x17D7840*/
	enable_timer(1);
	return;
}

void RIT_IRQHandler (void)
{	
	
	if (button==0){
		if((LPC_GPIO2->FIOPIN & (1<<10)) == 0){
			reset_game();
		}
		else{
		button=-1;
		disable_RIT();
		reset_RIT();
		NVIC_EnableIRQ(EINT0_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 20);
	}
	}
	else if(button==1){
		if((LPC_GPIO2->FIOPIN & (1<<11)) == 0){
			start_game();
		}
		else{
		button=-1;
		disable_RIT();
		reset_RIT();
		NVIC_EnableIRQ(EINT1_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 22);
	}
}
	else if(button==2){
		if((LPC_GPIO2->FIOPIN & (1<<12)) == 0){
			//pause_game();
		}
		else{
		button=-1;
		disable_RIT();
		reset_RIT();
		NVIC_EnableIRQ(EINT2_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 24);
	}
}
	
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */
	
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
