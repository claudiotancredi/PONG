		#include "../GLCD/GLCD.h" 
		#include "../timer/timer.h"
		#include "../adc/adc.h"
		#include <stdio.h>

		extern int game_status;
		extern int sound;
		extern int ball_position[];
		extern int flag_end_game;
	
		int current_score_p1 = 0;
		int current_score_p2 = 0;
		
		
		void resetDisplayAndShowInitialMessage(void){
			LCD_Clear(Black);
			GUI_Text(104, 147, (uint8_t *) "PONG", White, Black);
			GUI_Text(44, 165, (uint8_t *) "Press KEY1 to start", White, Black);
			return;
		}
			
		void drawGameBordersAndBallAndPaddles(void){
			LCD_DrawGameVBorders(0, 0,320,235,5,Red); //Side borders, includes drawing the ball and the paddles
			return;
		}
		
		void writeCurrentScoreP1start(void){
			char r[]="";
			char text[]="P1:";
			sprintf(r,"%d", current_score_p1);
			strcat(text,r);
			GUI_Text(8,155,(uint8_t*) text, White, Black); //write current score player 1
			return;
		}
		
		void stopPaddleAndBallAndSound(void){
			disable_timer(2);
			disable_timer(0);
			reset_timer(0);
			return;
		}
		
		void writePausedText(void){
			if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Black);
				}
				GUI_Text(95,155, (uint8_t*) "Paused", White, Black);
				if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green);
				}
				return;
		}
		
		void clearPausedText(void){
			LCD_DrawGameHBorder(95, 155, 150, 12, Black);
				if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green);
				}
				return;
		}
		
		void restartPaddleAndBall(void){
			sound=0; //necessario in caso si sia messo pausa durante il suono, altrimenti non fa l'enable a fine ADC handler
			enable_timer(0);
		}

		void reset_game(void){
			flag_end_game=0;
			resetDisplayAndShowInitialMessage();
			game_status=0;
			return;
		}

		void start_game(void){
			ADC_init();	
			LCD_DrawGameHBorder(44,147,230,30,Black); //Delete text
			drawGameBordersAndBallAndPaddles();
			writeCurrentScoreP1start();
			init_timer(1, 0x17D7840); /*1 s * 25 MHz = 25*10^6=0x17D7840       timer for countdown*/
			enable_timer(1);
			return;
		}

		void pause_game(void){
			if (game_status==1){
				stopPaddleAndBallAndSound();
				writePausedText();
				game_status=2;
			}
			else{
				clearPausedText();
				restartPaddleAndBall();
				game_status=1;
			}
			return;
		}
