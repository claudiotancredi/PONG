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
#include <stdio.h>

int pixels_range_max = 199;
unsigned short analog_range_max = 0xFFF;
extern int paddle_x;
unsigned short values[] = {0x7FF,0x7FF,0x7FF,0x7FF,0x7FF,0x7FF,0x7FF}; //sembra quasi non servire a nulla, anche se metto un solo valore
//funziona comunque, aggiungere valori rende solo più realistico il gioco facendo muovere il paddle con più coerenza senza
//"teletrasportarsi" dall'altra parte dello schermo
unsigned short mean = 0x7FF;
int circular_index = 0;
unsigned short last_mean = 0x7FF;
int ball_position [] = {230,157};
int ball_movement [] = {-1,+1};
extern int current_score;
extern int record;
int flag_end_game = 0;

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
 *----------------------------------------------------------------------------*/

unsigned short AD_current;   
unsigned short AD_last = 0x7FF;     /* Last converted value               */

void check_border(void){
	if (ball_position[0]==5 || ball_position[0]==230){
		ball_movement[0]*=-1;
	}
	if (ball_position[1]==5){
		ball_movement[1]*=-1;
	}
	return;
}

void check_paddle(void){
	char r[]="";
	int paddle_center;
	int ball_center;
	float dist;
	if (ball_position[1]==273 && ((ball_position[0]<paddle_x && ball_position[0]+4>=paddle_x) || (ball_position[0]>=paddle_x && ball_position[0]+4<=paddle_x+39) || (ball_position[0]<=paddle_x+39 && ball_position[0]+4>paddle_x+39))){
		paddle_center=paddle_x+19;
		ball_center=ball_position[0]+2;
		dist=(ball_center-paddle_center)/(float)21*5;
		ball_movement[0]=dist;
		ball_movement[1]*=-1;
		if (current_score<=100){
			current_score+=5;
		}
		else{
			current_score+=10;
		}
		sprintf(r,"%d", current_score);
		GUI_Text(8,156,(uint8_t*) r, White, Black);
	}
	return;
}

void check_end_game(void){
	int i;
	char r[]="";
	int len_cs, len_rec;
	char text1 []= "";
	char text2 [] = "";
	if (ball_position[1]>=300){
		sprintf(r,"%d", current_score);
		len_cs=strlen(r);
		while (len_cs>0){
			strcat(text1, " ");
			len_cs--;
		}
		sprintf(r,"%d", record);
		len_rec=strlen(r);
		while (len_rec>0){
			strcat(text2, " ");
			len_rec--;
		}
		flag_end_game=1;
		disable_timer(0);
		reset_timer(0);
		GUI_Text(233-9*strlen(r),8,(uint8_t*) text2, White, Black);
		GUI_Text(8,156,(uint8_t*) text1, White, Black);
		if (current_score>record){
			record=current_score;
		}
		current_score=0;
		mean = 0x7FF;
		for (i=0; i<7; i++){
			values[i]=mean;
		}
		circular_index = 0;
		last_mean = 0x7FF;
		ball_position[0]=230;
		ball_position[1]=157;
		ball_movement[0]=-1;
		ball_movement[1]=1;
		GUI_Text(88, 155, (uint8_t *) "You Lose", White, Black);
	}
	return;
}

void move_ball(void){
	char r[]="";
	static int flag_lost = 0;
	LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,ball_position[1],5,Black);
	sprintf(r,"%d", current_score);
	if (ball_position[0]<8+strlen(r)*11 && ball_position[1]>=154 && ball_position[1]<=166){
		GUI_Text(8,156,(uint8_t*) r, White, Black);
	}
	sprintf(r,"%d", record);
	if (ball_position[0]>233-11*strlen(r) && ball_position[1]>=5 && ball_position[1]<=19){
		GUI_Text(233-9*strlen(r),8,(uint8_t*) r, White, Black);
	}
	//se ancora non ho perso
	//se la palla deve andare oltre il bordo di sopra non farla andare, falla al più toccare
	if (ball_position[1]+ball_movement[1]<5){
		ball_position[1]=5;
	}
	else if (ball_position[1]+ball_movement[1]>273){
		//se la palla deve andare oltre il bordo logico di sotto
		
		//controlla se in realtà andrebbe a toccare il paddle, se sì allora falle al più toccare il paddle
		//MANCA IL CHECK SU BALL POSITION 0 E BALL MOVEMENT 0
		if (!flag_lost && ((ball_position[0]+ball_movement[0]<paddle_x && ball_position[0]+ball_movement[0]+4>=paddle_x) || (ball_position[0]+ball_movement[0]>=paddle_x && ball_position[0]+ball_movement[0]+4<=paddle_x+39) || (ball_position[0]+ball_movement[0]<=paddle_x+39 && ball_position[0]+ball_movement[0]+4>paddle_x+39))){
			ball_position[1]=273;
		}
		else{
			//altrimenti falla proseguire
		ball_position[1]+=ball_movement[1];
			//però setta il flag di sconfitta così non farò più questi controlli
			flag_lost=1;
	}
	}
	else{
		//altrimenti in generale sposta la palla sulla y
		ball_position[1]+=ball_movement[1];
	}
	//controlla i margini destro e sinistro e nel caso riposiziona la palla
	if (ball_position[0]+ball_movement[0]<5){
		ball_position[0]=5;
	}
	else if (ball_position[0]+ball_movement[0]>230){
		ball_position[0]=230;
	}
	else{
		ball_position[0]+=ball_movement[0];
	}
	LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,ball_position[1],5,Green);
	check_border();
	if (!flag_lost){
		check_paddle();
	}
	check_end_game();
	if (flag_end_game){
		flag_lost=0;
	}
	return;
}

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
		move_ball();
		if (!flag_end_game){
			init_timer(0,0x618A); //1 ms
		//init_timer(0, 0x98968 ); 						/* 25ms * 25MHz = 6.25*10^5 = 0x98968 */
	enable_timer(0);
		}
}
