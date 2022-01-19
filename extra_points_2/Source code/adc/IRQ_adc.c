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
	#include <stdlib.h>

	int pixels_range_max = 190;
	unsigned short analog_range_max = 0xFFF;
	extern int paddle_x_p1;
	extern int paddle_x_p2;
	int paddle_p2_direction = -1;
	int paddle_p2_speed = 5;
	unsigned short values[] = {0x7FF,0x7FF,0x7FF,0x7FF,0x7FF}; //sembra quasi non servire a nulla, anche se metto un solo valore
	//funziona comunque, aggiungere valori rende solo più realistico il gioco facendo muovere il paddle con più coerenza senza
	//"teletrasportarsi" dall'altra parte dello schermo
	const int number_of_values = 5;
	unsigned short mean = 0x7FF;
	int circular_index = 0;
	unsigned short last_mean = 0x7FF;
	int ball_position [] = {230,157};
	int ball_movement [] = {-2,2};
	extern int current_score_p1;
	extern int current_score_p2;
	int flag_end_game = 0; //da controllare
	extern int game_status;
	int flag_lost_p1 = 0;
	int flag_lost_p2=0;
	int flag_score_p1_zone = 0;
	int flag_score_p2_zone = 1;
	int last_flag_score_p1_zone = 0;
	int last_flag_score_p2_zone = 1;
	int sound=0;

	/*----------------------------------------------------------------------------
		A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
	 *----------------------------------------------------------------------------*/

	unsigned short AD_current;   
	unsigned short AD_last = 0x7FF;     /* Last converted value               */
	
	void writeCurrentScoreP2(void){
			char r[]="";
			char text[]="P2:";
			sprintf(r,"%d", current_score_p2);
			strcat(text,r);
			GUI_Text_Reverse(223,155,(uint8_t*) text, White, Black); //write current score player 2
			return;
		}
		
		void writeCurrentScoreP1(void){
			char r[]="";
			char text[]="P1:";
			sprintf(r,"%d", current_score_p1);
			strcat(text,r);
			GUI_Text(8,155,(uint8_t*) text, White, Black); //write current score player 1
			return;
		}
	
	void check_border_to_update_direction(void){
		//Se sono vicino a un bordo laterale
		if (ball_position[0]==5 || ball_position[0]==230){
			//disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//inverti la direzione sulla x
			ball_movement[0]*=-1;
			//setta il flag sound
			sound=1;
			//setta il timer 2 e avvialo
			init_timer(2,1263*12);
			enable_timer(2);
		}
		return;
	}
	
	void onPaddle(int paddle_x, int dir){
		int paddle_center;
		int ball_center;
		float dist;
		//Se la palla si trova sul paddle disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//setta il flag di sound
		sound=1;
			//inizializza ed abilita il timer 2 che riproduce il suono
			init_timer(2,1062*12);
			enable_timer(2);	
			//calcola la posizione del centro del paddle (approssimata)
		paddle_center=paddle_x+19;
			//calcola la posizione del centro della palla
			ball_center=ball_position[0]+2;
			//calcola la distanza centro palla - centro paddle, normalizzala (/22) e moltiplica per 5
			dist=(ball_center-paddle_center)/(float)22*5;
			//setta il movimento sull'asse x con questo valore dist compreso tra 0 e 5
			ball_movement[0]=dist;
			//ma non voglio che sia zero, in tal caso
			if (ball_movement[0]==0){
				//controllo se la palla si trova a sinistra o destra del centro del paddle e setto il movimento a -1/+1 rispettivamente
				if (paddle_center<ball_center){
				ball_movement[0]=1;
				}
				else{
					ball_movement[0]=-1;
				}
			}
			if (ball_movement[0]>0){
				ball_movement[0]+=1; //sommo 1 così aumento il range tra 2 e 6, per far sembrare il gioco più veloce
			}
			else{
				ball_movement[0]-=1;
			}
			//Se dist è tra 0 e 3
			if (dist<=3){
				//setta il movimento sull'asse y a 4 o -4
				ball_movement[1]=4*dir;
			}
			else {
				//altrimenti, se è tra 3 e 5, setta il movimento sull'asse y a 2 o -2
				ball_movement[1]=2*dir;
			}
	}

	void check_paddle(void){
		if (ball_position[1]==273 && ((ball_position[0]<paddle_x_p1 && ball_position[0]+4>=paddle_x_p1) || 
			(ball_position[0]>=paddle_x_p1 && ball_position[0]+4<=paddle_x_p1+39) || 
		(ball_position[0]<=paddle_x_p1+39 && ball_position[0]+4>paddle_x_p1+39))){
			onPaddle(paddle_x_p1, -1);
		}
		else if (ball_position[1]==42 && ((ball_position[0]<paddle_x_p2 && ball_position[0]+4>=paddle_x_p2) || 
			(ball_position[0]>=paddle_x_p2 && ball_position[0]+4<=paddle_x_p2+39) || 
		(ball_position[0]<=paddle_x_p2+39 && ball_position[0]+4>paddle_x_p2+39))){
			onPaddle(paddle_x_p2, 1);
		}
		return;
	}

	void check_end_game(void){
		int i;
		char r[]="", cs[]="";
		char text2[]="P2:";
		char text1[]="P1:";
		sprintf(r,"%d", current_score_p2);
		strcat(text2,r);
		sprintf(cs,"%d", current_score_p1);
		strcat(text1, cs);
		if (ball_position[1]>=300){
			game_status=4;
			current_score_p2+=1;
			if (current_score_p2==5){
			//Cancella la scritta dello score
			LCD_DrawGameHBorder(233-10*4,155,234,14,Black);
				writeCurrentScoreP2();
			}
			else{
				LCD_DrawGameHBorder(233-10*4,155,234,14,Black);
			}
		}
		else if (ball_position[1]<=16){
			game_status=4;
			current_score_p1+=1;
			if (current_score_p1==5){
						//Cancella la scritta dello score
			LCD_DrawGameHBorder(8,155,100,14,Black);
			writeCurrentScoreP1();
			}
			else{
				//Cancella la scritta dello score
			LCD_DrawGameHBorder(8,155,100,14,Black);
			writeCurrentScoreP1();
				LCD_DrawGameHBorder(233-10*4,155,234,14,Black);
			}
		}
		if (current_score_p1<5 && current_score_p2<5 && (ball_position[1]>=300 || ball_position[1]<=16)){
			disable_timer(0);
			reset_timer(0);
			last_flag_score_p1_zone=0;
			last_flag_score_p2_zone=1;
			flag_score_p1_zone=0;
			flag_score_p2_zone=1;
			LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Black); //Delete ball
			ball_position[0]=230;
			ball_position[1]=157;
			ball_movement[0]=-2;
			ball_movement[1]=2;
			LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green); //Draw ball in initial position
			flag_lost_p1=0;
			flag_lost_p2=0;
			
		}
		if ((current_score_p1==5 || current_score_p2==5) && (ball_position[1]>=300 || ball_position[1]<=16)){
			flag_end_game=1;
			last_flag_score_p1_zone=0;
			last_flag_score_p2_zone=1;
			flag_score_p1_zone=0;
			flag_score_p2_zone=1;
			disable_timer(0);
			reset_timer(0);
			if (current_score_p1==5){
				GUI_Text_Reverse(140, 132, (uint8_t *) "You Lose", White, Black);
				GUI_Text(92, 180, (uint8_t *) "You Win", White, Black);
			}
			else{
				GUI_Text_Reverse(140, 132, (uint8_t *) "You Win", White, Black);
				GUI_Text(88, 180, (uint8_t *) "You Lose", White, Black);
			}
			current_score_p1=0;
			current_score_p2=0;
			mean = 0x7FF;
			for (i=0; i<number_of_values; i++){
				values[i]=mean;
			}
			circular_index = 0;
			ball_position[0]=230;
			ball_position[1]=157;
			ball_movement[0]=-2;
			ball_movement[1]=2;
			last_mean = 0x7FF;
			paddle_x_p1=100;
			paddle_x_p2=(rand()%191)+5;
			game_status=3;
		}
		else{
			if (game_status==4){
			enable_timer(1);
			}
		}
		return;
	}
	
	void remove_scores_if_ball_overlaps(void){
		char r[]="", cs[]="";
		char text2[]="P2:";
		char text1[]="P1:";
		sprintf(r,"%d", current_score_p2);
		strcat(text2,r);
		sprintf(cs,"%d", current_score_p1);
		strcat(text1, cs);
		if (ball_position[0]<8+4*10 && ball_position[1]>=150 && ball_position[1]<=170){
			//Se la palla si trova nella zona dove è riportato il current score di p1 aggiorna i flag
			flag_score_p1_zone=1;
			flag_score_p2_zone=0;
		}
		if (ball_position[0]>233-10*4 && ball_position[1]>=150 && ball_position[1]<=170){
			//Se la palla si trova nella zona dove è riportato il current score di p2 aggiorna i flag
			flag_score_p1_zone=0;
			flag_score_p2_zone=1;
		}
		if (!(ball_position[0]<8+4*10 && ball_position[1]>=150 && ball_position[1]<=170) && 
			!(ball_position[0]>233-10*4 && ball_position[1]>=150 && ball_position[1]<=170)){
				//Se la palla non si sovrappone al testo degli scores aggiorna i flag
				flag_score_p1_zone=0;
				flag_score_p2_zone=0;
			}
		if (flag_score_p1_zone && last_flag_score_p1_zone!= flag_score_p1_zone){
			//Se il flag è settato ed è diverso da prima vuol dire che la palla è appena entrata nella zona del current score di p1
			//Quindi cancella la zona
			LCD_DrawGameHBorder(8,155,100,14,Black);
		}
		else if (!flag_score_p1_zone && last_flag_score_p1_zone!= flag_score_p1_zone){
			//Se invece il flag non è settato ma è diverso da prima vuol dire che la palla è appena uscita dalla zona del current score di p1
			//Quindi riscrivi lo score
			writeCurrentScoreP1();
		}
		if (flag_score_p2_zone && last_flag_score_p2_zone!= flag_score_p2_zone){
			//Se il flag è settato ed è diverso da prima vuol dire che la palla è appena entrata nella zona del current score di p2
			//Quindi cancella la zona
			LCD_DrawGameHBorder(233-10*4,155,234,14,Black);
		}
		else if (!flag_score_p2_zone && last_flag_score_p2_zone!= flag_score_p2_zone){
			//Se invece il flag non è settato ma è diverso da prima vuol dire che la palla è appena uscita dalla zona del current score di p2
			//Quindi riscrivi il current score di p2
			writeCurrentScoreP2();
		}
		//Aggiorna i last flags
		last_flag_score_p2_zone=flag_score_p2_zone;
		last_flag_score_p1_zone=flag_score_p1_zone;
		return;
	}
	
	void check_left_right_margins(void){
		if (ball_position[0]+ball_movement[0]<5){
			ball_position[0]=5;
		}
		else if (ball_position[0]+ball_movement[0]>230){
			ball_position[0]=230;
		}
		else{
			ball_position[0]+=ball_movement[0];
		}
		return;
	}
	
	void check_top_paddle_bottom_paddle(void){
		//se la palla deve andare oltre il bordo di sopra non farla andare, falla al più toccare
		if(ball_position[1]+ball_movement[1]<42){
			//se la palla deve andare oltre il bordo logico di sopra
			//controlla se in realtà andrebbe a toccare il paddle, se sì allora falle al più toccare il paddle
			if ((ball_position[0]<paddle_x_p2 && ball_position[0]+4>=paddle_x_p2) || 
				(ball_position[0]>=paddle_x_p2 && ball_position[0]+4<=paddle_x_p2+39) || 
			(ball_position[0]<=paddle_x_p2+39 && ball_position[0]+4>paddle_x_p2+39)){
				ball_position[1]=42;
			}
			else{
				//altrimenti falla proseguire
			ball_position[1]+=ball_movement[1];
				//però setta il flag di sconfitta per p2 così non farò più questi controlli
				flag_lost_p2=1;
		}
		}
		else if (ball_position[1]+ball_movement[1]>273){
			//se la palla deve andare oltre il bordo logico di sotto
			
			//controlla se in realtà andrebbe a toccare il paddle, se sì allora falle al più toccare il paddle
			if ((ball_position[0]<paddle_x_p1 && ball_position[0]+4>=paddle_x_p1) || 
				(ball_position[0]>=paddle_x_p1 && ball_position[0]+4<=paddle_x_p1+39) || 
			(ball_position[0]<=paddle_x_p1+39 && ball_position[0]+4>paddle_x_p1+39)){
				ball_position[1]=273;
			}
			else{
				//altrimenti falla proseguire
			ball_position[1]+=ball_movement[1];
				//però setta il flag di sconfitta così non farò più questi controlli
				flag_lost_p1=1;
		}
		}
		else{
			//altrimenti in generale sposta la palla sulla y
			ball_position[1]+=ball_movement[1];
		}
		return;
	}

	void check_paddle_after_loss(void){
		if (ball_position[0]+4>=paddle_x_p1-1 && ball_position[0]+4<=paddle_x_p1+10 && ball_position[1]>273 && ball_position[1]<=287){
				//verifica che la palla tocchi il paddle inferiore da sinistra o lo intersechi da sinistra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
					//Se la palla tocca il paddle disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//setta il flag di sound
		sound=1;
			//inizializza ed abilita il timer 2 che riproduce il suono
			init_timer(2,1062*12);
			enable_timer(2);	
					ball_position[0]=paddle_x_p1-5;
					ball_movement[0]*=-1;
			}
			else if (ball_position[0]<=paddle_x_p1+40 && ball_position[0]>=paddle_x_p1+29 && ball_position[1]>273 && ball_position[1]<=287){
				//verifica che la palla tocchi il paddle inferiore da destra o lo intersechi da destra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
				//Se la palla tocca il paddle disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//setta il flag di sound
		sound=1;
			//inizializza ed abilita il timer 2 che riproduce il suono
			init_timer(2,1062*12);
			enable_timer(2);	
				ball_position[0]=paddle_x_p1+40;
					ball_movement[0]*=-1;
			}
		if (ball_position[0]+4>=paddle_x_p2-1 && ball_position[0]+4<=paddle_x_p2+10 && ball_position[1]>27 && ball_position[1]<=41){
				//verifica che la palla tocchi il paddle superiore da sinistra o lo intersechi da sinistra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
					//Se la palla tocca il paddle disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//setta il flag di sound
		sound=1;
			//inizializza ed abilita il timer 2 che riproduce il suono
			init_timer(2,1062*12);
			enable_timer(2);	
					ball_position[0]=paddle_x_p2-5;
					ball_movement[0]*=-1;
			}
			else if (ball_position[0]<=paddle_x_p2+40 && ball_position[0]>=paddle_x_p2+29 && ball_position[1]>27 && ball_position[1]<=41){
				//verifica che la palla tocchi il paddle superiore da destra o lo intersechi da destra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
				//Se la palla tocca il paddle disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//setta il flag di sound
		sound=1;
			//inizializza ed abilita il timer 2 che riproduce il suono
			init_timer(2,1062*12);
			enable_timer(2);	
				ball_position[0]=paddle_x_p2+40;
					ball_movement[0]*=-1;
			}
			return;
	}
	
	void check_side_borders_after_loss(void){
		//se la palla non ha superato la zona dove ci sono i bordi, considera i margini dei bordi per resettare
				if (ball_position[0]+ball_movement[0]<5){
			ball_position[0]=5;
		}
		else if (ball_position[0]+ball_movement[0]>230){
			ball_position[0]=230;
		}
		return;
	}
	
	void move_ball(void){
		//cancella la palla
		LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Black);
		if(!flag_lost_p1 && !flag_lost_p2){
			//rimuovi temporaneamente gli scores se la palla ci passa su e ripristinali appena esce dalla zona del testo
		remove_scores_if_ball_overlaps();
			//controlla se con la prossima mossa supero i margini destro e sinistro e nel caso riposiziona la palla
			check_left_right_margins();
			//controlla se con la prossima mossa supero il margine superiore o se supero la soglia definita dal paddle, andando a perdere la partita
			check_top_paddle_bottom_paddle();
		}
		if (flag_lost_p1 || flag_lost_p2){
			//se ho perso
			//aggiorna la posizione della palla
			ball_position[0]+=ball_movement[0];
			ball_position[1]+=ball_movement[1];
			//Verifica gli urti laterali con il paddle dopo la sconfitta
			check_paddle_after_loss();
			//Verifica comunque che non vengano superati i bordi laterali
			check_side_borders_after_loss();
		}
		//Ridisegna la pallina
		LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green);
		//Aggiorna la direzione se siamo vicino a un bordo
		check_border_to_update_direction();
		if (!flag_lost_p1 && !flag_lost_p2){
			check_paddle();
		}
		if (flag_lost_p1 || flag_lost_p2){
			check_end_game();
		}
		if (flag_end_game){
			flag_lost_p1=0;
			flag_lost_p2=0;
		}
		return;
	}
	
	unsigned short computeDifference(unsigned short val1, unsigned short val2){
		if(val1>val2){
				return val1-val2;
			}
			else{
				return val2-val1;
			}
	}
	
	void move_paddle_p2(void){
		int newPos=paddle_x_p2+paddle_p2_direction*paddle_p2_speed;
		if (newPos <5){
			newPos=5;
			paddle_p2_direction=1;
		}
		else if (newPos>195){
			newPos=195;
			paddle_p2_direction=-1;
		}
		else if (newPos==5 || newPos==195){
			paddle_p2_direction*=-1;
		}
		//controlla che l'utente non abbia ancora perso (prima condizione), che la palla non intersechi orizzontalmente il paddle
				//e che la palla non intersechi verticalmente il paddle
			if (!(flag_lost_p2 && ((ball_position[0]<newPos && ball_position[0]+4>=newPos-1) || 
				(ball_position[0]>=newPos && ball_position[0]+4<=newPos+39) || 
			(ball_position[0]<=newPos+39+1 && ball_position[0]+4>newPos+39)) && ball_position[1]>27 && ball_position[1]<=41)){
				//se sono tutte e tre vere ottengo 1, quindi 0 negando. Se almeno una è falsa ottengo 0, quindi 1 negando
				//aggiorna la posizione del paddle andando a cancellare e disegnare le porzioni di paddle
				if (newPos<=paddle_x_p2-40){
					//se il nuovo paddle è molto più a sinistra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x_p2,32,paddle_x_p2+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(newPos, 32,newPos+39,10,Green);//paddle
			}
			else if(newPos>paddle_x_p2-40 && newPos <paddle_x_p2){
				//se il nuovo paddle interseca da sinistra il vecchio paddle, cancella la porzione di destra del vecchio paddle
				//e disegna la porzione di sinistra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x_p2+(40-(paddle_x_p2-newPos)),32,paddle_x_p2+39,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(newPos, 32,paddle_x_p2-1,10,Green);//paddle portion
			}
			else if (newPos==paddle_x_p2){ //non fare nulla, vecchio e nuovo paddle coincidono, però serve la condizione altrimenti
				//entro nell'else finale
			} 
			else if (newPos>paddle_x_p2 && newPos<paddle_x_p2+40){
				//se il nuovo paddle interseca da destra il vecchio paddle, cancella la porzione di sinistra del vecchio paddle
				//e disegna la porzione di destra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x_p2,32,newPos-1,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(paddle_x_p2+40, 32,newPos+39,10,Green);//paddle portion
			}
			else{
				//se il nuovo paddle è molto più a destra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x_p2,32,paddle_x_p2+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(newPos, 32,newPos+39,10,Green);//paddle
			}
			paddle_x_p2=newPos;
		}
		return;
	}
	
	void move_paddle_p1(int pixels_range_start){
				//aggiorna la posizione del paddle andando a cancellare e disegnare le porzioni di paddle
				if (pixels_range_start<=paddle_x_p1-40){
					//se il nuovo paddle è molto più a sinistra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x_p1,278,paddle_x_p1+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,10,Green);//paddle
			}
			else if(pixels_range_start>paddle_x_p1-40 && pixels_range_start <paddle_x_p1){
				//se il nuovo paddle interseca da sinistra il vecchio paddle, cancella la porzione di destra del vecchio paddle
				//e disegna la porzione di sinistra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x_p1+(40-(paddle_x_p1-pixels_range_start)),278,paddle_x_p1+39,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(pixels_range_start, 278,paddle_x_p1-1,10,Green);//paddle portion
			}
			else if (pixels_range_start==paddle_x_p1){ //non fare nulla, vecchio e nuovo paddle coincidono, però serve la condizione altrimenti
				//entro nell'else finale
			} 
			else if (pixels_range_start>paddle_x_p1 && pixels_range_start<paddle_x_p1+40){
				//se il nuovo paddle interseca da destra il vecchio paddle, cancella la porzione di sinistra del vecchio paddle
				//e disegna la porzione di destra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x_p1,278,pixels_range_start-1,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(paddle_x_p1+40, 278,pixels_range_start+39,10,Green);//paddle portion
			}
			else{
				//se il nuovo paddle è molto più a destra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x_p1,278,paddle_x_p1+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,10,Green);//paddle
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
			diff=computeDifference(AD_current,AD_last);
					
			if (diff < (float)3/100*analog_range_max){  
				//controllo che la differenza tra l'ultimo valore registrato (AD_last) e quello appena letto (AD_current) sia piccola.
				//se trovo un valore molto distante la prima volta non viene registrato e va bene così, lo identifico come rumore.
				//è statisticamente improbabile che capiti un valore molto distante una seconda volta, di fila (ma anche se dovesse accadere,
				//il movimento del paddle ne risentirà in piccolissima parte grazie a questa implementazione).
				//se capita un valore molto distante per una seconda volta di fila, ma vicino al valore precedente, si entra in questo if 
				//perché la differenza tra il valore precedente e quello corrente è piccola, si salva il valore molto distante nel vettore values
				//e i suoi effetti saranno comunque attutiti dalla media che calcolo. Se invece è l'utente ad aver fatto un giro rapido del
				//potenziometro si ottengono in successione tanti valori molto diversi da quelli salvati in values, il primo valore sarà "scartato",
				//poi verranno messi in values e saranno considerati "validi". 
				
				//ok, register the value and compute new mean
				values[circular_index]=AD_current;
				for (i=0; i<number_of_values; i++){
					sum+=values[i];
				}
				mean=(unsigned short)sum/number_of_values;
				circular_index++;
				circular_index%=number_of_values;
			}
			
			//Ora calcolo la differenza tra l'ultima media e la media appena calcolata
			diff=computeDifference(mean, last_mean);
			//La differenza deve essere maggiore di una certa quantità minima per poter considerare la nuova media derivante da
			//una misurazione e non da rumore che la altera, quindi se è maggiore di questa quantità vado a calcolare la nuova posizione
			//del paddle pixels_range_start
			if (diff>(float)2/100*analog_range_max){
				pixels_range_start=(float)mean/analog_range_max*pixels_range_max+5;
				//renderizza il paddle
				//controlla che l'utente non abbia ancora perso (prima condizione), che la palla non intersechi orizzontalmente il paddle
				//e che la palla non intersechi verticalmente il paddle
			if (!(flag_lost_p1 && ((ball_position[0]<pixels_range_start && ball_position[0]+4>=pixels_range_start-1) || 
				(ball_position[0]>=pixels_range_start && ball_position[0]+4<=pixels_range_start+39) || 
			(ball_position[0]<=pixels_range_start+39+1 && ball_position[0]+4>pixels_range_start+39)) && ball_position[1]>273 && ball_position[1]<=287)){
				move_paddle_p1(pixels_range_start);
			
			//aggiorna la posizione del paddle
			paddle_x_p1=pixels_range_start;
			}
			//aggiorna la media solo se, appunto, è abbastanza distante dal valore precedente. In questo modo evito le piccole oscillazioni del
			//paddle
			last_mean=mean;
			}
			//aggiorna AD_last solo se AD_current è diverso
			AD_last = AD_current;
			}
			move_paddle_p2();
			//renderizza e aggiorna la posizione della palla
			move_ball();
			if (flag_end_game==0 && game_status!=4 && sound==0){
		enable_timer(0);
			}
	}
