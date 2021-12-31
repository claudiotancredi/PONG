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
int paused = 0;

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
extern int game_status;
extern int ball_position[];

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
	LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,278,10,Green); //paddle
	sprintf(r,"%d", record);
	GUI_Text(233-9*strlen(r),8,(uint8_t*) r, White, Black);
	sprintf(r,"%d",current_score);
	GUI_Text(8,155,(uint8_t*) r, White, Black);
	init_timer(1, 0x17D7840); /*1 s * 25 MHz = 25*10^6=0x17D7840*/
	enable_timer(1);
	return;
}

void pause_game(void){
	if (game_status==1){
	disable_timer(0);
	reset_timer(0);
		if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
			LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,ball_position[1],5,Black);
		}
		GUI_Text(95,155, (uint8_t*) "Paused", White, Black);
		if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
			LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,ball_position[1],5,Green);
		}
		game_status=2;
	}
	else{
		LCD_DrawGameHBorder(95, 155, 150,155, 12, Black);
		if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
			LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,ball_position[1],5,Green);
		}
		game_status=1;
		init_timer(0,0x618A); //1 ms
		enable_timer(0);
	}
	return;
}

void RIT_IRQHandler (void)
{	
	
	if (button==0 && game_status==3){
		if((LPC_GPIO2->FIOPIN & (1<<10)) == 0){
			disable_RIT();
			reset_RIT();
			reset_game();
			game_status=0;
			NVIC_EnableIRQ(EINT0_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 20);
		}
		else{
		button=-1;
		disable_RIT();
		reset_RIT();
		NVIC_EnableIRQ(EINT0_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 20);
	}
	}
	else if(button==1 && game_status==0){
		if((LPC_GPIO2->FIOPIN & (1<<11)) == 0){
			disable_RIT();
			reset_RIT();
			start_game();
			game_status=1;
			NVIC_EnableIRQ(EINT1_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 22);
		}
		else{
		button=-1;
		disable_RIT();
		reset_RIT();
		NVIC_EnableIRQ(EINT1_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 22);
	}
}
	else if(button==2 && paused ==1 && (game_status==1 || game_status==2)){
		paused=0;
		if((LPC_GPIO2->FIOPIN & (1<<12)) == 0){
			disable_RIT();
			reset_RIT();
			pause_game();
			NVIC_EnableIRQ(EINT2_IRQn);
			LPC_PINCON->PINSEL4    |= (1 << 24);
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
