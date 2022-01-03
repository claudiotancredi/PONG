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
	unsigned short values[] = {0x7FF,0x7FF,0x7FF,0x7FF,0x7FF}; //sembra quasi non servire a nulla, anche se metto un solo valore
	//funziona comunque, aggiungere valori rende solo più realistico il gioco facendo muovere il paddle con più coerenza senza
	//"teletrasportarsi" dall'altra parte dello schermo
	const int number_of_values = 5;
	unsigned short mean = 0x7FF;
	int circular_index = 0;
	unsigned short last_mean = 0x7FF;
	int ball_position [] = {230,157};
	int ball_movement [] = {-1,+1};
	extern int current_score;
	extern int record;
	int flag_end_game = 0;
	extern int game_status;
	int flag_lost = 0;
	int flag_score_zone = 0;
	int flag_record_zone = 0;
	int last_flag_score_zone = 0;
	int last_flag_record_zone = 0;
	int sound=0;

	/*----------------------------------------------------------------------------
		A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
	 *----------------------------------------------------------------------------*/

	unsigned short AD_current;   
	unsigned short AD_last = 0x7FF;     /* Last converted value               */

	void check_border_to_update_direction(void){
		//Se sono vicino a un bordo laterale
		if (ball_position[0]==5 || ball_position[0]==230 || ball_position[0]==0 || ball_position[0]==235){
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
		if (ball_position[1]==5){ //se sono vicino al bordo superiore
			//disabilita il timer 0
			disable_timer(0);
			reset_timer(0);
			//inverti la direzione sulla y
			ball_movement[1]*=-1;
			//setta il flag sound
			sound=1;
			//setta il timer 2 e avvialo
			init_timer(2,1263*12);
			enable_timer(2);
		}
		return;
	}

	void check_paddle(void){
		char r[]="";
		int paddle_center;
		int ball_center;
		float dist;
		if (ball_position[1]==273 && ((ball_position[0]<paddle_x && ball_position[0]+4>=paddle_x) || 
			(ball_position[0]>=paddle_x && ball_position[0]+4<=paddle_x+39) || 
		(ball_position[0]<=paddle_x+39 && ball_position[0]+4>paddle_x+39))){
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
			//Se dist è tra 0 e 3
			if (dist<=3){
				//setta il movimento sull'asse y a -2
				ball_movement[1]=-3;
			}
			else {
				//altrimenti, se è tra 3 e 5, setta il movimento sull'asse y a -1
				ball_movement[1]=-1;
			}
			if (current_score<=100){
				//se lo score è <=100 aggiungi 5
				current_score+=5;
			}
			else{
				//altrimenti aggiungi 10
				current_score+=10;
			}
			//stampa il nuovo score
			sprintf(r,"%d", current_score);
			GUI_Text(8,155,(uint8_t*) r, White, Black);
		}
		return;
	}

	void check_end_game(void){
		int i;
		char r[]="";
		char newRec [] = "New record: ";
		if (ball_position[1]>=300){	
			sprintf(r,"%d", record);
			//Cancella la scritta del record
			LCD_DrawGameHBorder(233-11*strlen(r),8,234,12,Black);
			sprintf(r,"%d", current_score);
			//Cancella la scritta dello score
			LCD_DrawGameHBorder(8,155,100,12,Black);
			//Setta il flag di sconfitta a 1
			flag_end_game=1;
			//Ferma il timer 0
			disable_timer(0);
			reset_timer(0);
			if (current_score>record){
				record=current_score;
				sprintf(r,"%d", record);
				strcat(newRec, r);
				GUI_Text(88, 147, (uint8_t *) "You Lose", White, Black);
				GUI_Text(60, 165, (uint8_t *) newRec, White, Black);
			}
			else{
				GUI_Text(88, 155, (uint8_t *) "You Lose", White, Black);
			}
			current_score=0;
			mean = 0x7FF;
			for (i=0; i<number_of_values; i++){
				values[i]=mean;
			}
			circular_index = 0;
			last_mean = 0x7FF;
			ball_position[0]=230;
			ball_position[1]=157;
			ball_movement[0]=-1;
			ball_movement[1]=1;
			paddle_x=100;
			game_status=3;
		}
		return;
	}
	
	void remove_scores_if_ball_overlaps(void){
		char r[]="", cs[]="";
		sprintf(cs,"%d", current_score);
		sprintf(r,"%d", record);
		if (ball_position[0]<8+strlen(cs)*11 && ball_position[1]>=154 && ball_position[1]<=168){
			//Se la palla si trova nella zona dove è riportato il current score aggiorna i flag
			flag_score_zone=1;
			flag_record_zone=0;
		}
		if (ball_position[0]>233-11*strlen(r) && ball_position[1]>=5 && ball_position[1]<=19){
			//Se la palla si trova nella zona dove è riportato il record aggiorna i flag
			flag_score_zone=0;
			flag_record_zone=1;
		}
		if (!(ball_position[0]<8+strlen(cs)*11 && ball_position[1]>=154 && ball_position[1]<=168) && 
			!(ball_position[0]>233-11*strlen(r) && ball_position[1]>=5 && ball_position[1]<=19)){
				//Se la palla non si sovrappone al testo degli scores aggiorna i flag
				flag_score_zone=0;
				flag_record_zone=0;
			}
		if (flag_score_zone && last_flag_score_zone!= flag_score_zone){
			//Se il flag è settato ed è diverso da prima vuol dire che la palla è appena entrata nella zona del current score
			//Quindi cancella la zona
			LCD_DrawGameHBorder(8,155,100,12,Black);
		}
		else if (!flag_score_zone && last_flag_score_zone!= flag_score_zone){
			//Se invece il flag non è settato ma è diverso da prima vuol dire che la palla è appena uscita dalla zona del current score
			//Quindi riscrivi lo score
			GUI_Text(8,155,(uint8_t*) cs, White, Black);
		}
		if (flag_record_zone && last_flag_record_zone!= flag_record_zone){
			//Se il flag è settato ed è diverso da prima vuol dire che la palla è appena entrata nella zona del record
			//Quindi cancella la zona
			LCD_DrawGameHBorder(233-11*strlen(r),8,234,12,Black);
		}
		else if (!flag_record_zone && last_flag_record_zone!= flag_record_zone){
			//Se invece il flag non è settato ma è diverso da prima vuol dire che la palla è appena uscita dalla zona del record
			//Quindi riscrivi il record
			GUI_Text(233-9*strlen(r),8,(uint8_t*) r, White, Black);
		}
		//Aggiorna i last flags
		last_flag_record_zone=flag_record_zone;
		last_flag_score_zone=flag_score_zone;
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
	
	void check_top_margin_bottom_paddle(void){
		//se la palla deve andare oltre il bordo di sopra non farla andare, falla al più toccare
		if (ball_position[1]+ball_movement[1]<5){
			ball_position[1]=5;
		}
		else if (ball_position[1]+ball_movement[1]>273){
			//se la palla deve andare oltre il bordo logico di sotto
			
			//controlla se in realtà andrebbe a toccare il paddle, se sì allora falle al più toccare il paddle
			if ((ball_position[0]<paddle_x && ball_position[0]+4>=paddle_x) || 
				(ball_position[0]>=paddle_x && ball_position[0]+4<=paddle_x+39) || 
			(ball_position[0]<=paddle_x+39 && ball_position[0]+4>paddle_x+39)){
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
		return;
	}

	void check_paddle_after_loss(void){
		if (ball_position[0]+4>=paddle_x-1 && ball_position[0]+4<=paddle_x+10 && ball_position[1]>273 && ball_position[1]<=287){
				//verifica che la palla tocchi il paddle da sinistra o lo intersechi da sinistra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
					ball_position[0]=paddle_x-5;
					ball_movement[0]*=-1;
			}
			else if (ball_position[0]<=paddle_x+40 && ball_position[0]>=paddle_x+29 && ball_position[1]>273 && ball_position[1]<=287){
				//verifica che la palla tocchi il paddle da destra o lo intersechi da destra, se ciò accade fai in modo che 
			//la tocchi senza intersecarla e cambi la direzione lungo l'asse x
				ball_position[0]=paddle_x+40;
					ball_movement[0]*=-1;
			}
			return;
	}
	
	void check_side_borders_after_loss(void){
		//se la palla ha superato la zona in cui ci sono i bordi
		if (ball_position[1]>=278){
				//e se la palla vuole andare oltre il bordo sinistro del display
				if (ball_position[0]+ball_movement[0]<0){
					//resetta la posizione a 0 così da farle toccare il bordo sinistro del display ma non farla andare oltre
			ball_position[0]=0;
		}
		else if (ball_position[0]+ball_movement[0]>235){
			//altrimenti, se la palla vuole andare oltre il bordo destro del display, resetta la posizione a 235 così da farle 
			//toccare il bordo destro del display ma non farla andare oltre
			ball_position[0]=235;
		}
			}
			else{ //altrimenti, se la palla non ha superato la zona dove ci sono i bordi, considera i margini dei bordi per resettare
				if (ball_position[0]+ball_movement[0]<5){
			ball_position[0]=5;
		}
		else if (ball_position[0]+ball_movement[0]>230){
			ball_position[0]=230;
		}
			}
		return;
	}
	
	void move_ball(void){
		//cancella la palla
		LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Black);
		if(!flag_lost){
			//rimuovi temporaneamente gli scores se la palla ci passa su e ripristinali appena esce dalla zona del testo
		remove_scores_if_ball_overlaps();
			//controlla se con la prossima mossa supero i margini destro e sinistro e nel caso riposiziona la palla
			check_left_right_margins();
			//controlla se con la prossima mossa supero il margine superiore o se supero la soglia definita dal paddle, andando a perdere la partita
			check_top_margin_bottom_paddle();
		}
		if (flag_lost){
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
		if (!flag_lost){
			check_paddle();
		}
		check_end_game();
		if (flag_end_game){
			flag_lost=0;
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
	
	void move_paddle(int pixels_range_start){
		//controlla che l'utente non abbia ancora perso (prima condizione), che la palla non intersechi orizzontalmente il paddle
				//e che la palla non intersechi verticalmente il paddle
			if (!(flag_lost && ((ball_position[0]<pixels_range_start && ball_position[0]+4>=pixels_range_start) || 
				(ball_position[0]>=pixels_range_start && ball_position[0]+4<=pixels_range_start+39) || 
			(ball_position[0]<=pixels_range_start+39 && ball_position[0]+4>pixels_range_start+39)) && ball_position[1]>273 && ball_position[1]<=287)){
				//aggiorna la posizione del paddle andando a cancellare e disegnare le porzioni di paddle
				if (pixels_range_start<=paddle_x-40){
					//se il nuovo paddle è molto più a sinistra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,10,Green);//paddle
			}
			else if(pixels_range_start>paddle_x-40 && pixels_range_start <paddle_x){
				//se il nuovo paddle interseca da sinistra il vecchio paddle, cancella la porzione di destra del vecchio paddle
				//e disegna la porzione di sinistra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x+(40-(paddle_x-pixels_range_start)),278,paddle_x+39,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(pixels_range_start, 278,paddle_x-1,10,Green);//paddle portion
			}
			else if (pixels_range_start==paddle_x){ //non fare nulla, vecchio e nuovo paddle coincidono, però serve la condizione altrimenti
				//entro nell'else finale
			} 
			else if (pixels_range_start>paddle_x && pixels_range_start<paddle_x+40){
				//se il nuovo paddle interseca da destra il vecchio paddle, cancella la porzione di sinistra del vecchio paddle
				//e disegna la porzione di destra del nuovo paddle mantenendo i pixel in comune
				LCD_DrawGameHBorder(paddle_x,278,pixels_range_start-1,10,Black); //Delete paddle portion
				LCD_DrawGameHBorder(paddle_x+40, 278,pixels_range_start+39,10,Green);//paddle portion
			}
			else{
				//se il nuovo paddle è molto più a destra cancella il vecchio e disegna il nuovo
				LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,10,Black); //Delete paddle
				LCD_DrawGameHBorder(pixels_range_start, 278,pixels_range_start+39,10,Green);//paddle
			}
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
				pixels_range_start=(float)mean/analog_range_max*pixels_range_max;
				//renderizza il paddle
				move_paddle(pixels_range_start);
			
			//aggiorna la posizione del paddle
			paddle_x=pixels_range_start;
			//aggiorna la media solo se, appunto, è abbastanza distante dal valore precedente. In questo modo evito le piccole oscillazioni del
			//paddle
			last_mean=mean;
			}
			//aggiorna AD_last solo se AD_current è diverso
			AD_last = AD_current;
			}
			//renderizza e aggiorna la posizione della palla
			move_ball();
			if (flag_end_game==0 && sound==0){
		enable_timer(0);
			}
	}
