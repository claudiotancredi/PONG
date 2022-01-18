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
#include "../game/game.h"

extern int button;
extern int game_status;

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/

void RIT_IRQHandler (void)
{	
	static int down_reset = 0;
	static int down_start = 0;
	static int down_pause = 0;
	
	//reset
	if (button==0){
		down_reset++;
		if((LPC_GPIO2->FIOPIN & (1<<10)) == 0 && game_status==3){
			reset_RIT();
			switch(down_reset){
				case 1:
					reset_game();
					break;
				default:
					break;
			}
		}else{
			button=-1;
			down_reset=0;
			disable_RIT();
			reset_RIT();
			NVIC_EnableIRQ(EINT1_IRQn);
		NVIC_EnableIRQ(EINT2_IRQn);
			NVIC_EnableIRQ(EINT0_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 20);
		}
		}//start
	else if(button==1){
		down_start++;
		if((LPC_GPIO2->FIOPIN & (1<<11)) == 0  && game_status==0){
			reset_RIT();
			switch(down_start){
				case 1:
					start_game();
					break;
				default:
					break;
			}
		}else{
			button=-1;
			down_start=0;
			disable_RIT();
			reset_RIT();
			NVIC_EnableIRQ(EINT1_IRQn);
			NVIC_EnableIRQ(EINT0_IRQn);
		NVIC_EnableIRQ(EINT2_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 22);
		}
}//pause
	else if(button==2){
		down_pause++;
		if((LPC_GPIO2->FIOPIN & (1<<12)) == 0  && (game_status==1 || game_status==2)){
			reset_RIT();
			switch(down_pause){
				case 1:
					pause_game();
					break;
				default:
					break;
			}
		}else{
			button=-1;
			down_pause=0;
			disable_RIT();
			reset_RIT();
			NVIC_EnableIRQ(EINT2_IRQn);
			NVIC_EnableIRQ(EINT0_IRQn);
		NVIC_EnableIRQ(EINT1_IRQn);
		LPC_PINCON->PINSEL4    |= (1 << 24);
		}
	}
	
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */
	
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
