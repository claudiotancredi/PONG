/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_adc.c
** Last modified Date:  20184-12-30
** Last Version:        V1.00
** Descriptions:        functions to manage A/D interrupts
** Correlated files:    adc.h
**--------------------------------------------------------------------------------------------------------       
*********************************************************************************************************/

#include "lpc17xx.h"
#include "adc.h"
#include "../GLCD/GLCD.h" 

int pixels_range_max = 200;
unsigned short analog_range_max = 0xFFF;
extern int paddle_x;

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
 *----------------------------------------------------------------------------*/

unsigned short AD_current;   
unsigned short AD_last = 0x7FF;     /* Last converted value               */

void ADC_IRQHandler(void) {
	unsigned short diff;
  int pixels_range_start;
  AD_current = ((LPC_ADC->ADGDR>>4) & 0xFFF);/* Read Conversion Result             */
  if(AD_current != AD_last){
		if(AD_current>AD_last){
			diff=AD_current-AD_last;
		}
		else{
			diff=AD_last-AD_current;
		}
		if(diff >(float)3/100*analog_range_max && diff <(float)8/100*analog_range_max){
			pixels_range_start=(float)AD_current/analog_range_max*pixels_range_max;
		
		pixels_range_start=pixels_range_start/6*6;
			
		if (pixels_range_start<=paddle_x-40){
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+40,278,10,Black); //Delete paddle
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+40,278,10,Green);//paddle
		}
		else if(pixels_range_start>paddle_x-40 && pixels_range_start <paddle_x){
			LCD_DrawGameHBorder(paddle_x+(40-(paddle_x-pixels_range_start)),278,paddle_x+40,278,10,Black); //Delete paddle portion
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+(paddle_x-pixels_range_start),278,10,Green);//paddle
		}
		else if (pixels_range_start==paddle_x){
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+40,278,10,Green);//paddle
		}
		else if (pixels_range_start>paddle_x && pixels_range_start<=paddle_x+40){
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+(pixels_range_start-paddle_x),278,10,Black); //Delete paddle portion
			LCD_DrawGameHBorder(paddle_x+40, 278,pixels_range_start+(pixels_range_start-paddle_x),278,10,Green);//paddle
		}
		else{
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+40,278,10,Black); //Delete paddle
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+40,278,10,Green);//paddle
		}
		paddle_x=pixels_range_start;
		
		AD_last = AD_current;
		}
		
		
  }	
}
