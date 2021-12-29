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
#include "../timer/timer.h"

int pixels_range_max = 199;
unsigned short analog_range_max = 0xFFF;
extern int paddle_x;
unsigned short values[] = {0x7FF,0x7FF,0x7FF,0x7FF,0x7FF,0x7FF,0x7FF};
unsigned short mean = 0x7FF;
int circular_index = 0;
unsigned short last_mean = 0x7FF;

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
 *----------------------------------------------------------------------------*/

unsigned short AD_current;   
unsigned short AD_last = 0x7FF;     /* Last converted value               */

void ADC_IRQHandler(void) {
	int i;
	unsigned int sum;
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
		if (diff < (float)3/100*analog_range_max){  //diff agisce su AD_last e AD_current, quindi
			//se trovo un valore sballato la prima volta non viene registrato e va bene così, è
			//improbabile che capiti un valore sballato una seconda volta, se capita un valore sballato
			//la seconda volta e si entra in questo if si salva il valore sballato, i cui effetti saranno comunque attutiti dalla
			//media che calcolo. Se invece è l'utente a fare un giro rapido del potenziometro e si ottengono in successione tanti
			//valori molto diversi da quelli salvati in values, il primo valore sarà scartato, poi verranno messi in values e saranno
			//considerati validi.
			
			//ok, register the value and compute new mean
			values[circular_index]=AD_current;
			for (i=0; i<7; i++){
				sum+=values[i];
			}
			mean=(unsigned short)sum/7;
			circular_index++;
			circular_index%=7;
		}
		
		if (mean>last_mean){
			diff = mean-last_mean;
		}
		else{
			diff=last_mean-mean;
		}
		if (diff>(float)2/100*analog_range_max){
			pixels_range_start=(float)mean/analog_range_max*pixels_range_max;
			
		if (pixels_range_start<=paddle_x-40){
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,278,10,Black); //Delete paddle
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,278,10,Green);//paddle
		}
		else if(pixels_range_start>paddle_x-40 && pixels_range_start <paddle_x){
			LCD_DrawGameHBorder(paddle_x+(40-(paddle_x-pixels_range_start)),278,paddle_x+39,278,10,Black); //Delete paddle portion
			LCD_DrawGameHBorder(pixels_range_start, 278,paddle_x-1,278,10,Green);//paddle
		}
		else if (pixels_range_start==paddle_x){
		}
		else if (pixels_range_start>paddle_x && pixels_range_start<paddle_x+40){
			LCD_DrawGameHBorder(paddle_x,278,pixels_range_start-1,278,10,Black); //Delete paddle portion
			LCD_DrawGameHBorder(paddle_x+40, 278,pixels_range_start+39,278,10,Green);//paddle
		}
		else{
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,278,10,Black); //Delete paddle
			LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,278,10,Green);//paddle
		}
		paddle_x=pixels_range_start;
		last_mean=mean;
		}
		
			
		
		AD_last = AD_current;
		}
		
		init_timer(0, 0x98968 ); 						/* 25ms * 25MHz = 6.25*10^5 = 0x98968 */
	enable_timer(0);
  
}
