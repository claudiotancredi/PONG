#include "button.h"
#include "lpc17xx.h"

//	#include "../led/led.h" 					/* do not needed anymore, make your project clean */
#include "../RIT/RIT.h"		  			/* you now need RIT library 			 */


int button = -1;
int game_status = 0; //0 means not started (or reset), 1 means started, 2 means paused, 3 means lose
extern int paused;

void EINT0_IRQHandler (void)	  	/* INT0														 */
{		
		NVIC_DisableIRQ(EINT0_IRQn);		/* disable Button interrupts			 */
		button=0;
		init_RIT(0x004C4B40); //50 ms
	LPC_PINCON->PINSEL4    &= ~(1 << 20);     /* GPIO pin selection */
		enable_RIT();										/* enable RIT to count 50ms				 */
	LPC_SC->EXTINT &= (1 << 0);     /* clear pending interrupt         */
}


void EINT1_IRQHandler (void)	  	/* KEY1														 */
{
		NVIC_DisableIRQ(EINT1_IRQn);		/* disable Button interrupts			 */
		button=1;
		init_RIT(0x004C4B40); //50 ms
	LPC_PINCON->PINSEL4    &= ~(1 << 22);     /* GPIO pin selection */
		enable_RIT();										/* enable RIT to count 50ms				 */
	LPC_SC->EXTINT &= (1 << 1);     /* clear pending interrupt         */
}

void EINT2_IRQHandler (void)	  	/* KEY2														 */
{
		NVIC_DisableIRQ(EINT2_IRQn);		/* disable Button interrupts			 */
		paused=1;
		button=2;
		init_RIT(0x004C4B40); //50 ms
	LPC_PINCON->PINSEL4    &= ~(1 << 24);     /* GPIO pin selection */
		enable_RIT();										/* enable RIT to count 50ms				 */
  LPC_SC->EXTINT &= (1 << 2);     /* clear pending interrupt         */    
}


